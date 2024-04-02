#pragma once

#include "StarWorld.hpp"
#include "StarWorldClientState.hpp"
#include "StarCollisionGenerator.hpp"
#include "StarSpawner.hpp"
#include "StarNetPackets.hpp"
#include "StarCellularLighting.hpp"
#include "StarCellularLiquid.hpp"
#include "StarWeather.hpp"
#include "StarInterpolationTracker.hpp"
#include "StarWorldStructure.hpp"
#include "StarLuaRoot.hpp"
#include "StarLuaComponents.hpp"
#include "StarWorldRenderData.hpp"
#include "StarWarping.hpp"
#include "StarRpcThreadPromise.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_CLASS(WorldTemplate);
STAR_CLASS(Sky);
STAR_STRUCT(SkyParameters);
STAR_CLASS(DamageManager);
STAR_CLASS(WireProcessor);
STAR_CLASS(EntityMap);
STAR_CLASS(WorldStorage);
STAR_CLASS(FallingBlocksAgent);
STAR_CLASS(DungeonDefinition);
STAR_CLASS(WorldServer);
STAR_CLASS(TileEntity);
STAR_CLASS(UniverseSettings);
STAR_CLASS(UniverseServer);

STAR_EXCEPTION(WorldServerException, StarException);

// Describes the amount of optional processing that a call to update() in
// WorldServer performs for things like liquid simulation, wiring, sector
// generation etc.
enum class WorldServerFidelity {
  Minimum,
  Low,
  Medium,
  High
};
extern EnumMap<WorldServerFidelity> const WorldServerFidelityNames;

class WorldServer : public World {
public:
  typedef LuaMessageHandlingComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>> ScriptComponent;
  typedef shared_ptr<ScriptComponent> ScriptComponentPtr;

  // Create a new world with the given template, writing new storage file.
  WorldServer(WorldTemplatePtr const& worldTemplate, IODevicePtr storage);
  // Synonym for WorldServer(make_shared<WorldTemplate>(size), storage);
  WorldServer(Vec2U const& size, IODevicePtr storage);
  // Load an existing world from the given storage files
  WorldServer(IODevicePtr const& storage);
  // Load an existing world from the given in-memory chunks
  WorldServer(WorldChunks const& chunks);
  // Load an existing world from an in-memory representation
  ~WorldServer();

  void setWorldId(String worldId);
  String const& worldId() const;

  void setUniverseSettings(UniverseSettingsPtr universeSettings);
  UniverseSettingsPtr universeSettings() const;

  void setReferenceClock(ClockPtr clock);

  void initLua(UniverseServer* universe);

  // Give this world a central structure.  If there is a previous central
  // structure it is removed first.  Returns the structure with transformed
  // coordinates.
  WorldStructure setCentralStructure(WorldStructure centralStructure);
  WorldStructure const& centralStructure() const;
  // If there is an active central structure, it is removed and all unmodified
  // objects and blocks associated with the structure are removed.
  void removeCentralStructure();

  void setPlayerStart(Vec2F const& startPosition, bool respawnInWorld = false);

  bool spawnTargetValid(SpawnTarget const& spawnTarget) const;

  // Returns false if the client id already exists, or the spawn target is
  // invalid.
  bool addClient(ConnectionId clientId, SpawnTarget const& spawnTarget, bool isLocal);

  // Removes client, sends the WorldStopPacket, and returns any pending packets
  // for that client
  List<PacketPtr> removeClient(ConnectionId clientId);

  List<ConnectionId> clientIds() const;
  bool hasClient(ConnectionId clientId) const;
  RectF clientWindow(ConnectionId clientId) const;
  // May return null if a Player is not available or if the client id is not
  // valid.
  PlayerPtr clientPlayer(ConnectionId clientId) const;

  List<EntityId> players() const;

  void handleIncomingPackets(ConnectionId clientId, List<PacketPtr> const& packets);
  List<PacketPtr> getOutgoingPackets(ConnectionId clientId);
  void sendPacket(ConnectionId clientId, PacketPtr const& packet);

  Maybe<Json> receiveMessage(ConnectionId fromConnection, String const& message, JsonArray const& args);

  void startFlyingSky(bool enterHyperspace, bool startInWarp);
  void stopFlyingSkyAt(SkyParameters const& destination);
  void setOrbitalSky(SkyParameters const& destination);

  // Defaults to Medium
  WorldServerFidelity fidelity() const;
  void setFidelity(WorldServerFidelity fidelity);

  bool shouldExpire();
  void setExpiryTime(float expiryTime);

