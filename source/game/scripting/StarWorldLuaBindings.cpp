#include "StarWorldLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorld.hpp"
#include "StarBlocksAlongLine.hpp"
#include "StarSky.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarMonster.hpp"
#include "StarNpc.hpp"
#include "StarStagehand.hpp"
#include "StarLoungeableObject.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarProjectile.hpp"
#include "StarRoot.hpp"
#include "StarWorldServer.hpp"
#include "StarWorldClient.hpp"
#include "StarWorldTemplate.hpp"
#include "StarWorldParameters.hpp"
#include "StarItemDrop.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarLogging.hpp"
#include "StarObjectDatabase.hpp"
#include "StarItemDatabase.hpp"
#include "StarItem.hpp"
#include "StarTreasure.hpp"
#include "StarContainerObject.hpp"
#include "StarFarmableObject.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarVehicleDatabase.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarUniverseSettings.hpp"
#include "StarBiome.hpp"

namespace Star {
namespace LuaBindings {

  enum class EntityBoundMode { MetaBoundBox, CollisionArea, Position };

  EnumMap<EntityBoundMode> const EntityBoundModeNames = {
    {EntityBoundMode::MetaBoundBox, "MetaBoundBox"},
    {EntityBoundMode::CollisionArea, "CollisionArea"},
    {EntityBoundMode::Position, "Position"}
  };

  template <typename EntityT>
  using Selector = function<bool(shared_ptr<EntityT> const&)>;

  template <typename EntityT>
  LuaTable entityQueryImpl(World* world, LuaEngine& engine, LuaTable const& options, Selector<EntityT> selector) {
    Maybe<EntityId> withoutEntityId = options.get<Maybe<EntityId>>("withoutEntityId");
    Maybe<Set<EntityType>> includedTypes;
    if (auto types = options.get<Maybe<LuaTable>>("includedTypes")) {
      includedTypes = Set<EntityType>();
      types->iterate([&includedTypes](LuaValue const&, LuaString const& type) {
        if (type == "mobile") {
          includedTypes->add(EntityType::Player);
          includedTypes->add(EntityType::Monster);
          includedTypes->add(EntityType::Npc);
          includedTypes->add(EntityType::Projectile);
          includedTypes->add(EntityType::ItemDrop);
          includedTypes->add(EntityType::Vehicle);
        } else if (type == "creature") {
          includedTypes->add(EntityType::Player);
          includedTypes->add(EntityType::Monster);
          includedTypes->add(EntityType::Npc);
        } else {
          includedTypes->add(EntityTypeNames.getLeft(type.ptr()));
        }
      });
    }

    Maybe<String> callScript = options.get<Maybe<String>>("callScript");
    List<LuaValue> callScriptArgs = options.get<Maybe<List<LuaValue>>>("callScriptArgs").value();
    LuaValue callScriptResult = options.get<Maybe<LuaValue>>("callScriptResult").value(LuaBoolean(true));

    Maybe<Line2F> lineQuery = options.get<Maybe<Line2F>>("line");
    Maybe<PolyF> polyQuery = options.get<Maybe<PolyF>>("poly");
    Maybe<RectF> rectQuery = options.get<Maybe<RectF>>("rect");
    Maybe<pair<Vec2F, float>> radiusQuery;
    if (auto radius = options.get<Maybe<float>>("radius"))
      radiusQuery = make_pair(options.get<Vec2F>("center"), *radius);

    EntityBoundMode boundMode = EntityBoundModeNames.getLeft(options.get<Maybe<String>>("boundMode").value("CollisionArea"));
    Maybe<LuaString> order = options.get<Maybe<LuaString>>("order");

    auto geometry = world->geometry();

    auto innerSelector = [=](shared_ptr<EntityT> const& entity) -> bool {
      if (selector && !selector(entity))
        return false;

      if (includedTypes && !includedTypes->contains(entity->entityType()))
        return false;

      if (withoutEntityId && entity->entityId() == *withoutEntityId)
        return false;

      if (callScript) {
        auto scriptedEntity = as<ScriptedEntity>(entity);
        if (!scriptedEntity || !scriptedEntity->isMaster())
          return false;

        auto res = scriptedEntity->callScript(*callScript, luaUnpack(callScriptArgs));
        if (!res || *res != callScriptResult)
          return false;
      }

      auto position = entity->position();
      if (boundMode == EntityBoundMode::MetaBoundBox) {
        // If using MetaBoundBox, the regular line / box query methods already
        // enforce collision with MetaBoundBox
        if (radiusQuery)
          return geometry.rectIntersectsCircle(
              entity->metaBoundBox().translated(position), radiusQuery->first, radiusQuery->second);
      } else if (boundMode == EntityBoundMode::CollisionArea) {
        // Collision area queries either query based on the collision area if
        // that's given, or as a fallback the regular bound box.
        auto collisionArea = entity->collisionArea();
        if (collisionArea.isNull())
          collisionArea = entity->metaBoundBox();
        collisionArea.translate(position);

        if (lineQuery)
          return geometry.lineIntersectsRect(*lineQuery, collisionArea);
        if (polyQuery)
          return geometry.polyIntersectsPoly(*polyQuery, PolyF(collisionArea));
        if (rectQuery)
          return geometry.rectIntersectsRect(*rectQuery, collisionArea);
        if (radiusQuery)
          return geometry.rectIntersectsCircle(collisionArea, radiusQuery->first, radiusQuery->second);
      } else if (boundMode == EntityBoundMode::Position) {
        if (lineQuery)
          return geometry.lineIntersectsRect(*lineQuery, RectF(position, position));
        if (polyQuery)
          return geometry.polyContains(*polyQuery, position);
        if (rectQuery)
          return geometry.rectContains(*rectQuery, position);
        if (radiusQuery)
          return geometry.diff(radiusQuery->first, position).magnitude() <= radiusQuery->second;
      }

      return true;
    };

    List<shared_ptr<EntityT>> entities;

    if (lineQuery) {
      entities = world->lineQuery<EntityT>(lineQuery->min(), lineQuery->max(), innerSelector);
    } else if (polyQuery) {
      entities = world->query<EntityT>(polyQuery->boundBox(), innerSelector);
    } else if (rectQuery) {
      entities = world->query<EntityT>(*rectQuery, innerSelector);
    } else if (radiusQuery) {
      RectF region(radiusQuery->first - Vec2F::filled(radiusQuery->second),
          radiusQuery->first + Vec2F::filled(radiusQuery->second));
      entities = world->query<EntityT>(region, innerSelector);
    }

    if (order) {
      if (*order == "nearest") {
        Vec2F nearestPosition;
        if (lineQuery)
          nearestPosition = lineQuery->min();
        else if (polyQuery)
          nearestPosition = polyQuery->center();
        else if (rectQuery)
          nearestPosition = rectQuery->center();
        else if (radiusQuery)
          nearestPosition = radiusQuery->first;
        sortByComputedValue(entities,
            [world, nearestPosition](shared_ptr<EntityT> const& entity) {
              return world->geometry().diff(entity->position(), nearestPosition).magnitude();
            });
      } else if (*order == "random") {
        Random::shuffle(entities);
      } else {
        throw StarException(strf("Unsupported query order {}", order->ptr()));
      }
    }

    LuaTable entityIds = engine.createTable();
    int entityIdsIndex = 1;
    for (auto entity : entities)
      entityIds.set(entityIdsIndex++, entity->entityId());

    return entityIds;
  }

  template <typename EntityT>
  LuaTable entityQuery(World* world,
      LuaEngine& engine,
      Vec2F const& pos1,
      LuaValue const& pos2,
      Maybe<LuaTable> options,
      Selector<EntityT> selector = {}) {
    if (!options)
      options = engine.createTable();

    if (auto radius = engine.luaMaybeTo<float>(pos2)) {
      Vec2F center = pos1;
      options->set("center", center);
      options->set("radius", *radius);
      return entityQueryImpl<EntityT>(world, engine, *options, selector);
    } else {
      RectF rect(pos1, engine.luaTo<Vec2F>(pos2));
      options->set("rect", rect);
      return entityQueryImpl<EntityT>(world, engine, *options, selector);
    }
  }

  template <typename EntityT>
  LuaTable entityLineQuery(World* world,
      LuaEngine& engine,
      Vec2F const& point1,
      Vec2F const& point2,
      Maybe<LuaTable> options,
      Selector<EntityT> selector = {}) {
    Line2F line(point1, point2);

    if (!options)
      options = engine.createTable();

    options->set("line", line);

    return entityQueryImpl<EntityT>(world, engine, *options, selector);
  }

