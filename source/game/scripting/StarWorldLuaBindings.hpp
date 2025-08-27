#pragma once

#include "StarBiMap.hpp"
#include "StarRect.hpp"
#include "StarPoly.hpp"
#include "StarColor.hpp"
#include "StarDrawable.hpp"
#include "StarGameTypes.hpp"
#include "StarCollisionBlock.hpp"
#include "StarLua.hpp"
#include "StarPlatformerAStar.hpp"

namespace Star {

STAR_CLASS(World);
STAR_CLASS(WorldServer);
STAR_CLASS(WorldClient);
STAR_CLASS(Item);
STAR_CLASS(ScriptedEntity);

namespace LuaBindings {
  typedef function<Json(ScriptedEntityPtr const& entity, String const& functionName, JsonArray const& args)> CallEntityScriptFunction;

  LuaCallbacks makeWorldCallbacks(World* world);

  void addWorldDebugCallbacks(LuaCallbacks& callbacks);
  void addWorldEntityCallbacks(LuaCallbacks& callbacks, World* world);
  void addWorldEnvironmentCallbacks(LuaCallbacks& callbacks, World* world);

  namespace WorldCallbacks {
    float magnitude(World* world, Vec2F pos1, Maybe<Vec2F> pos2);
    Vec2F distance(World* world, Vec2F const& arg1, Vec2F const& arg2);
    bool polyContains(World* world, PolyF const& poly, Vec2F const& pos);
    LuaValue xwrap(World* world, LuaEngine& engine, LuaValue const& positionOrX);
    LuaValue nearestTo(World* world, LuaEngine& engine, Variant<Vec2F, float> const& sourcePositionOrX, Variant<Vec2F, float> const& targetPositionOrX);
    bool rectCollision(World* world, RectF const& arg1, Maybe<CollisionSet> const& arg2);
    bool pointTileCollision(World* world, Vec2F const& arg1, Maybe<CollisionSet> const& arg2);
    bool lineTileCollision(World* world, Vec2F const& arg1, Vec2F const& arg2, Maybe<CollisionSet> const& arg3);
    Maybe<pair<Vec2F, Vec2I>> lineTileCollisionPoint(World* world, Vec2F const& start, Vec2F const& end, Maybe<CollisionSet> const& maybeCollisionSet);
    bool rectTileCollision(World* world, RectF const& arg1, Maybe<CollisionSet> const& arg2);
    bool pointCollision(World* world, Vec2F const& point, Maybe<CollisionSet> const& collisionSet);
    LuaTupleReturn<Maybe<Vec2F>, Maybe<Vec2F>> lineCollision(World* world, Vec2F const& start, Vec2F const& end, Maybe<CollisionSet> const& maybeCollisionSet);
    bool polyCollision(World* world, PolyF const& arg1, Maybe<Vec2F> const& arg2, Maybe<CollisionSet> const& arg3);
    List<Vec2I> collisionBlocksAlongLine(World* world, Vec2F const& arg1, Vec2F const& arg2, Maybe<CollisionSet> const& arg3, Maybe<int> const& arg4);
    List<pair<Vec2I, LiquidLevel>> liquidAlongLine(World* world, Vec2F const& start, Vec2F const& end);
    Maybe<Vec2F> resolvePolyCollision(World* world, PolyF poly, Vec2F const& position, float maximumCorrection, Maybe<CollisionSet> const& collisionSet);
    bool tileIsOccupied(World* world, Vec2I const& arg1, Maybe<bool> const& arg2, Maybe<bool> const& arg3);
    bool placeObject(World* world, String const& arg1, Vec2I const& arg2, Maybe<int> const& arg3, Json const& arg4);
    Maybe<EntityId> spawnItem(World* world, Json const& itemType, Vec2F const& worldPosition, Maybe<size_t> const& inputCount, Json const& inputParameters, Maybe<Vec2F> const& initialVelocity, Maybe<float> const& intangibleTime);
    List<EntityId> spawnTreasure(World* world, Vec2F const& position, String const& pool, float level, Maybe<uint64_t> seed);
    Maybe<EntityId> spawnMonster(World* world, String const& arg1, Vec2F const& arg2, Maybe<JsonObject> const& arg3);
    Maybe<EntityId> spawnNpc(World* world, Vec2F const& arg1, String const& arg2, String const& arg3, float arg4, Maybe<uint64_t> arg5, Json const& arg6);
    Maybe<EntityId> spawnStagehand(World* world, Vec2F const& spawnPosition, String const& typeName, Json const& overrides);
    Maybe<EntityId> spawnProjectile(World* world, String const& arg1, Vec2F const& arg2, Maybe<EntityId> const& arg3, Maybe<Vec2F> const& arg4, bool arg5, Json const& arg6);
    Maybe<EntityId> spawnVehicle(World* world, String const& vehicleName, Vec2F const& pos, Json const& extraConfig);
    double time(World* world);
    uint64_t day(World* world);
    double timeOfDay(World* world);
    float dayLength(World* world);
    Json getProperty(World* world, String const& arg1, Json const& arg2);
    void setProperty(World* world, String const& arg1, Json const& arg2);
    Maybe<LiquidLevel> liquidAt(World* world, Variant<RectF, Vec2I> boundBoxOrPoint);
    float gravity(World* world, Vec2F const& arg1);
    bool spawnLiquid(World* world, Vec2F const& arg1, LiquidId arg2, float arg3);
    Maybe<LiquidLevel> destroyLiquid(World* world, Vec2F const& position);
    bool isTileProtected(World* world, Vec2F const& position);
    Maybe<PlatformerAStar::Path> findPlatformerPath(World* world, Vec2F const& start, Vec2F const& end, ActorMovementParameters actorMovementParameters, PlatformerAStar::Parameters searchParameters);
    PlatformerAStar::PathFinder platformerPathStart(World* world, Vec2F const& start, Vec2F const& end, ActorMovementParameters actorMovementParameters, PlatformerAStar::Parameters searchParameters);
  }