  void update(float dt);

  ConnectionId connection() const override;
  WorldGeometry geometry() const override;
  uint64_t currentStep() const override;
  MaterialId material(Vec2I const& position, TileLayer layer) const override;
  MaterialHue materialHueShift(Vec2I const& position, TileLayer layer) const override;
  ModId mod(Vec2I const& position, TileLayer layer) const override;
  MaterialHue modHueShift(Vec2I const& position, TileLayer layer) const override;
  MaterialColorVariant colorVariant(Vec2I const& position, TileLayer layer) const override;
  LiquidLevel liquidLevel(Vec2I const& pos) const override;
  LiquidLevel liquidLevel(RectF const& region) const override;

  TileModificationList validTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap) const override;
  TileModificationList applyTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap) override;
  EntityPtr entity(EntityId entityId) const override;
  void addEntity(EntityPtr const& entity, EntityId entityId = NullEntityId) override;
  EntityPtr closestEntity(Vec2F const& center, float radius, EntityFilter selector = EntityFilter()) const override;
  void forAllEntities(EntityCallback entityCallback) const override;
  void forEachEntity(RectF const& boundBox, EntityCallback callback) const override;
  void forEachEntityLine(Vec2F const& begin, Vec2F const& end, EntityCallback callback) const override;
  void forEachEntityAtTile(Vec2I const& pos, EntityCallbackOf<TileEntity> entityCallback) const override;
  EntityPtr findEntity(RectF const& boundBox, EntityFilter entityFilter) const override;
  EntityPtr findEntityLine(Vec2F const& begin, Vec2F const& end, EntityFilter entityFilter) const override;
  EntityPtr findEntityAtTile(Vec2I const& pos, EntityFilterOf<TileEntity> entityFilter) const override;
  bool tileIsOccupied(Vec2I const& pos, TileLayer layer, bool includeEphemeral = false, bool checkCollision = false) const override;
  CollisionKind tileCollisionKind(Vec2I const& pos) const override;
  void forEachCollisionBlock(RectI const& region, function<void(CollisionBlock const&)> const& iterator) const override;
  bool isTileConnectable(Vec2I const& pos, TileLayer layer, bool tilesOnly = false) const override;
  bool pointTileCollision(Vec2F const& point, CollisionSet const& collisionSet = DefaultCollisionSet) const override;
  bool lineTileCollision(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet = DefaultCollisionSet) const override;
  Maybe<pair<Vec2F, Vec2I>> lineTileCollisionPoint(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet = DefaultCollisionSet) const override;
  List<Vec2I> collidingTilesAlongLine(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet = DefaultCollisionSet, int maxSize = -1, bool includeEdges = true) const override;
  bool rectTileCollision(RectI const& region, CollisionSet const& collisionSet = DefaultCollisionSet) const override;
  TileDamageResult damageTiles(List<Vec2I> const& pos, TileLayer layer, Vec2F const& sourcePosition, TileDamage const& tileDamage, Maybe<EntityId> sourceEntity = {}) override;
  InteractiveEntityPtr getInteractiveInRange(Vec2F const& targetPosition, Vec2F const& sourcePosition, float maxRange) const override;
  bool canReachEntity(Vec2F const& position, float radius, EntityId targetEntity, bool preferInteractive = true) const override;
  RpcPromise<InteractAction> interact(InteractRequest const& request) override;
  float gravity(Vec2F const& pos) const override;
  float windLevel(Vec2F const& pos) const override;
  float lightLevel(Vec2F const& pos) const override;
  bool breathable(Vec2F const& pos) const override;
  float threatLevel() const override;
  StringList environmentStatusEffects(Vec2F const& pos) const override;
  StringList weatherStatusEffects(Vec2F const& pos) const override;
  bool exposedToWeather(Vec2F const& pos) const override;
  bool isUnderground(Vec2F const& pos) const override;
  bool disableDeathDrops() const override;
  List<PhysicsForceRegion> forceRegions() const override;
  Json getProperty(String const& propertyName, Json const& def = Json()) const override;
  void setProperty(String const& propertyName, Json const& property) override;
  void timer(float delay, WorldAction worldAction) override;
  double epochTime() const override;
  uint32_t day() const override;
  float dayLength() const override;
  float timeOfDay() const override;
  LuaRootPtr luaRoot() override;
  RpcPromise<Vec2F> findUniqueEntity(String const& uniqueId) override;
  RpcPromise<Json> sendEntityMessage(Variant<EntityId, String> const& entity, String const& message, JsonArray const& args = {}) override;
  bool isTileProtected(Vec2I const& pos) const override;

  void setTileProtection(DungeonId dungeonId, bool isProtected);
  // used to globally, temporarily disable protection for certain operations
  void setTileProtectionEnabled(bool enabled);

  void setDungeonGravity(DungeonId dungeonId, Maybe<float> gravity);
  void setDungeonBreathable(DungeonId dungeonId, Maybe<bool> breathable);

  void setDungeonId(RectI const& tileRegion, DungeonId dungeonId);

  // Signal a region to load / generate, returns true if it is now fully loaded
  // and generated
  bool signalRegion(RectI const& region);
  // Immediately generate a given region
  void generateRegion(RectI const& region);
  // Returns true if a region is fully active without signaling it.
  bool regionActive(RectI const& region);

  ScriptComponentPtr scriptContext(String const& contextName);

  // Queues a microdungeon for placement
  RpcPromise<Vec2I> enqueuePlacement(List<BiomeItemDistribution> distributions, Maybe<DungeonId> id);

  ServerTile const& getServerTile(Vec2I const& position, bool withSignal = false);
  // Gets mutable pointer to server tile and marks it as needing updates to all
  // clients.
  ServerTile* modifyServerTile(Vec2I const& position, bool withSignal = false);

  EntityId loadUniqueEntity(String const& uniqueId);

  WorldTemplatePtr worldTemplate() const;
  SkyPtr sky() const;
  void modifyLiquid(Vec2I const& pos, LiquidId liquid, float quantity, bool additive = false);
  void setLiquid(Vec2I const& pos, LiquidId liquid, float level, float pressure);
  List<ItemDescriptor> destroyBlock(TileLayer layer, Vec2I const& pos, bool genItems, bool destroyModFirst);
  void removeEntity(EntityId entityId, bool andDie);

  void updateTileEntityTiles(TileEntityPtr const& object, bool removing = false, bool checkBreaks = true);

  bool isVisibleToPlayer(RectF const& region) const;
  void activateLiquidRegion(RectI const& region);
  void activateLiquidLocation(Vec2I const& location);

  // if blocks cascade, we'll need to do a break check across all tile entities
  // when the timer next ticks
  void requestGlobalBreakCheck();

  void setSpawningEnabled(bool spawningEnabled);

  // Write all active sectors to disk without unloading them
  void sync();
  // Copy full world to in memory representation
  WorldChunks readChunks();

  bool forceModifyTile(Vec2I const& pos, TileModification const& modification, bool allowEntityOverlap);
  TileModificationList forceApplyTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap);

  DungeonId dungeonId(Vec2I const& pos) const;

  bool isPlayerModified(RectI const& region) const;

  ItemDescriptor collectLiquid(List<Vec2I> const& tilePositions, LiquidId liquidId);

  bool placeDungeon(String const& dungeonName, Vec2I const& position, Maybe<DungeonId> dungeonId = {}, bool forcePlacement = true);

  void addBiomeRegion(Vec2I const& position, String const& biomeName, String const& subBlockSelector, int width);
  void expandBiomeRegion(Vec2I const& position, int newWidth);

  // queue generation of the sectors that will be needed to insert or
  // expand a biome region in order to spread processing over time
  bool pregenerateAddBiome(Vec2I const& position, int width);
  bool pregenerateExpandBiome(Vec2I const& position, int newWidth);

  // set the biome at the given position to be the environment biome for the layer
  void setLayerEnvironmentBiome(Vec2I const& position);

  // for terrestrial worlds only. updates the planet type in the celestial as well as local
  // world parameters along with the primary biome and the weather pool
  void setPlanetType(String const& planetType, String const& primaryBiomeName);

  // used to notify the universe server that the celestial planet type has changed
  Maybe<pair<String, String>> pullNewPlanetType();

