#include "StarLuaGameConverters.hpp"
#include "StarStatusController.hpp"
#include "StarActorMovementController.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorld.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarMonster.hpp"
#include "StarNpc.hpp"
#include "StarStagehand.hpp"
#include "StarVehicle.hpp"
#include "StarContainerObject.hpp"
#include "StarFarmableObject.hpp"
#include "StarLoungeableObject.hpp"
#include "StarProjectile.hpp"
#include "StarItemDrop.hpp"
#include "StarItemDatabase.hpp"
#include "StarItem.hpp"
#include "StarRoot.hpp"

namespace Star {

LuaMethods<EntityPtr> LuaUserDataMethods<EntityPtr>::make() {
    LuaMethods<EntityPtr> methods;

    // general entity methods
    methods.registerMethod("exists",
    [&](EntityPtr const& entity) -> bool {
        return entity->inWorld();
    });

    methods.registerMethod("id",
    [&](EntityPtr const& entity) -> EntityId {
        return entity->entityId();
    });

    methods.registerMethod("canDamage",
    [&](EntityPtr const& entity, EntityId const& otherId) -> bool {
        if (entity->inWorld()) {
            auto other = entity->world()->entity(otherId);
            
            if (!other || !entity->getTeam().canDamage(other->getTeam(), false))
            return false;
            
            return true;
        }
        return false;
    });

    methods.registerMethod("damageTeam",
    [&](EntityPtr const& entity) -> Json {
        return entity->getTeam().toJson();
    });

    methods.registerMethod("aggressive",
    [&](EntityPtr const& entity) -> Json {
        if (auto monster = as<Monster>(entity))
            return monster->aggressive();
        if (auto npc = as<Npc>(entity))
            return npc->aggressive();
        return false;
    });

    methods.registerMethod("type",
    [&](EntityPtr const& entity, LuaEngine& engine) -> LuaString {
        return engine.createString(EntityTypeNames.getRight(entity->entityType()));
    });

    methods.registerMethod("typeName",
    [&](EntityPtr const& entity, LuaEngine& engine) -> Maybe<String> {
        if (auto monster = as<Monster>(entity))
            return monster->typeName();
        if (auto npc = as<Npc>(entity))
            return npc->npcType();
        if (auto vehicle = as<Vehicle>(entity))
            return vehicle->name();
        if (auto object = as<Object>(entity))
            return object->name();
        if (auto itemDrop = as<ItemDrop>(entity)) {
            if (itemDrop->item())
            return itemDrop->item()->name();
        }
        return {};
    });

    methods.registerMethod("position",
    [&](EntityPtr const& entity) -> Vec2F {
        return entity->position();
    });

    methods.registerMethod("metaBoundBox",
    [&](EntityPtr const& entity) -> RectF {
        return entity->metaBoundBox();
    });

    methods.registerMethod("velocity",
    [&](EntityPtr const& entity) -> Maybe<Vec2F> {
        if (auto monsterEntity = as<Monster>(entity))
            return monsterEntity->velocity();
        else if (auto toolUserEntity = as<ToolUserEntity>(entity))
            return toolUserEntity->velocity();
        else if (auto vehicleEntity = as<Vehicle>(entity))
            return vehicleEntity->velocity();
        else if (auto projectileEntity = as<Projectile>(entity))
            return projectileEntity->velocity();

        return {};
    });

    methods.registerMethod("name",
    [&](EntityPtr const& entity) -> String {
        return entity->name();
    });

    methods.registerMethod("description",
    [&](EntityPtr const& entity, Maybe<String> const& species) -> Maybe<String> {
        if (auto inspectableEntity = as<InspectableEntity>(entity)) {
            if (species)
            return inspectableEntity->inspectionDescription(*species);
        }

        return entity->description();
    });

    methods.registerMethod("uniqueId",
    [&](EntityPtr const& entity) -> LuaNullTermWrapper<Maybe<String>> {
        return entity->uniqueId();
    });

    methods.registerMethod("getParameter",
    [&](EntityPtr const& entity, String const& parameterName, Maybe<Json> const& defaultValue) -> Json {
        Json val = Json();
        
        bool handled = true;
        if (auto objectEntity = as<Object>(entity)) {
            val = objectEntity->configValue(parameterName);
        } else if (auto npcEntity = as<Npc>(entity)) {
            val = npcEntity->scriptConfigParameter(parameterName);
        } else if (auto projectileEntity = as<Projectile>(entity)) {
            val = projectileEntity->configValue(parameterName);
        } else if (auto stagehandEntity = as<Stagehand>(entity)) {
            val = stagehandEntity->configValue(parameterName);
        } else {
            handled = false;
        }
        if (!val && defaultValue && handled)
            val = *defaultValue;

        return val;
    });

    methods.registerMethod("sendMessage",
    [&](EntityPtr const& entity, String const& message, LuaVariadic<Json> args) -> RpcPromise<Json> {
        if (entity->inWorld()) {
            return entity->world()->sendEntityMessage(entity->entityId(), message, JsonArray::from(std::move(args)));
        }
        return RpcPromise<Json>::createFailed("Entity not in world");
    });

    // scripted entity methods
    methods.registerMethod("callScript",
    [&](EntityPtr const& entity, String const& function, LuaVariadic<LuaValue> const& args) -> Maybe<LuaValue> {
        auto scrEntity = as<ScriptedEntity>(entity);
        if (!scrEntity || !scrEntity->isMaster() || !entity->inWorld())
            throw StarException::format("Entity {} does not exist or is not a local master scripted entity", entity->entityId());
        return scrEntity->callScript(function, args);
    });

    // nametag entity methods
    methods.registerMethod("nametag",
    [&](EntityPtr const& entity) -> Maybe<Json> {
        Json result;
        if (auto nametagEntity = as<NametagEntity>(entity)) {
            result = JsonObject{
                {"nametag", nametagEntity->nametag()},
                {"displayed", nametagEntity->displayNametag()},
                {"color", jsonFromColor(Color::rgb(nametagEntity->nametagColor()))},
                {"origin", jsonFromVec2F(nametagEntity->nametagOrigin())},
            };
            if (auto status = nametagEntity->statusText())
                result.set("status", *status);
        }

        return result;
    });

    // portrait entity methods
    methods.registerMethod("portrait",
    [&](EntityPtr const& entity, String const& portraitMode) -> LuaNullTermWrapper<Maybe<List<Drawable>>> {
        if (auto portraitEntity = as<PortraitEntity>(entity))
            return portraitEntity->portrait(PortraitModeNames.getLeft(portraitMode));

        return {};
    });


    // damage bar entity methods
    methods.registerMethod("health",
    [&](EntityPtr const& entity) -> Maybe<Vec2F> {
        if (auto dmgEntity = as<DamageBarEntity>(entity)) {
            return Vec2F(dmgEntity->health(), dmgEntity->maxHealth());
        }
        return {};
    });

    // interactive entity methods
    methods.registerMethod("isInteractive",
    [&](EntityPtr const& entity) -> Maybe<bool> {
        if (auto interactEntity = as<InteractiveEntity>(entity))
            return interactEntity->isInteractive();
        return {};
    });

    // chatty entity methods
    methods.registerMethod("mouthPosition",
    [&](EntityPtr const& entity) -> Maybe<Vec2F> {
        if (auto chatty = as<ChattyEntity>(entity))
            return chatty->mouthPosition();
        return {};
    });

    // actor entity methods

    // status controller methods, they're networked anyway so might as well make them available to read
    methods.registerMethod("statusProperty", [&](EntityPtr entity, String name, Json const& def = Json()) -> Maybe<Json> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->statusProperty(name, def);
        return {};
    });
    methods.registerMethod("stat", [&](EntityPtr entity, String name) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->stat(name);
        return {};
    });
    methods.registerMethod("statPositive", [&](EntityPtr entity, String name) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->statPositive(name);
        return {};
    });
    methods.registerMethod("resourceNames", [&](EntityPtr entity) -> Maybe<StringList> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourceNames();
        return {};
    });
    methods.registerMethod("resource", [&](EntityPtr entity, String name) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resource(name);
        return {};
    });
    methods.registerMethod("isResource", [&](EntityPtr entity, String name) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->isResource(name);
        return {};
    });
    methods.registerMethod("resourcePositive", [&](EntityPtr entity, String name) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourcePositive(name);
        return {};
    });
    methods.registerMethod("resourceLocked", [&](EntityPtr entity, String name) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourceLocked(name);
        return {};
    });
    methods.registerMethod("resourceMax", [&](EntityPtr entity, String name) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourceMax(name);
        return {};
    });
    methods.registerMethod("resourcePercentage", [&](EntityPtr entity, String name) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourcePercentage(name);
        return {};
    });
    methods.registerMethod("getPersistentEffects", [&](EntityPtr entity, String name) -> Maybe<JsonArray> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->getPersistentEffects(name).transformed(jsonFromPersistentStatusEffect);
        return {};
    });
    methods.registerMethod("activeUniqueStatusEffectSummary", [&](EntityPtr entity) -> Maybe<List<JsonArray>> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->activeUniqueStatusEffectSummary().transformed([](pair<UniqueStatusEffect, Maybe<float>> effect) {
            JsonArray effectJson = {effect.first};
            if (effect.second)
                effectJson.append(*effect.second);
            return effectJson;
            });;
        return {};
    });
    methods.registerMethod("uniqueStatusEffectActive", [&](EntityPtr entity, String name) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->uniqueStatusEffectActive(name);
        return {};
    });

    // movement controller methods, they're networked anyway so might as well make them available to read

    methods.registerMethod("mass", [&](EntityPtr entity) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->mass();
        return {};
    });
    methods.registerMethod("boundBox", [&](EntityPtr entity) -> Maybe<RectF> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->collisionPoly().boundBox();
        return {};
    });
    methods.registerMethod("collisionPoly", [&](EntityPtr entity) -> Maybe<PolyF> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->collisionPoly();
        return {};
    });
    methods.registerMethod("collisionBody", [&](EntityPtr entity) -> Maybe<PolyF> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->collisionBody();
        return {};
    });
    methods.registerMethod("collisionBoundBox", [&](EntityPtr entity) -> Maybe<RectF> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->collisionBody().boundBox();
        return {};
    });
    methods.registerMethod("localBoundBox", [&](EntityPtr entity) -> Maybe<RectF> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->localBoundBox();
        return {};
    });
    methods.registerMethod("rotation", [&](EntityPtr entity) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->rotation();
        return {};
    });
    methods.registerMethod("isColliding", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->isColliding();
        return {};
    });
    methods.registerMethod("isNullColliding", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->isNullColliding();
        return {};
    });
    methods.registerMethod("isCollisionStuck", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->isCollisionStuck();
        return {};
    });
    methods.registerMethod("stickingDirection", [&](EntityPtr entity) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->stickingDirection();
        return {};
    });
    methods.registerMethod("liquidPercentage", [&](EntityPtr entity) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->liquidPercentage();
        return {};
    });
    methods.registerMethod("liquidId", [&](EntityPtr entity) -> Maybe<float> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->liquidId();
        return {};
    });
    methods.registerMethod("onGround", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->onGround();
        return {};
    });
    methods.registerMethod("zeroG", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->zeroG();
        return {};
    });
    methods.registerMethod("atWorldLimit", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->atWorldLimit();
        return {};
    });
    methods.registerMethod("anchorState", [&](EntityPtr entity) -> LuaVariadic<LuaValue> {
        if (auto actor = as<ActorEntity>(entity))
            if (auto anchorState = actor->movementController()->anchorState())
            return LuaVariadic<LuaValue>{LuaInt(anchorState->entityId), LuaInt(anchorState->positionIndex)};
        return LuaVariadic<LuaValue>();
    });
    // slightly inconsistent for the sake of being more clear what the function is
    methods.registerMethod("baseMovementParameters", [&](EntityPtr entity) -> Maybe<Json> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->baseParameters().toJson();
        return {};
    });
    // slightly inconsistent for the sake of being more clear what the function is
    methods.registerMethod("movementParameters", [&](EntityPtr entity) -> Maybe<Json> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->parameters().toJson();
        return {};
    });

    methods.registerMethod("walking", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->walking();
        return {};
    });
    methods.registerMethod("running", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->running();
        return {};
    });
    methods.registerMethod("movingDirection", [&](EntityPtr entity) -> Maybe<int> {
        if (auto actor = as<ActorEntity>(entity))
            return numericalDirection(actor->movementController()->movingDirection());
        return {};
    });
    methods.registerMethod("facingDirection", [&](EntityPtr entity) -> Maybe<int> {
        if (auto actor = as<ActorEntity>(entity))
            return numericalDirection(actor->movementController()->facingDirection());
        return {};
    });
    methods.registerMethod("crouching", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->crouching();
        return {};
    });
    methods.registerMethod("flying", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->flying();
        return {};
    });
    methods.registerMethod("falling", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->falling();
        return {};
    });
    methods.registerMethod("canJump", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->canJump();
        return {};
    });
    methods.registerMethod("jumping", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->jumping();
        return {};
    });
    methods.registerMethod("groundMovement", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->groundMovement();
        return {};
    });
    methods.registerMethod("liquidMovement", [&](EntityPtr entity) -> Maybe<bool> {
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->liquidMovement();
        return {};
    }); 

    // tool user entity methods
    methods.registerMethod("handItem",
    [&](EntityPtr const& entity, String const& handName) -> Maybe<String> {
        ToolHand toolHand;
        if (handName == "primary") {
            toolHand = ToolHand::Primary;
        } else if (handName == "alt") {
            toolHand = ToolHand::Alt;
        } else {
            throw StarException(strf("Unknown tool hand {}", handName));
        }

        if (auto toolUser = as<ToolUserEntity>(entity)) {
            if (auto item = toolUser->handItem(toolHand)) {
                return item->name();
            }
        }

        return {};
    });

    methods.registerMethod("handItemDescriptor",
    [&](EntityPtr const& entity, String const& handName) -> Json {
        ToolHand toolHand;
        if (handName == "primary") {
            toolHand = ToolHand::Primary;
        } else if (handName == "alt") {
            toolHand = ToolHand::Alt;
        } else {
            throw StarException(strf("Unknown tool hand {}", handName));
        }

        if (auto toolUser = as<ToolUserEntity>(entity)) {
            if (auto item = toolUser->handItem(toolHand)) {
                return item->descriptor().toJson();
            }
        }

        return Json();
    });

    methods.registerMethod("aimPosition",
    [&](EntityPtr const& entity) -> Maybe<Vec2F> {
        if (auto toolUser = as<ToolUserEntity>(entity))
            return toolUser->aimPosition();
        return {};
    });


    // humanoid entity methods
    methods.registerMethod("species",
    [&](EntityPtr const& entity) -> Maybe<String> {
        if (auto player = as<Player>(entity)) {
            return player->species();
        } else if (auto npc = as<Npc>(entity)) {
            return npc->species();
        } else {
            return {};
        }
    });

    methods.registerMethod("gender",
    [&](EntityPtr const& entity) -> Maybe<String> {
        if (auto player = as<Player>(entity)) {
            return GenderNames.getRight(player->gender());
        } else if (auto npc = as<Npc>(entity)) {
            return GenderNames.getRight(npc->gender());
        } else {
            return {};
        }
    });

    // player methods
    methods.registerMethod("currency",
    [&](EntityPtr const& entity, String const& currencyType) -> Maybe<uint64_t> {
        if (auto player = as<Player>(entity)) {
            return player->currency(currencyType);
        }
        return {};
    });

    methods.registerMethod("hasCountOfItem",
    [&](EntityPtr const& entity, Json descriptor, Maybe<bool> exactMatch) -> Maybe<uint64_t> {
        if (auto player = as<Player>(entity)) {
            return player->inventory()->hasCountOfItem(ItemDescriptor(descriptor), exactMatch.value(false));
        }
        return {};
    });

    // loungeable entity methods
    methods.registerMethod("loungingEntities",
    [&](EntityPtr const& entity, Maybe<size_t> anchorIndex) -> Maybe<List<EntityId>> {
        if (!entity->inWorld())
            return {};
        if (auto loungeable = as<LoungeableEntity>(entity))
            return loungeable->entitiesLoungingIn(anchorIndex.value()).values();
        return {};
    });

    methods.registerMethod("loungeableOccupied",
    [&](EntityPtr const& entity, Maybe<size_t> anchorIndex) -> Maybe<bool> {
        if (!entity->inWorld())
            return {};
        auto loungeable = as<LoungeableEntity>(entity);
        size_t anchor = anchorIndex.value();
        if (loungeable && loungeable->anchorCount() > anchor)
            return !loungeable->entitiesLoungingIn(anchor).empty();
        return {};
    });

    methods.registerMethod("loungeableAnchorCount",
    [&](EntityPtr const& entity) -> Maybe<size_t> {
        if (!entity->inWorld())
            return {};
        if (auto loungeable = as<LoungeableEntity>(entity))
            return loungeable->anchorCount();
        return {};
    });

    // object methods
    methods.registerMethod("objectSpaces",
    [&](EntityPtr const& entity) -> List<Vec2I> {
        if (auto tileEntity = as<TileEntity>(entity))
            return tileEntity->spaces();
        return {};
    });

    // farmables
    methods.registerMethod("farmableStage",
    [&](EntityPtr const& entity) -> Maybe<int> {
        if (auto farmable = as<FarmableObject>(entity)) {
            return farmable->stage();
        }

        return {};
    });

    // containers
    methods.registerMethod("containerSize",
    [&](EntityPtr const& entity) -> Maybe<int> {
        if (auto container = as<ContainerObject>(entity))
            return container->containerSize();

        return {};
    });

    methods.registerMethod("containerClose",
    [&](EntityPtr const& entity) -> bool {
        if (auto container = as<ContainerObject>(entity)) {
            container->containerClose();
            return true;
        }

        return false;
    });

    methods.registerMethod("containerOpen",
    [&](EntityPtr const& entity) -> bool {
        if (auto container = as<ContainerObject>(entity)) {
            container->containerOpen();
            return true;
        }

        return false;
    });

    methods.registerMethod("containerItems",
    [&](EntityPtr const& entity) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            JsonArray res;
            auto itemDb = Root::singleton().itemDatabase();
            for (auto const& item : container->itemBag()->items())
                res.append(itemDb->toJson(item));
            return res;
        }

        return Json();
    });

    methods.registerMethod("containerItemAt",
    [&](EntityPtr const& entity, size_t offset) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto items = container->itemBag()->items();
            if (offset < items.size()) {
                return itemDb->toJson(items.at(offset));
            }
        }

        return Json();
    });

    methods.registerMethod("containerConsume",
    [&](EntityPtr const& entity, Json const& items) -> Maybe<bool> {
        if (auto container = as<ContainerObject>(entity)) {
            auto toConsume = ItemDescriptor(items);
            return container->consumeItems(toConsume).result();
        }

        return {};
    });

    methods.registerMethod("containerConsumeAt",
    [&](EntityPtr const& entity, size_t offset, int count) -> Maybe<bool> {
        if (auto container = as<ContainerObject>(entity)) {
            if (offset < container->containerSize()) {
                return container->consumeItems(offset, count).result();
            }
        }

        return {};
    });

    methods.registerMethod("containerAvailable",
    [&](EntityPtr const& entity, Json const& items) -> Maybe<size_t> {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemBag = container->itemBag();
            auto toCheck = ItemDescriptor(items);
            return itemBag->available(toCheck);
        }

        return {};
    });

    methods.registerMethod("containerTakeAll",
    [&](EntityPtr const& entity) -> Json {
        auto itemDb = Root::singleton().itemDatabase();
        if (auto container = as<ContainerObject>(entity)) {
            if (auto itemList = container->clearContainer().result()) {
                JsonArray res;
                for (auto item : *itemList)
                    res.append(itemDb->toJson(item));
                return res;
            }
        }

        return Json();
    });

    methods.registerMethod("containerTakeAt",
    [&](EntityPtr const& entity, size_t offset) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            if (offset < container->containerSize()) {
                if (auto res = container->takeItems(offset).result())
                    return itemDb->toJson(*res);
            }
        }

        return Json();
    });

    methods.registerMethod("containerTakeNumItemsAt",
    [&](EntityPtr const& entity, size_t offset, int const& count) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            if (offset < container->containerSize()) {
                if (auto res = container->takeItems(offset, count).result())
                    return itemDb->toJson(*res);
            }
        }

        return Json();
    });

    methods.registerMethod("containerItemsCanFit",
    [&](EntityPtr const& entity, Json const& items) -> Maybe<size_t> {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto itemBag = container->itemBag();
            auto toSearch = itemDb->fromJson(items);
            return itemBag->itemsCanFit(toSearch);
        }

        return {};
    });

    methods.registerMethod("containerItemsFitWhere",
    [&](EntityPtr const& entity, Json const& items) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto itemBag = container->itemBag();
            auto toSearch = itemDb->fromJson(items);
            auto res = itemBag->itemsFitWhere(toSearch);
            return JsonObject{
                {"leftover", res.leftover},
                {"slots", jsonFromList<size_t>(res.slots)}
            };
        }

        return Json();
    });

    methods.registerMethod("containerAddItems",
    [&](EntityPtr const& entity, Json const& items) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto toInsert = itemDb->fromJson(items);
            if (auto res = container->addItems(toInsert).result())
                return itemDb->toJson(*res);
        }

        return items;
    });

    methods.registerMethod("containerStackItems",
    [&](EntityPtr const& entity, Json const& items) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto toInsert = itemDb->fromJson(items);
            if (auto res = container->addItems(toInsert).result())
                return itemDb->toJson(*res);
        }

        return items;
    });

    methods.registerMethod("containerPutItemsAt",
    [&](EntityPtr const& entity, Json const& items, size_t offset) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto toInsert = itemDb->fromJson(items);
            if (offset < container->containerSize()) {
                if (auto res = container->putItems(offset, toInsert).result())
                    return itemDb->toJson(*res);
            }
        }

        return items;
    });

    methods.registerMethod("containerSwapItems",
    [&](EntityPtr const& entity, Json const& items, size_t offset, bool noCombine) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto toSwap = itemDb->fromJson(items);
            if (offset < container->containerSize()) {
                if (auto res = container->swapItems(offset, toSwap, !noCombine).result())
                    return itemDb->toJson(*res);
            }
        }

        return items;
    });

    methods.registerMethod("containerItemApply",
    [&](EntityPtr const& entity, Json const& items, size_t offset) -> Json {
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto toSwap = itemDb->fromJson(items);
            if (offset < container->containerSize()) {
                if (auto res = container->swapItems(offset, toSwap, false).result())
                    return itemDb->toJson(*res);
            }
        }

        return items;
    });

    return methods;
} 

}