  namespace ClientWorldCallbacks {
    void resendEntity(WorldClient* world, EntityId arg1);
    RectI clientWindow(WorldClient* world);
  }

  namespace ServerWorldCallbacks {
    String id(WorldServer* world);
    bool breakObject(WorldServer* world, EntityId arg1, bool arg2);
    bool isVisibleToPlayer(WorldServer* world, RectF const& arg1);
    bool loadRegion(WorldServer* world, RectF const& arg1);
    bool regionActive(WorldServer* world, RectF const& arg1);
    void setTileProtection(WorldServer* world, DungeonId arg1, bool arg2);
    bool isPlayerModified(WorldServer* world, RectI const& region);
    Maybe<LiquidLevel> forceDestroyLiquid(WorldServer* world, Vec2F const& position);
    EntityId loadUniqueEntity(WorldServer* world, String const& uniqueId);
    void setUniqueId(WorldServer* world, EntityId entityId, Maybe<String> const& uniqueId);
    Json takeItemDrop(World* world, EntityId entityId, Maybe<EntityId> const& takenBy);
    void setPlayerStart(World* world, Vec2F const& playerStart, Maybe<bool> respawnInWorld);
    List<EntityId> players(World* world);
    LuaString fidelity(World* world, LuaEngine& engine);
    Maybe<LuaValue> callScriptContext(World* world, String const& contextName, String const& function, LuaVariadic<LuaValue> const& args);
    bool sendPacket(WorldServer* world, ConnectionId clientId, String const& packetType, Json const& packetData);
  }

  namespace WorldDebugCallbacks {
    void debugPoint(Vec2F const& arg1, Color const& arg2);
    void debugLine(Vec2F const& arg1, Vec2F const& arg2, Color const& arg3);
    void debugPoly(PolyF const& poly, Color const& color);
    void debugText(LuaEngine& engine, LuaVariadic<LuaValue> const& args);
  }

