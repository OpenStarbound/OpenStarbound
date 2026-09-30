#include "StarLuaEntityContext.hpp"
#include "StarStatusController.hpp"
#include "StarActorMovementController.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorld.hpp"
#include "StarPhysicsEntity.hpp"
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

LuaMethods<EntityContext> LuaUserDataMethods<EntityContext>::make() {
    LuaMethods<EntityContext> methods;

    // general entity methods
    methods.registerMethod("exists",
    [&](EntityContext const& context) -> bool {
        auto const& entity = context.entity;
        return entity->inWorld();
    });

    methods.registerMethod("id",
    [&](EntityContext const& context) -> EntityId {
        auto const& entity = context.entity;
        return entity->entityId();
    });

    methods.registerMethod("canDamage",
    [&](EntityContext const& context, EntityId const& otherId) -> bool {
        auto const& entity = context.entity;
        if (entity->inWorld()) {
            auto other = entity->world()->entity(otherId);
            
            if (!other || !entity->getTeam().canDamage(other->getTeam(), false))
            return false;
            
            return true;
        }
        return false;
    });

    methods.registerMethod("damageTeam",
    [&](EntityContext const& context) -> Json {
        auto const& entity = context.entity;
        return entity->getTeam().toJson();
    });

    methods.registerMethod("aggressive",
    [&](EntityContext const& context) -> Json {
        auto const& entity = context.entity;
        if (auto monster = as<Monster>(entity))
            return monster->aggressive();
        if (auto npc = as<Npc>(entity))
            return npc->aggressive();
        return false;
    });

    methods.registerMethod("type",
    [&](EntityContext const& context, LuaEngine& engine) -> LuaString {
        auto const& entity = context.entity;
        return engine.createString(EntityTypeNames.getRight(entity->entityType()));
    });

    methods.registerMethod("typeName",
    [&](EntityContext const& context, LuaEngine& engine) -> Maybe<String> {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context) -> Vec2F {
        auto const& entity = context.entity;
        return entity->position();
    });

    methods.registerMethod("metaBoundBox",
    [&](EntityContext const& context) -> RectF {
        auto const& entity = context.entity;
        return entity->metaBoundBox();
    });

    methods.registerMethod("velocity",
    [&](EntityContext const& context) -> Maybe<Vec2F> {
        auto const& entity = context.entity;
        if (auto mobileEntity = as<MobileEntity>(entity))
            return mobileEntity->movementController()->velocity();

        return {};
    });

    methods.registerMethod("name",
    [&](EntityContext const& context) -> String {
        auto const& entity = context.entity;
        return entity->name();
    });

    methods.registerMethod("description",
    [&](EntityContext const& context, Maybe<String> const& species) -> Maybe<String> {
        auto const& entity = context.entity;
        if (auto inspectableEntity = as<InspectableEntity>(entity)) {
            if (species)
            return inspectableEntity->inspectionDescription(*species);
        }

        return entity->description();
    });

    methods.registerMethod("uniqueId",
    [&](EntityContext const& context) -> LuaNullTermWrapper<Maybe<String>> {
        auto const& entity = context.entity;
        return entity->uniqueId();
    });

    methods.registerMethod("getParameter",
    [&](EntityContext const& context, String const& parameterName, Maybe<Json> const& defaultValue) -> Json {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context, String const& message, LuaVariadic<Json> args) -> RpcPromise<Json> {
        auto const& entity = context.entity;
        if (entity->inWorld()) {
            auto const& world = entity->world();
            if (!world->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Interact))
                if (world->connectionHasPermission(entity->originConnection(), WorldPermissionType::Interact))
                    return RpcPromise<Json>::createFailed("No interact permissions.");
            return entity->world()->sendEntityMessage(entity->entityId(), message, JsonArray::from(std::move(args)));
        }
        return RpcPromise<Json>::createFailed("Entity not in world");
    });

    // scripted entity methods
    methods.registerMethod("callScript",
    [&](EntityContext const& context, String const& function, LuaVariadic<LuaValue> const& args) -> Maybe<LuaValue> {
        auto const& entity = context.entity;
        auto scrEntity = as<ScriptedEntity>(entity);
        if (!scrEntity || !entity->inWorld())
            throw StarException::format("Entity {} does not exist", entity->entityId());
        if (!scrEntity->isMaster())
            return {};
            
        auto const& world = entity->world();
        if (!world->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Interact))
            if (world->connectionHasPermission(entity->originConnection(), WorldPermissionType::Interact))
                return {};
        return scrEntity->callScript(function, args);
    });

    // nametag entity methods
    methods.registerMethod("nametag",
    [&](EntityContext const& context) -> Maybe<Json> {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context, String const& portraitMode) -> LuaNullTermWrapper<Maybe<List<Drawable>>> {
        auto const& entity = context.entity;
        if (auto portraitEntity = as<PortraitEntity>(entity))
            return portraitEntity->portrait(PortraitModeNames.getLeft(portraitMode));

        return {};
    });


    // damage bar entity methods
    methods.registerMethod("health",
    [&](EntityContext const& context) -> Maybe<Vec2F> {
        auto const& entity = context.entity;
        if (auto dmgEntity = as<DamageBarEntity>(entity)) {
            return Vec2F(dmgEntity->health(), dmgEntity->maxHealth());
        }
        return {};
    });

    // interactive entity methods
    methods.registerMethod("isInteractive",
    [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto interactEntity = as<InteractiveEntity>(entity))
            return interactEntity->isInteractive();
        return {};
    });

    // chatty entity methods
    methods.registerMethod("mouthPosition",
    [&](EntityContext const& context) -> Maybe<Vec2F> {
        auto const& entity = context.entity;
        if (auto chatty = as<ChattyEntity>(entity))
            return chatty->mouthPosition();
        return {};
    });

    // actor entity methods

    // status controller methods, they're networked anyway so might as well make them available to read
    methods.registerMethod("statusProperty", [&](EntityContext const& context, String name, Json const& def = Json()) -> Maybe<Json> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->statusProperty(name, def);
        return {};
    });
    methods.registerMethod("stat", [&](EntityContext const& context, String name) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->stat(name);
        return {};
    });
    methods.registerMethod("statPositive", [&](EntityContext const& context, String name) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->statPositive(name);
        return {};
    });
    methods.registerMethod("resourceNames", [&](EntityContext const& context) -> Maybe<StringList> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourceNames();
        return {};
    });
    methods.registerMethod("resource", [&](EntityContext const& context, String name) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resource(name);
        return {};
    });
    methods.registerMethod("isResource", [&](EntityContext const& context, String name) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->isResource(name);
        return {};
    });
    methods.registerMethod("resourcePositive", [&](EntityContext const& context, String name) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourcePositive(name);
        return {};
    });
    methods.registerMethod("resourceLocked", [&](EntityContext const& context, String name) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourceLocked(name);
        return {};
    });
    methods.registerMethod("resourceMax", [&](EntityContext const& context, String name) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourceMax(name);
        return {};
    });
    methods.registerMethod("resourcePercentage", [&](EntityContext const& context, String name) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->resourcePercentage(name);
        return {};
    });
    methods.registerMethod("getPersistentEffects", [&](EntityContext const& context, String name) -> Maybe<JsonArray> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->getPersistentEffects(name).transformed(jsonFromPersistentStatusEffect);
        return {};
    });
    methods.registerMethod("activeUniqueStatusEffectSummary", [&](EntityContext const& context) -> Maybe<List<JsonArray>> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->activeUniqueStatusEffectSummary().transformed([](pair<UniqueStatusEffect, Maybe<float>> effect) {
            JsonArray effectJson = {effect.first};
            if (effect.second)
                effectJson.append(*effect.second);
            return effectJson;
            });;
        return {};
    });
    methods.registerMethod("uniqueStatusEffectActive", [&](EntityContext const& context, String name) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->statusController()->uniqueStatusEffectActive(name);
        return {};
    });

    // movement controller methods, they're networked anyway so might as well make them available to read

    methods.registerMethod("mass", [&](EntityContext const& context) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->mass();
        return {};
    });
    methods.registerMethod("boundBox", [&](EntityContext const& context) -> Maybe<RectF> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->collisionPoly().boundBox();
        return {};
    });
    methods.registerMethod("collisionPoly", [&](EntityContext const& context) -> Maybe<PolyF> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->collisionPoly();
        return {};
    });
    methods.registerMethod("collisionBody", [&](EntityContext const& context) -> Maybe<PolyF> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->collisionBody();
        return {};
    });
    methods.registerMethod("collisionBoundBox", [&](EntityContext const& context) -> Maybe<RectF> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->collisionBody().boundBox();
        return {};
    });
    methods.registerMethod("localBoundBox", [&](EntityContext const& context) -> Maybe<RectF> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->localBoundBox();
        return {};
    });
    methods.registerMethod("rotation", [&](EntityContext const& context) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->rotation();
        return {};
    });
    methods.registerMethod("isColliding", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->isColliding();
        return {};
    });
    methods.registerMethod("isNullColliding", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->isNullColliding();
        return {};
    });
    methods.registerMethod("isCollisionStuck", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->isCollisionStuck();
        return {};
    });
    methods.registerMethod("stickingDirection", [&](EntityContext const& context) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->stickingDirection();
        return {};
    });
    methods.registerMethod("liquidPercentage", [&](EntityContext const& context) -> Maybe<float> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->liquidPercentage();
        return {};
    });
    methods.registerMethod("liquidId", [&](EntityContext const& context) -> Maybe<uint8_t> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->liquidId();
        return {};
    });
    methods.registerMethod("onGround", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->onGround();
        return {};
    });
    methods.registerMethod("zeroG", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->zeroG();
        return {};
    });
    methods.registerMethod("atWorldLimit", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->atWorldLimit();
        return {};
    });
    methods.registerMethod("anchorState", [&](EntityContext const& context) -> LuaVariadic<LuaValue> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            if (auto anchorState = actor->movementController()->anchorState())
            return LuaVariadic<LuaValue>{LuaInt(anchorState->entityId), LuaInt(anchorState->positionIndex)};
        return LuaVariadic<LuaValue>();
    });
    // slightly inconsistent for the sake of being more clear what the function is
    methods.registerMethod("baseMovementParameters", [&](EntityContext const& context) -> Maybe<Json> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->baseParameters().toJson();
        return {};
    });
    // slightly inconsistent for the sake of being more clear what the function is
    methods.registerMethod("movementParameters", [&](EntityContext const& context) -> Maybe<Json> {
        auto const& entity = context.entity;
        if (auto mobile = as<MobileEntity>(entity))
            return mobile->movementController()->parameters().toJson();
        return {};
    });

    methods.registerMethod("walking", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->walking();
        return {};
    });
    methods.registerMethod("running", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->running();
        return {};
    });
    methods.registerMethod("movingDirection", [&](EntityContext const& context) -> Maybe<int> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return numericalDirection(actor->movementController()->movingDirection());
        return {};
    });
    methods.registerMethod("facingDirection", [&](EntityContext const& context) -> Maybe<int> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return numericalDirection(actor->movementController()->facingDirection());
        return {};
    });
    methods.registerMethod("crouching", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->crouching();
        return {};
    });
    methods.registerMethod("flying", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->flying();
        return {};
    });
    methods.registerMethod("falling", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->falling();
        return {};
    });
    methods.registerMethod("canJump", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->canJump();
        return {};
    });
    methods.registerMethod("jumping", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->jumping();
        return {};
    });
    methods.registerMethod("groundMovement", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->groundMovement();
        return {};
    });
    methods.registerMethod("liquidMovement", [&](EntityContext const& context) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (auto actor = as<ActorEntity>(entity))
            return actor->movementController()->liquidMovement();
        return {};
    }); 

    // tool user entity methods
    methods.registerMethod("handItem",
    [&](EntityContext const& context, String const& handName) -> Maybe<String> {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context, String const& handName) -> Json {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context) -> Maybe<Vec2F> {
        auto const& entity = context.entity;
        if (auto toolUser = as<ToolUserEntity>(entity))
            return toolUser->aimPosition();
        return {};
    });


    // humanoid entity methods
    methods.registerMethod("species",
    [&](EntityContext const& context) -> Maybe<String> {
        auto const& entity = context.entity;
        if (auto player = as<Player>(entity)) {
            return player->species();
        } else if (auto npc = as<Npc>(entity)) {
            return npc->species();
        } else {
            return {};
        }
    });

    methods.registerMethod("gender",
    [&](EntityContext const& context) -> Maybe<String> {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context, String const& currencyType) -> Maybe<uint64_t> {
        auto const& entity = context.entity;
        if (auto player = as<Player>(entity)) {
            return player->currency(currencyType);
        }
        return {};
    });

    methods.registerMethod("hasCountOfItem",
    [&](EntityContext const& context, Json descriptor, Maybe<bool> exactMatch) -> Maybe<uint64_t> {
        auto const& entity = context.entity;
        if (auto player = as<Player>(entity)) {
            return player->inventory()->hasCountOfItem(ItemDescriptor(descriptor), exactMatch.value(false));
        }
        return {};
    });

    // loungeable entity methods
    methods.registerMethod("loungingEntities",
    [&](EntityContext const& context, Maybe<size_t> anchorIndex) -> Maybe<List<EntityId>> {
        auto const& entity = context.entity;
        if (!entity->inWorld())
            return {};
        if (auto loungeable = as<LoungeableEntity>(entity))
            return loungeable->entitiesLoungingIn(anchorIndex.value()).values();
        return {};
    });

    methods.registerMethod("loungeableOccupied",
    [&](EntityContext const& context, Maybe<size_t> anchorIndex) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (!entity->inWorld())
            return {};
        auto loungeable = as<LoungeableEntity>(entity);
        size_t anchor = anchorIndex.value();
        if (loungeable && loungeable->anchorCount() > anchor)
            return !loungeable->entitiesLoungingIn(anchor).empty();
        return {};
    });

    methods.registerMethod("loungeableAnchorCount",
    [&](EntityContext const& context) -> Maybe<size_t> {
        auto const& entity = context.entity;
        if (!entity->inWorld())
            return {};
        if (auto loungeable = as<LoungeableEntity>(entity))
            return loungeable->anchorCount();
        return {};
    });

    // object methods
    methods.registerMethod("objectSpaces",
    [&](EntityContext const& context) -> List<Vec2I> {
        auto const& entity = context.entity;
        if (auto tileEntity = as<TileEntity>(entity))
            return tileEntity->spaces();
        return {};
    });

    // farmables
    methods.registerMethod("farmableStage",
    [&](EntityContext const& context) -> Maybe<int> {
        auto const& entity = context.entity;
        if (auto farmable = as<FarmableObject>(entity)) {
            return farmable->stage();
        }

        return {};
    });

    // containers
    methods.registerMethod("containerSize",
    [&](EntityContext const& context) -> Maybe<int> {
        auto const& entity = context.entity;
        if (auto container = as<ContainerObject>(entity))
            return container->containerSize();

        return {};
    });

    methods.registerMethod("containerClose",
    [&](EntityContext const& context) -> bool {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return false;
        
        if (auto container = as<ContainerObject>(entity)) {
            container->containerClose();
            return true;
        }

        return false;
    });

    methods.registerMethod("containerOpen",
    [&](EntityContext const& context) -> bool {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return false;
        
        if (auto container = as<ContainerObject>(entity)) {
            container->containerOpen();
            return true;
        }

        return false;
    });

    methods.registerMethod("containerItems",
    [&](EntityContext const& context) -> Json {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context, size_t offset) -> Json {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context, Json const& items) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return {};
        
        if (auto container = as<ContainerObject>(entity)) {
            auto toConsume = ItemDescriptor(items);
            return container->consumeItems(toConsume).result();
        }

        return {};
    });

    methods.registerMethod("containerConsumeAt",
    [&](EntityContext const& context, size_t offset, int count) -> Maybe<bool> {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return {};
        
        if (auto container = as<ContainerObject>(entity)) {
            if (offset < container->containerSize()) {
                return container->consumeItems(offset, count).result();
            }
        }

        return {};
    });

    methods.registerMethod("containerAvailable",
    [&](EntityContext const& context, Json const& items) -> Maybe<size_t> {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return {};
        
        if (auto container = as<ContainerObject>(entity)) {
            auto itemBag = container->itemBag();
            auto toCheck = ItemDescriptor(items);
            return itemBag->available(toCheck);
        }

        return {};
    });

    methods.registerMethod("containerTakeAll",
    [&](EntityContext const& context) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return Json();
        
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
    [&](EntityContext const& context, size_t offset) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return Json();
        
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
    [&](EntityContext const& context, size_t offset, int const& count) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return Json();
        
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
    [&](EntityContext const& context, Json const& items) -> Maybe<size_t> {
        auto const& entity = context.entity;
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto itemBag = container->itemBag();
            auto toSearch = itemDb->fromJson(items);
            return itemBag->itemsCanFit(toSearch);
        }

        return {};
    });

    methods.registerMethod("containerItemsFitWhere",
    [&](EntityContext const& context, Json const& items) -> Json {
        auto const& entity = context.entity;
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
    [&](EntityContext const& context, Json const& items) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return items;
        
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto toInsert = itemDb->fromJson(items);
            if (auto res = container->addItems(toInsert).result())
                return itemDb->toJson(*res);
        }

        return items;
    });

    methods.registerMethod("containerStackItems",
    [&](EntityContext const& context, Json const& items) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return items;
        
        if (auto container = as<ContainerObject>(entity)) {
            auto itemDb = Root::singleton().itemDatabase();
            auto toInsert = itemDb->fromJson(items);
            if (auto res = container->addItems(toInsert).result())
                return itemDb->toJson(*res);
        }

        return items;
    });

    methods.registerMethod("containerPutItemsAt",
    [&](EntityContext const& context, Json const& items, size_t offset) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return items;
        
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
    [&](EntityContext const& context, Json const& items, size_t offset, bool noCombine) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return items;
        
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
    [&](EntityContext const& context, Json const& items, size_t offset) -> Json {
        auto const& entity = context.entity;
        if (entity->inWorld() && !entity->world()->connectionHasPermission(callerConnection(context.caller), WorldPermissionType::Containers))
            return items;
        
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

    methods.registerMethod("movingCollisionCount",
    [&](EntityContext const& context) -> size_t {
        auto const& entity = context.entity;
        if (auto phys = as<PhysicsEntity>(entity)) {
            return phys->movingCollisionCount();
        }

        return 0;
    });

    methods.registerMethod("movingCollision",
    [&](EntityContext const& context, size_t index) -> Maybe<PhysicsMovingCollision> {
        auto const& entity = context.entity;
        if (auto phys = as<PhysicsEntity>(entity)) {
            return phys->movingCollision(index);
        }

        return {};
    });

    return methods;
} 

}