private:
  struct ClientInfo {
    ClientInfo(ConnectionId clientId, InterpolationTracker const trackerInit);

    List<RectI> monitoringRegions(EntityMapPtr const& entityMap) const;

    bool needsDamageNotification(RemoteDamageNotification const& rdn) const;

    ConnectionId clientId;
    uint64_t skyNetVersion;
    uint64_t weatherNetVersion;
    WorldClientState clientState;
    bool pendingForward;
    bool started;
    bool local;

    List<PacketPtr> outgoingPackets;

    // All slave entities for which the player should be knowledgable about.
    HashMap<EntityId, uint64_t> clientSlavesNetVersion;

    // Batch send tile updates
    HashSet<Vec2I> pendingTileUpdates;
    HashSet<Vec2I> pendingLiquidUpdates;
    HashSet<pair<Vec2I, TileLayer>> pendingTileDamageUpdates;
    HashSet<ServerTileSectorArray::Sector> pendingSectors;
    HashSet<ServerTileSectorArray::Sector> activeSectors;

    InterpolationTracker interpolationTracker;
  };

  struct TileEntitySpaces {
    List<MaterialSpace> materials;
    List<Vec2I> roots;
  };

  typedef function<ServerTile const& (Vec2I)> ServerTileGetter;

  void init(bool firstTime);

  // Returns nothing if the processing defined by the given configuration entry
  // should not run this tick, if it should run this tick, returns the number
  // of ticks since the last run.
  Maybe<unsigned> shouldRunThisStep(String const& timingConfiguration);

  TileModificationList doApplyTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap, bool ignoreTileProtection = false);

  // Queues pending (step based) updates to the given player
  void queueUpdatePackets(ConnectionId clientId, bool sendRemoteUpdates);
  void updateDamage(float dt);

  void updateDamagedBlocks(float dt);

  // Check for any newly broken entities in this rect
  void checkEntityBreaks(RectF const& rect);
  // Push modified tile data to each client.
  void queueTileUpdates(Vec2I const& pos);
  void queueTileDamageUpdates(Vec2I const& pos, TileLayer layer);
  void writeNetTile(Vec2I const& pos, NetTile& netTile) const;

  void dirtyCollision(RectI const& region);
  void freshenCollision(RectI const& region);

  Vec2F findPlayerStart(Maybe<Vec2F> firstTry = {});
  Vec2F findPlayerSpaceStart(float targetX);
  void readMetadata();
  void writeMetadata();
  float gravityFromTile(ServerTile const& tile) const;

  bool isFloatingDungeonWorld() const;

  void setupForceRegions();

  Json m_serverConfig;

  WorldTemplatePtr m_worldTemplate;
  WorldStructure m_centralStructure;
  Vec2F m_playerStart;
  bool m_adjustPlayerStart;
  bool m_respawnInWorld;
  JsonObject m_worldProperties;

  Maybe<pair<String, String>> m_newPlanetType;

  UniverseSettingsPtr m_universeSettings;

  EntityMapPtr m_entityMap;
  ServerTileSectorArrayPtr m_tileArray;
  ServerTileGetter m_tileGetterFunction;
  WorldStoragePtr m_worldStorage;
  WorldServerFidelity m_fidelity;
  Json m_fidelityConfig;

  HashSet<Vec2I> m_damagedBlocks;
  DamageManagerPtr m_damageManager;
  WireProcessorPtr m_wireProcessor;
  LuaRootPtr m_luaRoot;

  StringMap<ScriptComponentPtr> m_scriptContexts;

  WorldGeometry m_geometry;
  double m_currentTime;
  uint64_t m_currentStep;
  mutable CellularLightIntensityCalculator m_lightIntensityCalculator;
  SkyPtr m_sky;

  ServerWeather m_weather;

  CollisionGenerator m_collisionGenerator;
  List<CollisionBlock> m_workingCollisionBlocks;

  HashMap<pair<EntityId, uint64_t>, pair<ByteArray, uint64_t>> m_netStateCache;
  OrderedHashMap<ConnectionId, shared_ptr<ClientInfo>> m_clientInfo;

  GameTimer m_entityUpdateTimer;
  GameTimer m_tileEntityBreakCheckTimer;

  shared_ptr<LiquidCellEngine<LiquidId>> m_liquidEngine;
  FallingBlocksAgentPtr m_fallingBlocksAgent;
  Spawner m_spawner;

  // Keep track of material spaces and roots registered by tile entities to
  // make sure we can cleanly remove them when they change or when the entity
  // is removed / uninitialized
  HashMap<EntityId, TileEntitySpaces> m_tileEntitySpaces;

  List<pair<float, WorldAction>> m_timers;

  bool m_needsGlobalBreakCheck;

  bool m_generatingDungeon;
  HashMap<DungeonId, float> m_dungeonIdGravity;
  HashMap<DungeonId, bool> m_dungeonIdBreathable;
  Set<DungeonId> m_protectedDungeonIds;
  bool m_tileProtectionEnabled;

  HashMap<Uuid, pair<ConnectionId, MVariant<ConnectionId, RpcPromiseKeeper<Json>>>> m_entityMessageResponses;

  List<PhysicsForceRegion> m_forceRegions;

  String m_worldId;

  GameTimer m_expiryTimer;
};

}