  namespace WorldEntityCallbacks {
    LuaTable entityQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options);
    LuaTable monsterQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options);
    LuaTable npcQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options);
    LuaTable objectQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options);
    LuaTable itemDropQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options);
    LuaTable playerQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options);
    LuaTable loungeableQuery(World* world, LuaEngine& engine, Vec2F const& pos1, LuaValue const& pos2, Maybe<LuaTable> options);
    LuaTable entityLineQuery(World* world, LuaEngine& engine, Vec2F const& pos1, Vec2F const& pos2, Maybe<LuaTable> options);
    LuaTable objectLineQuery(World* world, LuaEngine& engine, Vec2F const& pos1, Vec2F const& pos2, Maybe<LuaTable> options);
    LuaTable npcLineQuery(World* world, LuaEngine& engine, Vec2F const& pos1, Vec2F const& pos2, Maybe<LuaTable> options);
    bool entityExists(World* world, EntityId entityId);
    bool entityCanDamage(World* world, EntityId sourceId, EntityId targetId);
    Json entityDamageTeam(World* world, EntityId entityId);
    bool entityAggressive(World* world, EntityId entityId);
    Maybe<LuaString> entityType(World* world, LuaEngine& engine, EntityId entityId);
    Maybe<Vec2F> entityPosition(World* world, EntityId entityId);
    Maybe<Vec2F> entityVelocity(World* world, EntityId entityId);
    Maybe<RectF> entityMetaBoundBox(World* world, EntityId entityId);
    Maybe<uint64_t> entityCurrency(World* world, EntityId entityId, String const& currencyType);
    Maybe<uint64_t> entityHasCountOfItem(World* world, EntityId entityId, Json descriptor, Maybe<bool> exactMatch);
    Maybe<Vec2F> entityHealth(World* world, EntityId entityId);
    Maybe<String> entitySpecies(World* world, EntityId entityId);
    Maybe<String> entityGender(World* world, EntityId entityId);
    Maybe<String> entityName(World* world, EntityId entityId);
    Maybe<Json> entityNametag(World* world, EntityId entityId);
    Maybe<String> entityDescription(World* world, EntityId entityId, Maybe<String> const& species);
    LuaNullTermWrapper<Maybe<List<Drawable>>> entityPortrait(World* world, EntityId entityId, String const& portraitMode);
    Maybe<String> entityHandItem(World* world, EntityId entityId, String const& handName);
    Json entityHandItemDescriptor(World* world, EntityId entityId, String const& handName);
    LuaNullTermWrapper<Maybe<String>> entityUniqueId(World* world, EntityId entityId);
    Json getObjectParameter(World* world, EntityId entityId, String const& parameterName, Maybe<Json> const& defaultValue);
    Json getNpcScriptParameter(World* world, EntityId entityId, String const& parameterName, Maybe<Json> const& defaultValue);
    List<Vec2I> objectSpaces(World* world, EntityId entityId);
    Maybe<int> farmableStage(World* world, EntityId entityId);
    Maybe<int> containerSize(World* world, EntityId entityId);
    bool containerClose(World* world, EntityId entityId);
    bool containerOpen(World* world, EntityId entityId);
    Json containerItems(World* world, EntityId entityId);
    Json containerItemAt(World* world, EntityId entityId, size_t offset);
    Maybe<bool> containerConsume(World* world, EntityId entityId, Json const& items);
    Maybe<bool> containerConsumeAt(World* world, EntityId entityId, size_t offset, int count);
    Maybe<size_t> containerAvailable(World* world, EntityId entityId, Json const& items);
    Json containerTakeAll(World* world, EntityId entityId);
    Json containerTakeAt(World* world, EntityId entityId, size_t offset);
    Json containerTakeNumItemsAt(World* world, EntityId entityId, size_t offset, int const& count);
    Maybe<size_t> containerItemsCanFit(World* world, EntityId entityId, Json const& items);
    Json containerItemsFitWhere(World* world, EntityId entityId, Json const& items);
    Json containerAddItems(World* world, EntityId entityId, Json const& items);
    Json containerStackItems(World* world, EntityId entityId, Json const& items);
    Json containerPutItemsAt(World* world, EntityId entityId, Json const& items, size_t offset);
    Json containerSwapItems(World* world, EntityId entityId, Json const& items, size_t offset);
    Json containerSwapItemsNoCombine(World* world, EntityId entityId, Json const& items, size_t offset);
    Json containerItemApply(World* world, EntityId entityId, Json const& items, size_t offset);
    Maybe<LuaValue> callScriptedEntity(World* world, EntityId entityId, String const& function, LuaVariadic<LuaValue> const& args);
    RpcPromise<Vec2F> findUniqueEntity(World* world, String const& uniqueId);
    RpcPromise<Json> sendEntityMessage(World* world, LuaEngine& engine, LuaValue entityId, String const& message, LuaVariadic<Json> args);
    Maybe<List<EntityId>> loungingEntities(World* world, EntityId entityId, Maybe<size_t> anchorIndex);
    Maybe<bool> loungeableOccupied(World* world, EntityId entityId, Maybe<size_t> anchorIndex);
    Maybe<size_t> loungeableAnchorCount(World* world, EntityId entityId);
    bool isMonster(World* world, EntityId entityId, Maybe<bool> const& aggressive);
    Maybe<String> monsterType(World* world, EntityId entityId);
    Maybe<String> npcType(World* world, EntityId entityId);
    Maybe<String> stagehandType(World* world, EntityId entityId);
    bool isNpc(World* world, EntityId entityId, Maybe<int> const& damageTeam);
  }

  namespace WorldEnvironmentCallbacks {
    float lightLevel(World* world, Vec2F const& position);
    float windLevel(World* world, Vec2F const& position);
    bool breathable(World* world, Vec2F const& position);
    bool underground(World* world, Vec2F const& position);
    LuaValue material(World* world, LuaEngine& engine, Vec2F const& position, String const& layerName);
    LuaValue mod(World* world, LuaEngine& engine, Vec2F const& position, String const& layerName);
    float materialHueShift(World* world, Vec2F const& position, String const& layerName);
    float modHueShift(World* world, Vec2F const& position, String const& layerName);
    MaterialColorVariant materialColor(World* world, Vec2F const& position, String const& layerName);
    void setMaterialColor(World* world, Vec2F const& position, String const& layerName, MaterialColorVariant color);
    bool damageTiles(World* world, List<Vec2I> const& arg1, String const& arg2, Vec2F const& arg3, String const& arg4, float arg5, Maybe<unsigned> const& arg6, Maybe<EntityId> sourceEntity);
    bool damageTileArea(World* world, Vec2F center, float radius, String layer, Vec2F sourcePosition, String damageType, float damage, Maybe<unsigned> const& harvestLevel, Maybe<EntityId> sourceEntity);
    bool placeMaterial(World* world, Vec2I const& arg1, String const& arg2, String const& arg3, Maybe<int> const& arg4, bool arg5);
    bool replaceMaterials(World* world, List<Vec2I> const& tilePositions, String const& layer, String const& materialName, Maybe<int> const& hueShift, bool enableDrops);
    bool replaceMaterialArea(World* world, Vec2F center, float radius, String const& layer, String const& materialName, Maybe<int> const& hueShift, bool enableDrops);
    bool placeMod(World* world, Vec2I const& arg1, String const& arg2, String const& arg3, Maybe<int> const& arg4, bool arg5);
  }
}

}