  LuaCallbacks makeWorldCallbacks(World* world) {
    LuaCallbacks callbacks;

    addWorldDebugCallbacks(callbacks);
    addWorldEnvironmentCallbacks(callbacks, world);
    addWorldEntityCallbacks(callbacks, world);

    callbacks.registerCallbackWithSignature<float, Vec2F, Maybe<Vec2F>>("magnitude", bind(&WorldCallbacks::magnitude, world, _1, _2));
    callbacks.registerCallbackWithSignature<Vec2F, Vec2F, Vec2F>("distance", bind(WorldCallbacks::distance, world, _1, _2));
    callbacks.registerCallbackWithSignature<bool, PolyF, Vec2F>("polyContains", bind(WorldCallbacks::polyContains, world, _1, _2));
    callbacks.registerCallbackWithSignature<LuaValue, LuaEngine&, LuaValue>("xwrap", bind(WorldCallbacks::xwrap, world, _1, _2));
    callbacks.registerCallbackWithSignature<LuaValue, LuaEngine&, Variant<Vec2F, float>, Variant<Vec2F, float>>("nearestTo", bind(WorldCallbacks::nearestTo, world, _1, _2, _3));

    callbacks.registerCallbackWithSignature<bool, RectF, Maybe<CollisionSet>>("rectCollision", bind(WorldCallbacks::rectCollision, world, _1, _2));
    callbacks.registerCallbackWithSignature<bool, Vec2F, Maybe<CollisionSet>>("pointTileCollision", bind(WorldCallbacks::pointTileCollision, world, _1, _2));
    callbacks.registerCallbackWithSignature<bool, Vec2F, Vec2F, Maybe<CollisionSet>>("lineTileCollision", bind(WorldCallbacks::lineTileCollision, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<pair<Vec2F, Vec2I>>, Vec2F, Vec2F, Maybe<CollisionSet>>("lineTileCollisionPoint", bind(WorldCallbacks::lineTileCollisionPoint, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<bool, RectF, Maybe<CollisionSet>>("rectTileCollision", bind(WorldCallbacks::rectTileCollision, world, _1, _2));
    callbacks.registerCallbackWithSignature<bool, Vec2F, Maybe<CollisionSet>>("pointCollision", bind(WorldCallbacks::pointCollision, world, _1, _2));
    callbacks.registerCallbackWithSignature<LuaTupleReturn<Maybe<Vec2F>, Maybe<Vec2F>>, Vec2F, Vec2F, Maybe<CollisionSet>>("lineCollision", bind(WorldCallbacks::lineCollision, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<bool, PolyF, Maybe<Vec2F>, Maybe<CollisionSet>>("polyCollision", bind(WorldCallbacks::polyCollision, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<List<Vec2I>, Vec2F, Vec2F, Maybe<CollisionSet>, Maybe<int>>("collisionBlocksAlongLine", bind(WorldCallbacks::collisionBlocksAlongLine, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<List<pair<Vec2I, LiquidLevel>>, Vec2F, Vec2F>("liquidAlongLine", bind(WorldCallbacks::liquidAlongLine, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<Vec2F>, PolyF, Vec2F, float, Maybe<CollisionSet>>("resolvePolyCollision", bind(WorldCallbacks::resolvePolyCollision, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<bool, Vec2I, Maybe<bool>, Maybe<bool>>("tileIsOccupied", bind(WorldCallbacks::tileIsOccupied, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<bool, String, Vec2I, Maybe<int>, Json>("placeObject", bind(WorldCallbacks::placeObject, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<Maybe<EntityId>, Json, Vec2F, Maybe<size_t>, Json, Maybe<Vec2F>, Maybe<float>>("spawnItem", bind(WorldCallbacks::spawnItem, world, _1, _2, _3, _4, _5, _6));
    callbacks.registerCallbackWithSignature<List<EntityId>, Vec2F, String, float, Maybe<uint64_t>>("spawnTreasure", bind(WorldCallbacks::spawnTreasure, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<Maybe<EntityId>, String, Vec2F, Maybe<JsonObject>>("spawnMonster", bind(WorldCallbacks::spawnMonster, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<EntityId>, Vec2F, String, String, float, Maybe<uint64_t>, Json>("spawnNpc", bind(WorldCallbacks::spawnNpc, world, _1, _2, _3, _4, _5, _6));
    callbacks.registerCallbackWithSignature<Maybe<EntityId>, Vec2F, String, Json>("spawnStagehand", bind(WorldCallbacks::spawnStagehand, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<EntityId>, String, Vec2F, Maybe<EntityId>, Maybe<Vec2F>, bool, Json>("spawnProjectile", bind(WorldCallbacks::spawnProjectile, world, _1, _2, _3, _4, _5, _6));
    callbacks.registerCallbackWithSignature<Maybe<EntityId>, String, Vec2F, Json>("spawnVehicle", bind(WorldCallbacks::spawnVehicle, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<float>("threatLevel", bind(&World::threatLevel, world));
    callbacks.registerCallbackWithSignature<double>("time", bind(WorldCallbacks::time, world));
    callbacks.registerCallbackWithSignature<uint64_t>("day", bind(WorldCallbacks::day, world));
    callbacks.registerCallbackWithSignature<double>("timeOfDay", bind(WorldCallbacks::timeOfDay, world));
    callbacks.registerCallbackWithSignature<float>("dayLength", bind(WorldCallbacks::dayLength, world));
    callbacks.registerCallbackWithSignature<Json, String, Json>("getProperty", bind(WorldCallbacks::getProperty, world, _1, _2));
    callbacks.registerCallbackWithSignature<void, String, Json>("setProperty", bind(WorldCallbacks::setProperty, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<LiquidLevel>, Variant<RectF, Vec2I>>("liquidAt", bind(WorldCallbacks::liquidAt, world, _1));
    callbacks.registerCallbackWithSignature<float, Vec2F>("gravity", bind(WorldCallbacks::gravity, world, _1));
    callbacks.registerCallbackWithSignature<bool, Vec2F, LiquidId, float>("spawnLiquid", bind(WorldCallbacks::spawnLiquid, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<LiquidLevel>, Vec2F>("destroyLiquid", bind(WorldCallbacks::destroyLiquid, world, _1));
    callbacks.registerCallbackWithSignature<bool, Vec2F>("isTileProtected", bind(WorldCallbacks::isTileProtected, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<PlatformerAStar::Path>, Vec2F, Vec2F, ActorMovementParameters, PlatformerAStar::Parameters>("findPlatformerPath", bind(WorldCallbacks::findPlatformerPath, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<PlatformerAStar::PathFinder, Vec2F, Vec2F, ActorMovementParameters, PlatformerAStar::Parameters>("platformerPathStart", bind(WorldCallbacks::platformerPathStart, world, _1, _2, _3, _4));

    callbacks.registerCallback("type", [world](LuaEngine& engine) -> LuaString {
        if (auto serverWorld = as<WorldServer>(world)) {
          if (auto worldParameters = serverWorld->worldTemplate()->worldParameters())
            return engine.createString(worldParameters->typeName);
        } else if (auto clientWorld = as<WorldClient>(world)) {
          if (auto worldParameters = clientWorld->currentTemplate()->worldParameters())
            return engine.createString(worldParameters->typeName);
        }
        return engine.createString("unknown");
      });

    callbacks.registerCallback("size", [world]() -> Vec2I {
        if (auto serverWorld = as<WorldServer>(world))
          return (Vec2I)serverWorld->worldTemplate()->size();
        else if (auto clientWorld = as<WorldClient>(world))
          return (Vec2I)clientWorld->currentTemplate()->size();
        return Vec2I();
      });

    callbacks.registerCallback("inSurfaceLayer", [world](Vec2I const& position) -> bool {
        if (auto serverWorld = as<WorldServer>(world))
          return serverWorld->worldTemplate()->inSurfaceLayer(position);
        else if (auto clientWorld = as<WorldClient>(world))
          return clientWorld->currentTemplate()->inSurfaceLayer(position);
        return false;
      });

    callbacks.registerCallback("surfaceLevel", [world]() -> float {
        if (auto serverWorld = as<WorldServer>(world))
          return serverWorld->worldTemplate()->surfaceLevel();
        else if (auto clientWorld = as<WorldClient>(world))
          return clientWorld->currentTemplate()->surfaceLevel();
        else
          return world->geometry().size()[1] / 2.0f;
      });

    callbacks.registerCallback("terrestrial", [world]() -> bool {
        if (auto serverWorld = as<WorldServer>(world)) {
          if (auto worldParameters = serverWorld->worldTemplate()->worldParameters())
            return worldParameters->type() == WorldParametersType::TerrestrialWorldParameters;
        } else if (auto clientWorld = as<WorldClient>(world)) {
          if (auto worldParameters = clientWorld->currentTemplate()->worldParameters())
            return worldParameters->type() == WorldParametersType::TerrestrialWorldParameters;
        }
        return false;
      });

    callbacks.registerCallback("itemDropItem", [world](EntityId const& entityId) -> Json {
        if (auto itemDrop = world->get<ItemDrop>(entityId))
          return itemDrop->item()->descriptor().toJson();
        return {};
      });

    callbacks.registerCallback("biomeBlocksAt", [world](Vec2I position) -> Maybe<List<MaterialId>> {
        WorldTemplateConstPtr worldTemplate;
        if (auto worldClient = as<WorldClient>(world))
          worldTemplate = worldClient->currentTemplate();
        else if (auto worldServer = as<WorldServer>(world))
          worldTemplate = worldServer->worldTemplate();

        if (worldTemplate) {
          WorldTemplate::BlockInfo block = worldTemplate->blockInfo(position[0], position[1]);
          if (auto biome = worldTemplate->biome(block.blockBiomeIndex)) {
            List<MaterialId> blocks = {biome->mainBlock};
            blocks.appendAll(biome->subBlocks);
            return blocks;
          }
        }

        return {};
      });
      

    callbacks.registerCallback("dungeonId", [world](Vec2I position) -> DungeonId {
        if (auto serverWorld = as<WorldServer>(world)) {
          return serverWorld->dungeonId(position);
        } else {
          return as<WorldClient>(world)->dungeonId(position);
        }
      });

    if (auto clientWorld = as<WorldClient>(world)) {
      callbacks.registerCallback("isClient", []() { return true;  });
      callbacks.registerCallback("isServer", []() { return false; });
      callbacks.registerCallbackWithSignature<RectI>("clientWindow", bind(ClientWorldCallbacks::clientWindow, clientWorld));
      callbacks.registerCallback("players", [clientWorld]() {
        List<EntityId> playerIds;

        clientWorld->forAllEntities([&](EntityPtr const& entity) {
          if (entity->entityType() == EntityType::Player)
            playerIds.emplace_back(entity->entityId());
        });

        return playerIds;
      });
    }

    if (auto serverWorld = as<WorldServer>(world)) {
      callbacks.registerCallback("isClient", []() { return false; });
      callbacks.registerCallback("isServer", []() { return true;  });

      callbacks.registerCallbackWithSignature<bool, EntityId, bool>("breakObject", bind(ServerWorldCallbacks::breakObject, serverWorld, _1, _2));
      callbacks.registerCallbackWithSignature<bool, RectF>("isVisibleToPlayer", bind(ServerWorldCallbacks::isVisibleToPlayer, serverWorld, _1));
      callbacks.registerCallbackWithSignature<bool, RectF>("loadRegion", bind(ServerWorldCallbacks::loadRegion, serverWorld, _1));
      callbacks.registerCallbackWithSignature<bool, RectF>("regionActive", bind(ServerWorldCallbacks::regionActive, serverWorld, _1));
      callbacks.registerCallbackWithSignature<void, DungeonId, bool>("setTileProtection", bind(ServerWorldCallbacks::setTileProtection, serverWorld, _1, _2));
      callbacks.registerCallbackWithSignature<bool, RectI>("isPlayerModified", bind(ServerWorldCallbacks::isPlayerModified, serverWorld, _1));
      callbacks.registerCallbackWithSignature<Maybe<LiquidLevel>, Vec2F>("forceDestroyLiquid", bind(ServerWorldCallbacks::forceDestroyLiquid, serverWorld, _1));
      callbacks.registerCallbackWithSignature<EntityId, String>("loadUniqueEntity", bind(ServerWorldCallbacks::loadUniqueEntity, serverWorld, _1));
      callbacks.registerCallbackWithSignature<void, EntityId, String>("setUniqueId", bind(ServerWorldCallbacks::setUniqueId, serverWorld, _1, _2));
      callbacks.registerCallbackWithSignature<Json, EntityId, Maybe<EntityId>>("takeItemDrop", bind(ServerWorldCallbacks::takeItemDrop, world, _1, _2));
      callbacks.registerCallbackWithSignature<void, Vec2F, Maybe<bool>>("setPlayerStart", bind(ServerWorldCallbacks::setPlayerStart, world, _1, _2));
      callbacks.registerCallbackWithSignature<List<EntityId>>("players", bind(ServerWorldCallbacks::players, world));
      callbacks.registerCallbackWithSignature<LuaString, LuaEngine&>("fidelity", bind(ServerWorldCallbacks::fidelity, world, _1));
      callbacks.registerCallbackWithSignature<Maybe<LuaValue>, String, String, LuaVariadic<LuaValue>>("callScriptContext", bind(ServerWorldCallbacks::callScriptContext, world, _1, _2, _3));

      callbacks.registerCallbackWithSignature<double>("skyTime", [serverWorld]() {
          return serverWorld->sky()->epochTime();
        });
      callbacks.registerCallbackWithSignature<void, double>("setSkyTime", [serverWorld](double skyTime) {
          return serverWorld->sky()->setEpochTime(skyTime);
        });

      callbacks.registerCallback("setExpiryTime", [serverWorld](float expiryTime) { serverWorld->setExpiryTime(expiryTime); });

      callbacks.registerCallback("flyingType", [serverWorld]() -> String { return FlyingTypeNames.getRight(serverWorld->sky()->flyingType()); });
      callbacks.registerCallback("warpPhase", [serverWorld]() -> String { return WarpPhaseNames.getRight(serverWorld->sky()->warpPhase()); });
      callbacks.registerCallback("setUniverseFlag", [serverWorld](String flagName) { return serverWorld->universeSettings()->setFlag(flagName); });
      callbacks.registerCallback("universeFlags", [serverWorld]() { return serverWorld->universeSettings()->flags(); });
      callbacks.registerCallback("universeFlagSet", [serverWorld](String const& flagName) { return serverWorld->universeSettings()->flags().contains(flagName); });
      callbacks.registerCallback("placeDungeon", [serverWorld](String dungeonName, Vec2I position, Maybe<DungeonId> dungeonId) -> bool {
          return serverWorld->placeDungeon(dungeonName, position, dungeonId);
        });
      callbacks.registerCallback("tryPlaceDungeon", [serverWorld](String dungeonName, Vec2I position, Maybe<DungeonId> dungeonId) -> bool {
          return serverWorld->placeDungeon(dungeonName, position, dungeonId, false);
        });

      callbacks.registerCallback("addBiomeRegion", [serverWorld](Vec2I position, String biomeName, String subBlockSelector, int width) {
          serverWorld->addBiomeRegion(position, biomeName, subBlockSelector, width);
        });
      callbacks.registerCallback("expandBiomeRegion", [serverWorld](Vec2I position, int width) {
          serverWorld->expandBiomeRegion(position, width);
        });

      callbacks.registerCallback("pregenerateAddBiome", [serverWorld](Vec2I position, int width) {
          return serverWorld->pregenerateAddBiome(position, width);
        });
      callbacks.registerCallback("pregenerateExpandBiome", [serverWorld](Vec2I position, int width) {
          return serverWorld->pregenerateExpandBiome(position, width);
        });

      callbacks.registerCallback("setLayerEnvironmentBiome", [serverWorld](Vec2I position) {
          serverWorld->setLayerEnvironmentBiome(position);
        });

      callbacks.registerCallback("setPlanetType", [serverWorld](String planetType, String primaryBiomeName) {
          serverWorld->setPlanetType(planetType, primaryBiomeName);
        });

      callbacks.registerCallback("setDungeonGravity", [serverWorld](DungeonId dungeonId, Maybe<float> gravity) {
          serverWorld->setDungeonGravity(dungeonId, gravity);
        });

      callbacks.registerCallback("setDungeonBreathable", [serverWorld](DungeonId dungeonId, Maybe<bool> breathable) {
          serverWorld->setDungeonBreathable(dungeonId, breathable);
        });

      callbacks.registerCallback("setDungeonId", [serverWorld](RectI tileRegion, DungeonId dungeonId) {
          serverWorld->setDungeonId(tileRegion, dungeonId);
        });

      callbacks.registerCallback("enqueuePlacement", [serverWorld](List<Json> distributionConfigs, Maybe<DungeonId> id) {
          auto distributions = distributionConfigs.transformed([](Json const& config) {
            return BiomeItemDistribution(config, Random::randu64());
          });
          return serverWorld->enqueuePlacement(move(distributions), id);
        });
    }

    return callbacks;
  }

  void addWorldDebugCallbacks(LuaCallbacks& callbacks) {
    callbacks.registerCallback("debugPoint", WorldDebugCallbacks::debugPoint);
    callbacks.registerCallback("debugLine", WorldDebugCallbacks::debugLine);
    callbacks.registerCallback("debugPoly", WorldDebugCallbacks::debugPoly);
    callbacks.registerCallback("debugText", WorldDebugCallbacks::debugText);
  }

  void addWorldEntityCallbacks(LuaCallbacks& callbacks, World* world) {
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, LuaValue, Maybe<LuaTable>>("entityQuery", bind(WorldEntityCallbacks::entityQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, LuaValue, Maybe<LuaTable>>("monsterQuery", bind(WorldEntityCallbacks::monsterQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, LuaValue, Maybe<LuaTable>>("npcQuery", bind(WorldEntityCallbacks::npcQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, LuaValue, Maybe<LuaTable>>("objectQuery", bind(WorldEntityCallbacks::objectQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, LuaValue, Maybe<LuaTable>>("itemDropQuery", bind(WorldEntityCallbacks::itemDropQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, LuaValue, Maybe<LuaTable>>("playerQuery", bind(WorldEntityCallbacks::playerQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, LuaValue, Maybe<LuaTable>>("loungeableQuery", bind(WorldEntityCallbacks::loungeableQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, Vec2F, Maybe<LuaTable>>("entityLineQuery", bind(WorldEntityCallbacks::entityLineQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, Vec2F, Maybe<LuaTable>>("objectLineQuery", bind(WorldEntityCallbacks::objectLineQuery, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<LuaTable, LuaEngine&, Vec2F, Vec2F, Maybe<LuaTable>>("npcLineQuery", bind(WorldEntityCallbacks::npcLineQuery, world, _1, _2, _3, _4));
    callbacks.registerCallback("objectAt",
        [world](Vec2I const& tilePosition) -> Maybe<int> {
          if (auto object = world->findEntityAtTile(tilePosition, [](TileEntityPtr const& entity) { return is<Object>(entity); }))
            return object->entityId();
          else
            return {};
        });
    callbacks.registerCallbackWithSignature<bool, int>("entityExists", bind(WorldEntityCallbacks::entityExists, world, _1));
    callbacks.registerCallbackWithSignature<bool, int, int>("entityCanDamage", bind(WorldEntityCallbacks::entityCanDamage, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId>("entityDamageTeam", bind(WorldEntityCallbacks::entityDamageTeam, world, _1));
    callbacks.registerCallbackWithSignature<Json, EntityId>("entityAggressive", bind(WorldEntityCallbacks::entityAggressive, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<LuaString>, LuaEngine&, int>("entityType", bind(WorldEntityCallbacks::entityType, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<Vec2F>, int>("entityPosition", bind(WorldEntityCallbacks::entityPosition, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<Vec2F>, int>("entityVelocity", bind(WorldEntityCallbacks::entityVelocity, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<RectF>, int>("entityMetaBoundBox", bind(WorldEntityCallbacks::entityMetaBoundBox, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<uint64_t>, EntityId, String>("entityCurrency", bind(WorldEntityCallbacks::entityCurrency, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<uint64_t>, EntityId, Json, Maybe<bool>>("entityHasCountOfItem", bind(WorldEntityCallbacks::entityHasCountOfItem, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<Vec2F>, EntityId>("entityHealth", bind(WorldEntityCallbacks::entityHealth, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId>("entitySpecies", bind(WorldEntityCallbacks::entitySpecies, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId>("entityGender", bind(WorldEntityCallbacks::entityGender, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId>("entityName", bind(WorldEntityCallbacks::entityName, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId, Maybe<String>>("entityDescription", bind(WorldEntityCallbacks::entityDescription, world, _1, _2));
    callbacks.registerCallbackWithSignature<LuaNullTermWrapper<Maybe<List<Drawable>>>, EntityId, String>("entityPortrait", bind(WorldEntityCallbacks::entityPortrait, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId, String>("entityHandItem", bind(WorldEntityCallbacks::entityHandItem, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId, String>("entityHandItemDescriptor", bind(WorldEntityCallbacks::entityHandItemDescriptor, world, _1, _2));
    callbacks.registerCallbackWithSignature<LuaNullTermWrapper<Maybe<String>>, EntityId>("entityUniqueId", bind(WorldEntityCallbacks::entityUniqueId, world, _1));
    callbacks.registerCallbackWithSignature<Json, EntityId, String, Maybe<Json>>("getObjectParameter", bind(WorldEntityCallbacks::getObjectParameter, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Json, EntityId, String, Maybe<Json>>("getNpcScriptParameter", bind(WorldEntityCallbacks::getNpcScriptParameter, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<List<Vec2I>, EntityId>("objectSpaces", bind(WorldEntityCallbacks::objectSpaces, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<int>, EntityId>("farmableStage", bind(WorldEntityCallbacks::farmableStage, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<int>, EntityId>("containerSize", bind(WorldEntityCallbacks::containerSize, world, _1));
    callbacks.registerCallbackWithSignature<bool, EntityId>("containerClose", bind(WorldEntityCallbacks::containerClose, world, _1));
    callbacks.registerCallbackWithSignature<bool, EntityId>("containerOpen", bind(WorldEntityCallbacks::containerOpen, world, _1));
    callbacks.registerCallbackWithSignature<Json, EntityId>("containerItems", bind(WorldEntityCallbacks::containerItems, world, _1));
    callbacks.registerCallbackWithSignature<Json, EntityId, size_t>("containerItemAt", bind(WorldEntityCallbacks::containerItemAt, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<bool>, EntityId, Json>("containerConsume", bind(WorldEntityCallbacks::containerConsume, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<bool>, EntityId, size_t, int>("containerConsumeAt", bind(WorldEntityCallbacks::containerConsumeAt, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<size_t>, EntityId, Json>("containerAvailable", bind(WorldEntityCallbacks::containerAvailable, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId>("containerTakeAll", bind(WorldEntityCallbacks::containerTakeAll, world, _1));
    callbacks.registerCallbackWithSignature<Json, EntityId, size_t>("containerTakeAt", bind(WorldEntityCallbacks::containerTakeAt, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId, size_t, int>("containerTakeNumItemsAt", bind(WorldEntityCallbacks::containerTakeNumItemsAt, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<size_t>, EntityId, Json>("containerItemsCanFit", bind(WorldEntityCallbacks::containerItemsCanFit, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId, Json>("containerItemsFitWhere", bind(WorldEntityCallbacks::containerItemsFitWhere, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId, Json>("containerAddItems", bind(WorldEntityCallbacks::containerAddItems, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId, Json>("containerStackItems", bind(WorldEntityCallbacks::containerStackItems, world, _1, _2));
    callbacks.registerCallbackWithSignature<Json, EntityId, Json, size_t>("containerPutItemsAt", bind(WorldEntityCallbacks::containerPutItemsAt, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Json, EntityId, Json, size_t>("containerSwapItems", bind(WorldEntityCallbacks::containerSwapItems, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Json, EntityId, Json, size_t>("containerSwapItemsNoCombine", bind(WorldEntityCallbacks::containerSwapItemsNoCombine, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Json, EntityId, Json, size_t>("containerItemApply", bind(WorldEntityCallbacks::containerItemApply, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<Maybe<LuaValue>, EntityId, String, LuaVariadic<LuaValue>>("callScriptedEntity", bind(WorldEntityCallbacks::callScriptedEntity, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<RpcPromise<Vec2F>, String>("findUniqueEntity", bind(WorldEntityCallbacks::findUniqueEntity, world, _1));
    callbacks.registerCallbackWithSignature<RpcPromise<Json>, LuaEngine&, LuaValue, String, LuaVariadic<Json>>("sendEntityMessage", bind(WorldEntityCallbacks::sendEntityMessage, world, _1, _2, _3, _4));
    callbacks.registerCallbackWithSignature<Maybe<bool>, EntityId>("loungeableOccupied", bind(WorldEntityCallbacks::loungeableOccupied, world, _1));
    callbacks.registerCallbackWithSignature<bool, EntityId, Maybe<bool>>("isMonster", bind(WorldEntityCallbacks::isMonster, world, _1, _2));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId>("monsterType", bind(WorldEntityCallbacks::monsterType, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId>("npcType", bind(WorldEntityCallbacks::npcType, world, _1));
    callbacks.registerCallbackWithSignature<Maybe<String>, EntityId>("stagehandType", bind(WorldEntityCallbacks::stagehandType, world, _1));
    callbacks.registerCallbackWithSignature<bool, EntityId, Maybe<int>>("isNpc", bind(WorldEntityCallbacks::isNpc, world, _1, _2));
    callbacks.registerCallback("isEntityInteractive", [world](EntityId entityId) -> Maybe<bool> {
        if (auto entity = world->get<InteractiveEntity>(entityId))
          return entity->isInteractive();
        return {};
      });
    callbacks.registerCallback("entityMouthPosition", [world](EntityId entityId) -> Maybe<Vec2F> {
        if (auto entity = world->get<ChattyEntity>(entityId))
          return entity->mouthPosition();
        return {};
      });
    callbacks.registerCallback("entityTypeName", [world](EntityId entityId) -> Maybe<String> {
        auto entity = world->entity(entityId);
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
  }

  void addWorldEnvironmentCallbacks(LuaCallbacks& callbacks, World* world) {
    callbacks.registerCallbackWithSignature<float, Vec2F>("lightLevel", bind(WorldEnvironmentCallbacks::lightLevel, world, _1));
    callbacks.registerCallbackWithSignature<float, Vec2F>("windLevel", bind(WorldEnvironmentCallbacks::windLevel, world, _1));
    callbacks.registerCallbackWithSignature<bool, Vec2F>("breathable", bind(WorldEnvironmentCallbacks::breathable, world, _1));
    callbacks.registerCallbackWithSignature<bool, Vec2F>("underground", bind(WorldEnvironmentCallbacks::underground, world, _1));
    callbacks.registerCallbackWithSignature<LuaValue, LuaEngine&, Vec2F, String>("material", bind(WorldEnvironmentCallbacks::material, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<LuaValue, LuaEngine&, Vec2F, String>("mod", bind(WorldEnvironmentCallbacks::mod, world, _1, _2, _3));
    callbacks.registerCallbackWithSignature<float, Vec2F, String>("materialHueShift", bind(WorldEnvironmentCallbacks::materialHueShift, world, _1, _2));
    callbacks.registerCallbackWithSignature<float, Vec2F, String>("modHueShift", bind(WorldEnvironmentCallbacks::modHueShift, world, _1, _2));
    callbacks.registerCallbackWithSignature<MaterialColorVariant, Vec2F, String>("materialColor", bind(WorldEnvironmentCallbacks::materialColor, world, _1, _2));
    callbacks.registerCallbackWithSignature<void, Vec2F, String, MaterialColorVariant>("setMaterialColor", bind(WorldEnvironmentCallbacks::setMaterialColor, world, _1, _2, _3));

    callbacks.registerCallback("oceanLevel", [world](Vec2I position) -> int {
        if (auto serverWorld = as<WorldServer>(world)) {
          return serverWorld->worldTemplate()->blockInfo(position[0], position[1]).oceanLiquidLevel;
        } else {
          auto clientWorld = as<WorldClient>(world);
          return clientWorld->currentTemplate()->blockInfo(position[0], position[1]).oceanLiquidLevel;
        }
      });

    callbacks.registerCallback("environmentStatusEffects", [world](Vec2F const& position) {
        return world->environmentStatusEffects(position);
      });

    callbacks.registerCallbackWithSignature<bool, List<Vec2I>, String, Vec2F, String, float, Maybe<unsigned>, Maybe<EntityId>>("damageTiles", bind(WorldEnvironmentCallbacks::damageTiles, world, _1, _2, _3, _4, _5, _6, _7));
    callbacks.registerCallbackWithSignature<bool, Vec2F, float, String, Vec2F, String, float, Maybe<unsigned>, Maybe<EntityId>>("damageTileArea", bind(WorldEnvironmentCallbacks::damageTileArea, world, _1, _2, _3, _4, _5, _6, _7, _8));
    callbacks.registerCallbackWithSignature<bool, Vec2I, String, String, Maybe<int>, bool>("placeMaterial", bind(WorldEnvironmentCallbacks::placeMaterial, world, _1, _2, _3, _4, _5));
    callbacks.registerCallbackWithSignature<bool, Vec2I, String, String, Maybe<int>, bool>("placeMod", bind(WorldEnvironmentCallbacks::placeMod, world, _1, _2, _3, _4, _5));

    callbacks.registerCallback("radialTileQuery", [world](Vec2F center, float radius, String layerName) -> List<Vec2I> {
      auto layer = TileLayerNames.getLeft(layerName);
      return tileAreaBrush(radius, center, false).filtered([&](Vec2I const& t) -> bool {
        return world->material(t, layer) != EmptyMaterialId;
      });
    });
  }

  float WorldCallbacks::magnitude(World* world, Vec2F pos1, Maybe<Vec2F> pos2) {
    if (pos2)
      return world->geometry().diff(pos1, *pos2).magnitude();
    else
      return pos1.magnitude();
  }

  Vec2F WorldCallbacks::distance(World* world, Vec2F const& arg1, Vec2F const& arg2) {
    return world->geometry().diff(arg1, arg2);
  }

  bool WorldCallbacks::polyContains(World* world, PolyF const& poly, Vec2F const& pos) {
    return world->geometry().polyContains(poly, pos);
  }

  LuaValue WorldCallbacks::xwrap(World* world, LuaEngine& engine, LuaValue const& positionOrX) {
    if (auto x = engine.luaMaybeTo<float>(positionOrX))
      return LuaFloat(world->geometry().xwrap(*x));
    return engine.luaFrom<Vec2F>(world->geometry().xwrap(engine.luaTo<Vec2F>(positionOrX)));
  }

  LuaValue WorldCallbacks::nearestTo(World* world, LuaEngine& engine, Variant<Vec2F, float> const& sourcePositionOrX, Variant<Vec2F, float> const& targetPositionOrX) {
    if (targetPositionOrX.is<Vec2F>()) {
      Vec2F targetPosition = targetPositionOrX.get<Vec2F>();
      Vec2F sourcePosition;
      if (sourcePositionOrX.is<Vec2F>())
        sourcePosition = sourcePositionOrX.get<Vec2F>();
      else
        sourcePosition[0] = sourcePositionOrX.get<float>();

      return engine.luaFrom<Vec2F>(world->geometry().nearestTo(sourcePosition, targetPosition));

    } else {
      float targetX = targetPositionOrX.get<float>();
      float sourceX;
      if (sourcePositionOrX.is<Vec2F>())
        sourceX = sourcePositionOrX.get<Vec2F>()[0];
      else
        sourceX = sourcePositionOrX.get<float>();

      return LuaFloat(world->geometry().nearestTo(sourceX, targetX));
    }
  }

  bool WorldCallbacks::rectCollision(World* world, RectF const& arg1, Maybe<CollisionSet> const& arg2) {
    PolyF body = PolyF(arg1);

    if (arg2)
      return world->polyCollision(body, *arg2);
    else
      return world->polyCollision(body);
  }

  bool WorldCallbacks::pointTileCollision(World* world, Vec2F const& arg1, Maybe<CollisionSet> const& arg2) {
    if (arg2)
      return world->pointTileCollision(arg1, *arg2);
    else
      return world->pointTileCollision(arg1);
  }

  bool WorldCallbacks::lineTileCollision(
      World* world, Vec2F const& arg1, Vec2F const& arg2, Maybe<CollisionSet> const& arg3) {
    Vec2F const begin = arg1;
    Vec2F const end = arg2;

    if (arg3)
      return world->lineTileCollision(begin, end, *arg3);
    else
      return world->lineTileCollision(begin, end);
  }

  Maybe<pair<Vec2F, Vec2I>> WorldCallbacks::lineTileCollisionPoint(World* world, Vec2F const& begin, Vec2F const& end, Maybe<CollisionSet> const& collisionSet) {
    if (collisionSet)
      return world->lineTileCollisionPoint(begin, end, *collisionSet);
    else
      return world->lineTileCollisionPoint(begin, end);
  }

  bool WorldCallbacks::rectTileCollision(World* world, RectF const& arg1, Maybe<CollisionSet> const& arg2) {
    RectI const region = RectI::integral(arg1);

    if (arg2)
      return world->rectTileCollision(region, *arg2);
    else
      return world->rectTileCollision(region);
  }

  bool WorldCallbacks::pointCollision(World* world, Vec2F const& point, Maybe<CollisionSet> const& collisionSet) {
    return world->pointCollision(point, collisionSet.value(DefaultCollisionSet));
  }

  LuaTupleReturn<Maybe<Vec2F>, Maybe<Vec2F>> WorldCallbacks::lineCollision(World* world, Vec2F const& start, Vec2F const& end, Maybe<CollisionSet> const& collisionSet) {
    Maybe<Vec2F> point;
    Maybe<Vec2F> normal;
    auto collision = world->lineCollision(Line2F(start, end), collisionSet.value(DefaultCollisionSet));
    if (collision) {
      point = collision->first;
      normal = collision->second;
    }
    return luaTupleReturn(point, normal);
  }

  bool WorldCallbacks::polyCollision(
      World* world, PolyF const& arg1, Maybe<Vec2F> const& arg2, Maybe<CollisionSet> const& arg3) {
    PolyF body = arg1;

    Vec2F center;
    if (arg2) {
      center = *arg2;
      body.translate(center);
    }

    if (arg3)
      return world->polyCollision(body, *arg3);
    else
      return world->polyCollision(body);
  }

  List<Vec2I> WorldCallbacks::collisionBlocksAlongLine(
      World* world, Vec2F const& arg1, Vec2F const& arg2, Maybe<CollisionSet> const& arg3, Maybe<int> const& arg4) {
    Vec2F const begin = arg1;
    Vec2F const end = arg2;

    CollisionSet collisionSet = arg3.value(DefaultCollisionSet);
    int const maxSize = arg4 ? *arg4 : -1;
    return world->collidingTilesAlongLine(begin, end, collisionSet, maxSize);
  }

  List<pair<Vec2I, LiquidLevel>> WorldCallbacks::liquidAlongLine(World* world, Vec2F const& start, Vec2F const& end) {
    List<pair<Vec2I, LiquidLevel>> levels;
    forBlocksAlongLine<float>(start, world->geometry().diff(end, start), [&](int x, int y) {
        auto liquidLevel = world->liquidLevel(RectF::withSize(Vec2F(x, y), Vec2F(1, 1)));
        if (liquidLevel.liquid != EmptyLiquidId)
          levels.append(pair<Vec2I, LiquidLevel>(Vec2I(x, y), liquidLevel));
        return true;
      });
    return levels;
  }

  Maybe<Vec2F> WorldCallbacks::resolvePolyCollision(
      World* world, PolyF poly, Vec2F const& position, float maximumCorrection, Maybe<CollisionSet> const& maybeCollisionSet) {
    struct CollisionPoly {
      PolyF poly;
      Vec2F center;
      float sortingDistance;
    };

    poly.translate(position);
    List<CollisionPoly> collisions;
    CollisionSet collisionSet = maybeCollisionSet.value(DefaultCollisionSet);
    world->forEachCollisionBlock(RectI::integral(poly.boundBox().padded(maximumCorrection + 1.0f)),
        [&](auto const& block) {
          if (collisionSet.contains(block.kind))
            collisions.append({block.poly, Vec2F(block.space), 0.0f});
        });

    auto resolveCollision = [&](Maybe<Vec2F> direction, float maximumDistance, int loops) -> Maybe<Vec2F> {
      PolyF body = poly;
      Vec2F correction;
      for (int i = 0; i < loops; ++i) {
        Vec2F bodyCenter = body.center();
        for (auto& cp : collisions)
          cp.sortingDistance = vmagSquared(bodyCenter - cp.center);
        sort(collisions, [](auto const& a, auto const& b) { return a.sortingDistance < b.sortingDistance; });

        bool anyIntersects = false;
        for (auto const& cp : collisions) {
          PolyF::IntersectResult intersection;
          if (direction)
            intersection = body.directionalSatIntersection(cp.poly, *direction, false);
          else
            intersection = body.satIntersection(cp.poly);

          if (intersection.intersects) {
            anyIntersects = true;
            body.translate(intersection.overlap);
            correction += intersection.overlap;
            if (vmag(correction) > maximumDistance)
              return {};
          }
        }

        if (!anyIntersects)
          return correction;
      }

      for (auto const& cp : collisions) {
        if (body.intersects(cp.poly))
          return {};
      }

      return correction;
    };

    // First try any-directional SAT separation for two loops
    if (auto resolution = resolveCollision({}, maximumCorrection, 2))
      return position + *resolution;

    // Then, try direction-limiting SAT in cardinals, then 45 degs, then in
    // between, for 16 total angles in a circle.
    for (int i : {4, 8, 12, 0, 2, 6, 10, 14, 1, 3, 7, 5, 15, 13, 9, 11}) {
      float angle = i * Constants::pi / 8;
      Vec2F dir = Vec2F::withAngle(angle, 1.0f);
      if (auto resolution = resolveCollision(dir, maximumCorrection, 1))
        return position + *resolution;
    }

    return {};
  }

  bool WorldCallbacks::tileIsOccupied(
      World* world, Vec2I const& arg1, Maybe<bool> const& arg2, Maybe<bool> const& arg3) {
    Vec2I const tile = arg1;
    bool const tileLayerBool = arg2.value(true);
    bool const includeEphemeral = arg3.value(false);

    TileLayer const tileLayer = tileLayerBool ? TileLayer::Foreground : TileLayer::Background;

    return world->tileIsOccupied(tile, tileLayer, includeEphemeral);
  }

  bool WorldCallbacks::placeObject(World* world,
      String const& objectType,
      Vec2I const& worldPosition,
      Maybe<int> const& objectDirection,
      Json const& objectParameters) {
    auto objectDatabase = Root::singleton().objectDatabase();

    try {
      Direction direction = Direction::Right;
      if (objectDirection && *objectDirection < 0)
        direction = Direction::Left;

      Json parameters = objectParameters ? objectParameters : JsonObject();

      auto placedObject = objectDatabase->createForPlacement(world, objectType, worldPosition, direction, parameters);
      if (placedObject) {
        world->addEntity(placedObject);
        return true;
      }
    } catch (StarException const& exception) {
      Logger::warn("Could not create placable object of kind '{}', exception caught: {}",
          objectType,
          outputException(exception, false));
    }

    return false;
  }

  Maybe<EntityId> WorldCallbacks::spawnItem(World* world,
      Json const& itemType,
      Vec2F const& worldPosition,
      Maybe<size_t> const& inputCount,
      Json const& inputParameters,
      Maybe<Vec2F> const& initialVelocity,
      Maybe<float> const& intangibleTime) {
    Vec2F const position = worldPosition;

    try {
      ItemDescriptor descriptor;
      if (itemType.isType(Json::Type::String)) {
        size_t count = inputCount.value(1);

        Json parameters = inputParameters ? inputParameters : JsonObject();

        descriptor = ItemDescriptor(itemType.toString(), count, parameters);
      } else {
        descriptor = ItemDescriptor(itemType);
      }

      if (auto itemDrop = ItemDrop::createRandomizedDrop(descriptor, position)) {
        if (initialVelocity)
          itemDrop->setVelocity(*initialVelocity);
        if (intangibleTime)
          itemDrop->setIntangibleTime(*intangibleTime);
        world->addEntity(itemDrop);
        return itemDrop->inWorld() ? itemDrop->entityId() : Maybe<EntityId>();
      }

      Logger::warn("Could not spawn item, item empty in WorldCallbacks::spawnItem");
    } catch (StarException const& exception) {
      Logger::warn("Could not spawn Item of kind '{}', exception caught: {}", itemType, outputException(exception, false));
    }

    return {};
  }

  List<EntityId> WorldCallbacks::spawnTreasure(
      World* world, Vec2F const& position, String const& pool, float level, Maybe<uint64_t> seed) {
    List<EntityId> entities;
    auto treasureDatabase = Root::singleton().treasureDatabase();
    try {
      for (auto const& treasureItem : treasureDatabase->createTreasure(pool, level, seed.value(Random::randu64()))) {
        ItemDropPtr entity = ItemDrop::createRandomizedDrop(treasureItem, position);
        entities.append(entity->entityId());
        world->addEntity(entity);
      }
    } catch (StarException const& exception) {
      Logger::warn(
          "Could not spawn treasure from pool '{}', exception caught: {}", pool, outputException(exception, false));
    }
    return entities;
  }

  Maybe<EntityId> WorldCallbacks::spawnMonster(
      World* world, String const& arg1, Vec2F const& arg2, Maybe<JsonObject> const& arg3) {
    Vec2F const spawnPosition = arg2;
    auto monsterDatabase = Root::singleton().monsterDatabase();

    try {
      JsonObject parameters;
      parameters["aggressive"] = Random::randb();
      if (arg3)
        parameters.merge(*arg3, true);

      float level = 1;
      if (parameters.contains("level"))
        level = parameters.get("level").toFloat();
      auto monster = monsterDatabase->createMonster(monsterDatabase->randomMonster(arg1, parameters), level);

      monster->setPosition(spawnPosition);
      world->addEntity(monster);
      return monster->inWorld() ? monster->entityId() : Maybe<EntityId>();
    } catch (StarException const& exception) {
      Logger::warn(
          "Could not spawn Monster of type '{}', exception caught: {}", arg1, outputException(exception, false));
      return {};
    }
  }

  Maybe<EntityId> WorldCallbacks::spawnNpc(World* world,
      Vec2F const& arg1,
      String const& arg2,
      String const& arg3,
      float arg4,
      Maybe<uint64_t> arg5,
      Json const& arg6) {
    Vec2F const spawnPosition = arg1;

    String typeName = arg3;
    float level = arg4;

    uint64_t seed;
    if (arg5)
      seed = *arg5;
    else
      seed = Random::randu64();

    Json overrides = arg6 ? arg6 : JsonObject();

    auto npcDatabase = Root::singleton().npcDatabase();
    try {
      auto npc = npcDatabase->createNpc(npcDatabase->generateNpcVariant(arg2, typeName, level, seed, overrides));
      npc->setPosition(spawnPosition);
      world->addEntity(npc);
      return npc->inWorld() ? npc->entityId() : Maybe<EntityId>();
    } catch (StarException const& exception) {
      Logger::warn("Could not spawn NPC of species '{}' and type '{}', exception caught: {}",
          arg2,
          typeName,
          outputException(exception, false));
      return {};
    }
  }

  Maybe<EntityId> WorldCallbacks::spawnStagehand(
      World* world, Vec2F const& spawnPosition, String const& typeName, Json const& overrides) {
    auto stagehandDatabase = Root::singleton().stagehandDatabase();
    try {
      auto stagehand = stagehandDatabase->createStagehand(typeName, overrides);
      stagehand->setPosition(spawnPosition);
      world->addEntity(stagehand);
      return stagehand->inWorld() ? stagehand->entityId() : Maybe<EntityId>();
    } catch (StarException const& exception) {
      Logger::warn(
          "Could not spawn Stagehand of type '{}', exception caught: {}", typeName, outputException(exception, false));
      return {};
    }
  }

  Maybe<EntityId> WorldCallbacks::spawnProjectile(World* world,
      String const& projectileType,
      Vec2F const& spawnPosition,
      Maybe<EntityId> const& sourceEntityId,
      Maybe<Vec2F> const& projectileDirection,
      bool trackSourceEntity,
      Json const& projectileParameters) {

    try {
      auto projectileDatabase = Root::singleton().projectileDatabase();
      auto projectile = projectileDatabase->createProjectile(projectileType, projectileParameters ? projectileParameters : JsonObject());
      projectile->setInitialPosition(spawnPosition);
      projectile->setInitialDirection(projectileDirection.value());
      projectile->setSourceEntity(sourceEntityId.value(NullEntityId), trackSourceEntity);
      world->addEntity(projectile);
      return projectile->inWorld() ? projectile->entityId() : Maybe<EntityId>();
    } catch (StarException const& exception) {
      Logger::warn(
          "Could not spawn Projectile of type '{}', exception caught: {}", projectileType, outputException(exception, false));
      return {};
    }
  }

  Maybe<EntityId> WorldCallbacks::spawnVehicle(
      World* world, String const& vehicleName, Vec2F const& pos, Json const& extraConfig) {
    auto vehicleDatabase = Root::singleton().vehicleDatabase();
    auto vehicle = vehicleDatabase->create(vehicleName, extraConfig);
    vehicle->setPosition(pos);
    world->addEntity(vehicle);
    if (vehicle->inWorld())
      return vehicle->entityId();
    return {};
  }

  double WorldCallbacks::time(World* world) {
    return world->epochTime();
  }

  uint64_t WorldCallbacks::day(World* world) {
    return world->day();
  }

  double WorldCallbacks::timeOfDay(World* world) {
    return world->timeOfDay() / world->dayLength();
  }

  float WorldCallbacks::dayLength(World* world) {
    return world->dayLength();
  }

  Json WorldCallbacks::getProperty(World* world, String const& arg1, Json const& arg2) {
    return world->getProperty(arg1, arg2);
  }

  void WorldCallbacks::setProperty(World* world, String const& arg1, Json const& arg2) {
    world->setProperty(arg1, arg2);
  }

  Maybe<LiquidLevel> WorldCallbacks::liquidAt(World* world, Variant<RectF, Vec2I> boundBoxOrPoint) {
    LiquidLevel liquidLevel = boundBoxOrPoint.call([world](auto const& bbop) { return world->liquidLevel(bbop); });
    if (liquidLevel.liquid != EmptyLiquidId)
      return liquidLevel;
    return {};
  }

  float WorldCallbacks::gravity(World* world, Vec2F const& arg1) {
    return world->gravity(arg1);
  }

  bool WorldCallbacks::spawnLiquid(World* world, Vec2F const& position, LiquidId liquid, float quantity) {
    return world->modifyTile(Vec2I::floor(position), PlaceLiquid{liquid, quantity}, true);
  }

  Maybe<LiquidLevel> WorldCallbacks::destroyLiquid(World* world, Vec2F const& position) {
    auto liquidLevel = world->liquidLevel(Vec2I::floor(position));
    if (liquidLevel.liquid != EmptyLiquidId) {
      if (world->modifyTile(Vec2I::floor(position), PlaceLiquid{EmptyLiquidId, 0}, true))
        return liquidLevel;
    }
    return {};
  }

  bool WorldCallbacks::isTileProtected(World* world, Vec2F const& position) {
    return world->isTileProtected(Vec2I::floor(position));
  }

  Maybe<PlatformerAStar::Path> WorldCallbacks::findPlatformerPath(World* world,
      Vec2F const& start,
      Vec2F const& end,
      ActorMovementParameters actorMovementParameters,
      PlatformerAStar::Parameters searchParameters) {
    PlatformerAStar::PathFinder pathFinder(world, start, end, move(actorMovementParameters), move(searchParameters));
    pathFinder.explore({});
    return pathFinder.result();
  }

  PlatformerAStar::PathFinder WorldCallbacks::platformerPathStart(World* world,
      Vec2F const& start,
      Vec2F const& end,
      ActorMovementParameters actorMovementParameters,
      PlatformerAStar::Parameters searchParameters) {
    return PlatformerAStar::PathFinder(world, start, end, move(actorMovementParameters), move(searchParameters));
  }

  RectI ClientWorldCallbacks::clientWindow(WorldClient* world) {
    return world->clientWindow();	
  }

  bool ServerWorldCallbacks::breakObject(WorldServer* world, EntityId arg1, bool arg2) {
    if (auto entity = world->get<Object>(arg1)) {
      bool smash = arg2;
      entity->breakObject(smash);
      return true;
    }
    return false;
  }

  bool ServerWorldCallbacks::isVisibleToPlayer(WorldServer* world, RectF const& arg1) {
    return world->isVisibleToPlayer(arg1);
  }

  bool ServerWorldCallbacks::loadRegion(WorldServer* world, RectF const& arg1) {
    return world->signalRegion(RectI::integral(arg1));
  }

  bool ServerWorldCallbacks::regionActive(WorldServer* world, RectF const& arg1) {
    return world->regionActive(RectI::integral(arg1));
  }

  void ServerWorldCallbacks::setTileProtection(WorldServer* world, DungeonId arg1, bool arg2) {
    DungeonId dungeonId = arg1;
    bool isProtected = arg2;
    world->setTileProtection(dungeonId, isProtected);
  }

  bool ServerWorldCallbacks::isPlayerModified(WorldServer* world, RectI const& region) {
    return world->isPlayerModified(region);
  }

  Maybe<LiquidLevel> ServerWorldCallbacks::forceDestroyLiquid(WorldServer* world, Vec2F const& position) {
    auto liquidLevel = world->liquidLevel(Vec2I::floor(position));
    if (liquidLevel.liquid != EmptyLiquidId) {
      if (world->forceModifyTile(Vec2I::floor(position), PlaceLiquid{EmptyLiquidId, 0}, true))
        return liquidLevel;
    }
    return {};
  }

  EntityId ServerWorldCallbacks::loadUniqueEntity(WorldServer* world, String const& uniqueId) {
    return world->loadUniqueEntity(uniqueId);
  }

  void ServerWorldCallbacks::setUniqueId(WorldServer* world, EntityId entityId, Maybe<String> const& uniqueId) {
    auto entity = world->entity(entityId);
    if (auto npc = as<Npc>(entity.get()))
      npc->setUniqueId(uniqueId);
    else if (auto monster = as<Monster>(entity.get()))
      monster->setUniqueId(uniqueId);
    else if (auto object = as<Object>(entity.get()))
      object->setUniqueId(uniqueId);
    else if (auto stagehand = as<Stagehand>(entity.get()))
      stagehand->setUniqueId(uniqueId);
    else if (entity)
      throw StarException::format("Cannot set unique id on entity of type {}", EntityTypeNames.getRight(entity->entityType()));
    else
      throw StarException::format("No such entity with id {}", entityId);
  }

  Json ServerWorldCallbacks::takeItemDrop(World* world, EntityId entityId, Maybe<EntityId> const& takenBy) {
    auto itemDrop = world->get<ItemDrop>(entityId);
    if (itemDrop && itemDrop->canTake() && itemDrop->isMaster()) {
      ItemPtr item;
      if (takenBy)
        item = itemDrop->takeBy(*takenBy);
      else
        item = itemDrop->take();

      if (item)
        return item->descriptor().toJson();
    }

    return Json();
  }

  void ServerWorldCallbacks::setPlayerStart(World* world, Vec2F const& playerStart, Maybe<bool> respawnInWorld) {
    as<WorldServer>(world)->setPlayerStart(playerStart, respawnInWorld.isValid() && respawnInWorld.value());
  }

  List<EntityId> ServerWorldCallbacks::players(World* world) {
    return as<WorldServer>(world)->players();
  }

  LuaString ServerWorldCallbacks::fidelity(World* world, LuaEngine& engine) {
    return engine.createString(WorldServerFidelityNames.getRight(as<WorldServer>(world)->fidelity()));
  }

  Maybe<LuaValue> ServerWorldCallbacks::callScriptContext(World* world, String const& contextName, String const& function, LuaVariadic<LuaValue> const& args) {
    auto context = as<WorldServer>(world)->scriptContext(contextName);
    if (!context)
      throw StarException::format("Context {} does not exist", contextName);
    return context->invoke(function, args);
  }

  void WorldDebugCallbacks::debugPoint(Vec2F const& arg1, Color const& arg2) {
    SpatialLogger::logPoint("world", arg1, arg2.toRgba());
  }

  void WorldDebugCallbacks::debugLine(Vec2F const& arg1, Vec2F const& arg2, Color const& arg3) {
    SpatialLogger::logLine("world", arg1, arg2, arg3.toRgba());
  }

  void WorldDebugCallbacks::debugPoly(PolyF const& poly, Color const& color) {
    SpatialLogger::logPoly("world", poly, color.toRgba());
  }

  void WorldDebugCallbacks::debugText(LuaEngine& engine, LuaVariadic<LuaValue> const& args) {
    if (args.size() < 3)
      throw StarException(strf("Too few arguments to debugText: {}", args.size()));

    Vec2F const position = engine.luaTo<Vec2F>(args.at(args.size() - 2));
    Vec4B const color = engine.luaTo<Color>(args.at(args.size() - 1)).toRgba();

    String text = formatLua(engine.luaTo<String>(args.at(0)), slice<List<LuaValue>>(args, 1, args.size() - 2));
    SpatialLogger::logText("world", text, position, color);
  }

  LuaTable WorldEntityCallbacks::entityQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options) {
    return LuaBindings::entityQuery<Entity>(world, engine, pos1, pos2, move(options));
  }

  LuaTable WorldEntityCallbacks::monsterQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options) {
    return LuaBindings::entityQuery<Monster>(world, engine, pos1, pos2, move(options));
  }

  LuaTable WorldEntityCallbacks::npcQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options) {
    return LuaBindings::entityQuery<Npc>(world, engine, pos1, pos2, move(options));
  }

  LuaTable WorldEntityCallbacks::objectQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options) {
    String objectName;
    if (options)
      objectName = options->get<Maybe<String>>("name").value();

    return LuaBindings::entityQuery<Object>(world,
        engine,
        pos1,
        pos2,
        move(options),
        [&objectName](shared_ptr<Object> const& entity) -> bool {
          return objectName.empty() || entity->name() == objectName;
        });
  }

  LuaTable WorldEntityCallbacks::itemDropQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options) {
    return LuaBindings::entityQuery<ItemDrop>(world, engine, pos1, pos2, move(options));
  }

  LuaTable WorldEntityCallbacks::playerQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options) {
    return LuaBindings::entityQuery<Player>(world, engine, pos1, pos2, move(options));
  }

  LuaTable WorldEntityCallbacks::loungeableQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options) {
    String orientationName;
    if (options)
      orientationName = options->get<Maybe<String>>("orientation").value();

    LoungeOrientation orientation = LoungeOrientation::None;
    if (orientationName == "sit")
      orientation = LoungeOrientation::Sit;
    else if (orientationName == "lay")
      orientation = LoungeOrientation::Lay;
    else if (orientationName == "stand")
      orientation = LoungeOrientation::Stand;
    else if (orientationName.empty())
      orientation = LoungeOrientation::None;
    else
      throw StarException(strf("Unsupported loungeableQuery orientation {}", orientationName));

    auto filter = [orientation](shared_ptr<LoungeableObject> const& entity) -> bool {
      auto loungeable = as<LoungeableEntity>(entity);
      if (!loungeable || loungeable->anchorCount() == 0)
        return false;

      if (orientation == LoungeOrientation::None)
        return true;
      auto pos = loungeable->loungeAnchor(0);
      return pos && pos->orientation == orientation;
    };

    return LuaBindings::entityQuery<LoungeableObject>(world, engine, pos1, pos2, move(options), filter);
  }

  LuaTable WorldEntityCallbacks::entityLineQuery(World* world, LuaEngine& engine, Vec2F const& point1, Vec2F const& point2, Maybe<LuaTable> options) {
    return LuaBindings::entityLineQuery<Entity>(world, engine, point1, point2, move(options));
  }

  LuaTable WorldEntityCallbacks::objectLineQuery(World* world, LuaEngine& engine, Vec2F const& point1, Vec2F const& point2, Maybe<LuaTable> options) {
    return LuaBindings::entityLineQuery<Object>(world, engine, point1, point2, move(options));
  }

  LuaTable WorldEntityCallbacks::npcLineQuery(World* world, LuaEngine& engine, Vec2F const& point1, Vec2F const& point2, Maybe<LuaTable> options) {
    return LuaBindings::entityLineQuery<Npc>(world, engine, point1, point2, move(options));
  }

  bool WorldEntityCallbacks::entityExists(World* world, EntityId entityId) {
    return (bool)world->entity(entityId);
  }

  bool WorldEntityCallbacks::entityCanDamage(World* world, EntityId sourceId, EntityId targetId) {
    auto source = world->entity(sourceId);
    auto target = world->entity(targetId);

    if (!source || !target || !source->getTeam().canDamage(target->getTeam(), false))
      return false;

    return true;
  }

  Json WorldEntityCallbacks::entityDamageTeam(World* world, EntityId entityId) {
    if (auto entity = world->entity(entityId))
      return entity->getTeam().toJson();
    return {};
  }

  bool WorldEntityCallbacks::entityAggressive(World* world, EntityId entityId) {
    auto entity = world->entity(entityId);
    if (auto monster = as<Monster>(entity))
      return monster->aggressive();
    if (auto npc = as<Npc>(entity))
      return npc->aggressive();
    return false;
  }

  Maybe<LuaString> WorldEntityCallbacks::entityType(World* world, LuaEngine& engine, EntityId entityId) {
    if (auto entity = world->entity(entityId))
      return engine.createString(EntityTypeNames.getRight(entity->entityType()));
    return {};
  }

  Maybe<Vec2F> WorldEntityCallbacks::entityPosition(World* world, EntityId entityId) {
    if (auto entity = world->entity(entityId)) {
      return entity->position();
    } else {
      return {};
    }
  }

  Maybe<RectF> WorldEntityCallbacks::entityMetaBoundBox(World* world, EntityId entityId) {
    if (auto entity = world->entity(entityId)) {
      return entity->metaBoundBox();
    } else {
      return {};
    }
  }

  Maybe<Vec2F> WorldEntityCallbacks::entityVelocity(World* world, EntityId entityId) {
    auto entity = world->entity(entityId);

    if (auto monsterEntity = as<Monster>(entity))
      return monsterEntity->velocity();
    else if (auto npcEntity = as<Npc>(entity))
      return npcEntity->velocity();
    else if (auto playerEntity = as<Player>(entity))
      return playerEntity->velocity();
    else if (auto vehicleEntity = as<Vehicle>(entity))
      return vehicleEntity->velocity();

    return {};
  }

  Maybe<uint64_t> WorldEntityCallbacks::entityCurrency(World* world, EntityId entityId, String const& currencyType) {
    if (auto player = world->get<Player>(entityId)) {
      return player->currency(currencyType);
    } else {
      return {};
    }
  }

  Maybe<uint64_t> WorldEntityCallbacks::entityHasCountOfItem(World* world, EntityId entityId, Json descriptor, Maybe<bool> exactMatch) {
    if (auto player = world->get<Player>(entityId)) {
      return player->inventory()->hasCountOfItem(ItemDescriptor(descriptor), exactMatch.value(false));
    } else {
      return {};
    }
  }

  Maybe<Vec2F> WorldEntityCallbacks::entityHealth(World* world, EntityId entityId) {
    if (auto entity = world->get<DamageBarEntity>(entityId)) {
      return Vec2F(entity->health(), entity->maxHealth());
    } else {
      return {};
    }
  }

  Maybe<String> WorldEntityCallbacks::entitySpecies(World* world, EntityId entityId) {
    if (auto player = world->get<Player>(entityId)) {
      return player->species();
    } else if (auto npc = world->get<Npc>(entityId)) {
      return npc->species();
    } else {
      return {};
    }
  }

  Maybe<String> WorldEntityCallbacks::entityGender(World* world, EntityId entityId) {
    if (auto player = world->get<Player>(entityId)) {
      return GenderNames.getRight(player->gender());
    } else if (auto npc = world->get<Npc>(entityId)) {
      return GenderNames.getRight(npc->gender());
    } else {
      return {};
    }
  }

  Maybe<String> WorldEntityCallbacks::entityName(World* world, EntityId entityId) {
    auto entity = world->entity(entityId);

    if (auto portraitEntity = as<PortraitEntity>(entity)) {
      return portraitEntity->name();
    } else if (auto objectEntity = as<Object>(entity)) {
      return objectEntity->name();
    } else if (auto itemDropEntity = as<ItemDrop>(entity)) {
      if (itemDropEntity->item())
        return itemDropEntity->item()->name();
    } else if (auto vehicleEntity = as<Vehicle>(entity)) {
      return vehicleEntity->name();
    } else if (auto stagehandEntity = as<Stagehand>(entity)) {
      return stagehandEntity->typeName();
    } else if (auto projectileEntity = as<Projectile>(entity)) {
      return projectileEntity->typeName();
    }

    return {};
  }

  Maybe<String> WorldEntityCallbacks::entityDescription(World* world, EntityId entityId, Maybe<String> const& species) {
    if (auto entity = world->entity(entityId)) {
      if (auto inspectableEntity = as<InspectableEntity>(entity)) {
        if (species)
          return inspectableEntity->inspectionDescription(*species);
      }

      return entity->description();
    }

    return {};
  }

  LuaNullTermWrapper<Maybe<List<Drawable>>> WorldEntityCallbacks::entityPortrait(World* world, EntityId entityId, String const& portraitMode) {
    if (auto portraitEntity = as<PortraitEntity>(world->entity(entityId)))
      return portraitEntity->portrait(PortraitModeNames.getLeft(portraitMode));

    return {};
  }

  Maybe<String> WorldEntityCallbacks::entityHandItem(World* world, EntityId entityId, String const& handName) {
    ToolHand toolHand;
    if (handName == "primary") {
      toolHand = ToolHand::Primary;
    } else if (handName == "alt") {
      toolHand = ToolHand::Alt;
    } else {
      throw StarException(strf("Unknown tool hand {}", handName));
    }

    if (auto entity = world->get<ToolUserEntity>(entityId)) {
      if (auto item = entity->handItem(toolHand)) {
        return item->name();
      }
    }

    return {};
  }

  Json WorldEntityCallbacks::entityHandItemDescriptor(World* world, EntityId entityId, String const& handName) {
    ToolHand toolHand;
    if (handName == "primary") {
      toolHand = ToolHand::Primary;
    } else if (handName == "alt") {
      toolHand = ToolHand::Alt;
    } else {
      throw StarException(strf("Unknown tool hand {}", handName));
    }

    if (auto entity = world->get<ToolUserEntity>(entityId)) {
      if (auto item = entity->handItem(toolHand)) {
        return item->descriptor().toJson();
      }
    }

    return Json();
  }

  LuaNullTermWrapper<Maybe<String>> WorldEntityCallbacks::entityUniqueId(World* world, EntityId entityId) {
    if (auto entity = world->entity(entityId))
      return entity->uniqueId();
    return {};
  }

  Json WorldEntityCallbacks::getObjectParameter(World* world, EntityId entityId, String const& parameterName, Maybe<Json> const& defaultValue) {
    Json val = Json();

    if (auto objectEntity = as<Object>(world->entity(entityId))) {
      val = objectEntity->configValue(parameterName);
      if (!val && defaultValue)
        val = *defaultValue;
    }

    return val;
  }

  Json WorldEntityCallbacks::getNpcScriptParameter(World* world, EntityId entityId, String const& parameterName, Maybe<Json> const& defaultValue) {
    Json val = Json();

    if (auto npcEntity = as<Npc>(world->entity(entityId))) {
      val = npcEntity->scriptConfigParameter(parameterName);
      if (!val && defaultValue)
        val = *defaultValue;
    }

    return val;
  }

  List<Vec2I> WorldEntityCallbacks::objectSpaces(World* world, EntityId entityId) {
    if (auto tileEntity = as<TileEntity>(world->entity(entityId)))
      return tileEntity->spaces();
    return {};
  }

  Maybe<int> WorldEntityCallbacks::farmableStage(World* world, EntityId entityId) {
    if (auto farmable = world->get<FarmableObject>(entityId)) {
      return farmable->stage();
    }

    return {};
  }

  Maybe<int> WorldEntityCallbacks::containerSize(World* world, EntityId entityId) {
    if (auto container = world->get<ContainerObject>(entityId))
      return container->containerSize();

    return {};
  }

  bool WorldEntityCallbacks::containerClose(World* world, EntityId entityId) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      container->containerClose();
      return true;
    }

    return false;
  }

  bool WorldEntityCallbacks::containerOpen(World* world, EntityId entityId) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      container->containerOpen();
      return true;
    }

    return false;
  }

  Json WorldEntityCallbacks::containerItems(World* world, EntityId entityId) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      JsonArray res;
      auto itemDb = Root::singleton().itemDatabase();
      for (auto const& item : container->itemBag()->items())
        res.append(itemDb->toJson(item));
      return res;
    }

    return Json();
  }

  Json WorldEntityCallbacks::containerItemAt(World* world, EntityId entityId, size_t offset) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto items = container->itemBag()->items();
      if (offset < items.size()) {
        return itemDb->toJson(items.at(offset));
      }
    }

    return Json();
  }

  Maybe<bool> WorldEntityCallbacks::containerConsume(World* world, EntityId entityId, Json const& items) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto toConsume = ItemDescriptor(items);
      return container->consumeItems(toConsume).result();
    }

    return {};
  }

  Maybe<bool> WorldEntityCallbacks::containerConsumeAt(World* world, EntityId entityId, size_t offset, int count) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      if (offset < container->containerSize()) {
        return container->consumeItems(offset, count).result();
      }
    }

    return {};
  }

  Maybe<size_t> WorldEntityCallbacks::containerAvailable(World* world, EntityId entityId, Json const& items) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemBag = container->itemBag();
      auto toCheck = ItemDescriptor(items);
      return itemBag->available(toCheck);
    }

    return {};
  }

  Json WorldEntityCallbacks::containerTakeAll(World* world, EntityId entityId) {
    auto itemDb = Root::singleton().itemDatabase();
    if (auto container = world->get<ContainerObject>(entityId)) {
      if (auto itemList = container->clearContainer().result()) {
        JsonArray res;
        for (auto item : *itemList)
          res.append(itemDb->toJson(item));
        return res;
      }
    }

    return Json();
  }

  Json WorldEntityCallbacks::containerTakeAt(World* world, EntityId entityId, size_t offset) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      if (offset < container->containerSize()) {
        if (auto res = container->takeItems(offset).result())
          return itemDb->toJson(*res);
      }
    }

    return Json();
  }

  Json WorldEntityCallbacks::containerTakeNumItemsAt(World* world, EntityId entityId, size_t offset, int const& count) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      if (offset < container->containerSize()) {
        if (auto res = container->takeItems(offset, count).result())
          return itemDb->toJson(*res);
      }
    }

    return Json();
  }

  Maybe<size_t> WorldEntityCallbacks::containerItemsCanFit(World* world, EntityId entityId, Json const& items) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto itemBag = container->itemBag();
      auto toSearch = itemDb->fromJson(items);
      return itemBag->itemsCanFit(toSearch);
    }

    return {};
  }

  Json WorldEntityCallbacks::containerItemsFitWhere(World* world, EntityId entityId, Json const& items) {
    if (auto container = world->get<ContainerObject>(entityId)) {
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
  }

  Json WorldEntityCallbacks::containerAddItems(World* world, EntityId entityId, Json const& items) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto toInsert = itemDb->fromJson(items);
      if (auto res = container->addItems(toInsert).result())
        return itemDb->toJson(*res);
    }

    return items;
  }

  Json WorldEntityCallbacks::containerStackItems(World* world, EntityId entityId, Json const& items) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto toInsert = itemDb->fromJson(items);
      if (auto res = container->addItems(toInsert).result())
        return itemDb->toJson(*res);
    }

    return items;
  }

  Json WorldEntityCallbacks::containerPutItemsAt(World* world, EntityId entityId, Json const& items, size_t offset) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto toInsert = itemDb->fromJson(items);
      if (offset < container->containerSize()) {
        if (auto res = container->putItems(offset, toInsert).result())
          return itemDb->toJson(*res);
      }
    }

    return items;
  }

  Json WorldEntityCallbacks::containerSwapItems(World* world, EntityId entityId, Json const& items, size_t offset) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto toSwap = itemDb->fromJson(items);
      if (offset < container->containerSize()) {
        if (auto res = container->swapItems(offset, toSwap, true).result())
          return itemDb->toJson(*res);
      }
    }

    return items;
  }

  Json WorldEntityCallbacks::containerSwapItemsNoCombine(World* world, EntityId entityId, Json const& items, size_t offset) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto toSwap = itemDb->fromJson(items);
      if (offset < container->containerSize()) {
        if (auto res = container->swapItems(offset, toSwap, false).result())
          return itemDb->toJson(*res);
      }
    }

    return items;
  }

  Json WorldEntityCallbacks::containerItemApply(World* world, EntityId entityId, Json const& items, size_t offset) {
    if (auto container = world->get<ContainerObject>(entityId)) {
      auto itemDb = Root::singleton().itemDatabase();
      auto toSwap = itemDb->fromJson(items);
      if (offset < container->containerSize()) {
        if (auto res = container->swapItems(offset, toSwap, false).result())
          return itemDb->toJson(*res);
      }
    }

    return items;
  }

  Maybe<LuaValue> WorldEntityCallbacks::callScriptedEntity(World* world, EntityId entityId, String const& function, LuaVariadic<LuaValue> const& args) {
    auto entity = as<ScriptedEntity>(world->entity(entityId));
    if (!entity || !entity->isMaster())
      throw StarException::format("Entity {} does not exist or is not a local master scripted entity", entityId);
    return entity->callScript(function, args);
  }

  RpcPromise<Vec2F> WorldEntityCallbacks::findUniqueEntity(World* world, String const& uniqueId) {
    return world->findUniqueEntity(uniqueId);
  }

  RpcPromise<Json> WorldEntityCallbacks::sendEntityMessage(World* world, LuaEngine& engine, LuaValue entityId, String const& message, LuaVariadic<Json> args) {
    if (entityId.is<LuaString>())
      return world->sendEntityMessage(engine.luaTo<String>(entityId), message, JsonArray::from(move(args)));
    else
      return world->sendEntityMessage(engine.luaTo<EntityId>(entityId), message, JsonArray::from(move(args)));
  }

  Maybe<bool> WorldEntityCallbacks::loungeableOccupied(World* world, EntityId entityId) {
    auto entity = world->get<LoungeableEntity>(entityId);
    if (entity && entity->anchorCount() > 0)
      return !entity->entitiesLoungingIn(0).empty();
    return {};
  }

  bool WorldEntityCallbacks::isMonster(World* world, EntityId entityId, Maybe<bool> const& aggressive) {
    if (auto entity = world->get<Monster>(entityId))
      return !aggressive || *aggressive == entity->aggressive();

    return false;
  }

  Maybe<String> WorldEntityCallbacks::monsterType(World* world, EntityId entityId) {
    if (auto monster = world->get<Monster>(entityId))
      return monster->typeName();

    return {};
  }

  Maybe<String> WorldEntityCallbacks::npcType(World* world, EntityId entityId) {
    if (auto npc = world->get<Npc>(entityId))
      return npc->npcType();

    return {};
  }

  Maybe<String> WorldEntityCallbacks::stagehandType(World* world, EntityId entityId) {
    if (auto stagehand = world->get<Stagehand>(entityId))
      return stagehand->typeName();

    return {};
  }

  bool WorldEntityCallbacks::isNpc(World* world, EntityId entityId, Maybe<int> const& damageTeam) {
    if (auto entity = world->get<Npc>(entityId)) {
      return !damageTeam || *damageTeam == entity->getTeam().team;
    }

    return false;
  }

  float WorldEnvironmentCallbacks::lightLevel(World* world, Vec2F const& position) {
    return world->lightLevel(position);
  }

  float WorldEnvironmentCallbacks::windLevel(World* world, Vec2F const& position) {
    return world->windLevel(position);
  }

  bool WorldEnvironmentCallbacks::breathable(World* world, Vec2F const& position) {
    return world->breathable(position);
  }

  bool WorldEnvironmentCallbacks::underground(World* world, Vec2F const& position) {
    return world->isUnderground(position);
  }

  LuaValue WorldEnvironmentCallbacks::material(World* world, LuaEngine& engine, Vec2F const& position, String const& layerName) {
    TileLayer layer;
    if (layerName == "foreground") {
      layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported material layer {}", layerName));
    }

    auto materialId = world->material(Vec2I::floor(position), layer);
    if (materialId == NullMaterialId) {
      return LuaNil;
    } else if (materialId == EmptyMaterialId) {
      return false;
    } else {
      auto materialDatabase = Root::singleton().materialDatabase();
      return engine.createString(materialDatabase->materialName(materialId));
    }
  }

  LuaValue WorldEnvironmentCallbacks::mod(World* world, LuaEngine& engine, Vec2F const& position, String const& layerName) {
    TileLayer layer;
    if (layerName == "foreground") {
      layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported mod layer {}", layerName));
    }

    auto modId = world->mod(Vec2I::floor(position), layer);
    if (isRealMod(modId)) {
      auto materialDatabase = Root::singleton().materialDatabase();
      return engine.createString(materialDatabase->modName(modId));
    }

    return LuaNil;
  }

  float WorldEnvironmentCallbacks::materialHueShift(World* world, Vec2F const& position, String const& layerName) {
    TileLayer layer;
    if (layerName == "foreground") {
      layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported material layer {}", layerName));
    }

    return world->materialHueShift(Vec2I::floor(position), layer);
  }

  float WorldEnvironmentCallbacks::modHueShift(World* world, Vec2F const& position, String const& layerName) {
    TileLayer layer;
    if (layerName == "foreground") {
      layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported material layer {}", layerName));
    }

    return world->modHueShift(Vec2I::floor(position), layer);
  }

  MaterialColorVariant WorldEnvironmentCallbacks::materialColor(World* world, Vec2F const& position, String const& layerName) {
    TileLayer layer;
    if (layerName == "foreground") {
      layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported material layer {}", layerName));
    }

    return world->colorVariant(Vec2I::floor(position), layer);
  }

  void WorldEnvironmentCallbacks::setMaterialColor(World* world, Vec2F const& position, String const& layerName, MaterialColorVariant color) {
    TileLayer layer;
    if (layerName == "foreground") {
      layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported material layer {}", layerName));
    }

    world->modifyTile(Vec2I::floor(position), PlaceMaterialColor{layer, color}, true);
  }

  bool WorldEnvironmentCallbacks::damageTiles(World* world,
      List<Vec2I> const& arg1,
      String const& arg2,
      Vec2F const& arg3,
      String const& arg4,
      float arg5,
      Maybe<unsigned> const& arg6,
      Maybe<EntityId> sourceEntity) {
    List<Vec2I> tilePositions = arg1;

    TileLayer layer;
    auto layerName = arg2;
    if (layerName == "foreground") {
      layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported tile layer {}", layerName));
    }

    unsigned harvestLevel = 999;
    if (arg6)
      harvestLevel = *arg6;

    auto tileDamage = TileDamage(TileDamageTypeNames.getLeft(arg4), arg5, harvestLevel);
    auto res = world->damageTiles(tilePositions, layer, arg3, tileDamage, sourceEntity);
    return res != TileDamageResult::None;
  }

  bool WorldEnvironmentCallbacks::damageTileArea(World* world,
      Vec2F center,
      float radius,
      String layer,
      Vec2F sourcePosition,
      String damageType,
      float damage,
      Maybe<unsigned> const& harvestLevel,
      Maybe<EntityId> sourceEntity) {
    auto tiles = tileAreaBrush(radius, center, false);
    return damageTiles(world, tiles, layer, sourcePosition, damageType, damage, harvestLevel, sourceEntity);
  }

  bool WorldEnvironmentCallbacks::placeMaterial(World* world, Vec2I const& arg1, String const& arg2, String const& arg3, Maybe<int> const& arg4, bool arg5) {
    auto tilePosition = arg1;

    PlaceMaterial placeMaterial;

    std::string layerName = arg2.utf8();
    auto split = layerName.find_first_of('+');
    if (split != NPos) {
      auto overrideName = layerName.substr(split + 1);
      layerName = layerName.substr(0, split);
      if (overrideName == "empty" || overrideName == "none")
        placeMaterial.collisionOverride = TileCollisionOverride::Empty;
      else if (overrideName == "block")
        placeMaterial.collisionOverride = TileCollisionOverride::Block;
      else if (overrideName == "platform")
        placeMaterial.collisionOverride = TileCollisionOverride::Platform;
      else
        throw StarException(strf("Unsupported collision override {}", overrideName));
    }

    if (layerName == "foreground")
      placeMaterial.layer = TileLayer::Foreground;
    else if (layerName == "background")
      placeMaterial.layer = TileLayer::Background;
    else
      throw StarException(strf("Unsupported tile layer {}", layerName));

    auto materialName = arg3;
    auto materialDatabase = Root::singleton().materialDatabase();
    if (!materialDatabase->materialNames().contains(materialName))
      throw StarException(strf("Unknown material name {}", materialName));
    placeMaterial.material = materialDatabase->materialId(materialName);

    if (arg4)
      placeMaterial.materialHueShift = (MaterialHue)*arg4;

    bool allowOverlap = arg5;

    return world->modifyTile(tilePosition, placeMaterial, allowOverlap);
  }

  bool WorldEnvironmentCallbacks::placeMod(World* world, Vec2I const& arg1, String const& arg2, String const& arg3, Maybe<int> const& arg4, bool arg5) {
    auto tilePosition = arg1;

    PlaceMod placeMod;

    auto layerName = arg2;
    if (layerName == "foreground") {
      placeMod.layer = TileLayer::Foreground;
    } else if (layerName == "background") {
      placeMod.layer = TileLayer::Background;
    } else {
      throw StarException(strf("Unsupported tile layer {}", layerName));
    }

    auto modName = arg3;
    auto materialDatabase = Root::singleton().materialDatabase();
    if (!materialDatabase->modNames().contains(modName))
      throw StarException(strf("Unknown mod name {}", modName));
    placeMod.mod = materialDatabase->modId(modName);

    if (arg4)
      placeMod.modHueShift = (MaterialHue)*arg4;

    bool allowOverlap = arg5;

    return world->modifyTile(tilePosition, placeMod, allowOverlap);
  }
}

}
