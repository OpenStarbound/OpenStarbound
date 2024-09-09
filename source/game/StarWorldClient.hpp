#pragma once

#include "StarWorldClientState.hpp"
#include "StarNetPackets.hpp"
#include "StarWorldRenderData.hpp"
#include "StarAmbient.hpp"
#include "StarCellularLighting.hpp"
#include "StarWeather.hpp"
#include "StarInterpolationTracker.hpp"
#include "StarWorldStructure.hpp"
#include "StarChatAction.hpp"
#include "StarWiring.hpp"
#include "StarEntityRendering.hpp"
#include "StarWorld.hpp"
#include "StarGameTimers.hpp"
#include "StarLuaRoot.hpp"
#include "StarTickRateMonitor.hpp"

namespace Star {

STAR_STRUCT(Biome);
STAR_CLASS(WorldTemplate);
STAR_CLASS(Sky);
STAR_CLASS(Parallax);
STAR_CLASS(LuaRoot);
STAR_CLASS(DamageManager);
STAR_CLASS(EntityMap);
STAR_CLASS(ParticleManager);
STAR_CLASS(WorldClient);
STAR_CLASS(Player);
STAR_CLASS(Item);
STAR_CLASS(CelestialLog);
STAR_CLASS(ClientContext);
STAR_CLASS(PlayerStorage);
STAR_STRUCT(OverheadBar);

STAR_EXCEPTION(WorldClientException, StarException);

class WorldClient : public World {
public:
  WorldClient(PlayerPtr mainPlayer);
  ~WorldClient();

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

  // Is this WorldClient properly initialized in a world
  bool inWorld() const;

  bool inSpace() const;
  bool flying() const;

  bool mainPlayerDead() const;
  void reviveMainPlayer();
  bool respawnInWorld() const;

  bool setRespawnInWorld(bool value);

  void removeEntity(EntityId entityId, bool andDie);

  WorldTemplateConstPtr currentTemplate() const;
  SkyConstPtr currentSky() const;

  void dimWorld();
  bool interactiveHighlightMode() const;
  void setInteractiveHighlightMode(bool enabled);
  void setParallax(ParallaxPtr newParallax);
  void overrideGravity(float gravity);
  void resetGravity();

  // Disable normal client-side lighting algorithm, everything full brightness.
  bool fullBright() const;
  void setFullBright(bool fullBright);
  // Disable asynchronous client-side lighting algorithm, run on main thread.
  bool asyncLighting() const;
  void setAsyncLighting(bool asyncLighting);
  // Spatial log generated collision geometry.
  bool collisionDebug() const;
  void setCollisionDebug(bool collisionDebug);

  void handleIncomingPackets(List<PacketPtr> const& packets);
  List<PacketPtr> getOutgoingPackets();
  
  // Sets default callbacks in the LuaRoot.
  void setLuaCallbacks(String const& groupName, LuaCallbacks const& callbacks);

  // Set the rendering window for this client.
  void setClientWindow(RectI window);
  // Sets the client window around the position of the main player.
  void centerClientWindowOnPlayer(Vec2U const& windowSize);
  void centerClientWindowOnPlayer();
  RectI clientWindow() const;
  WorldClientState& clientState();

  void update(float dt);
  // borderTiles here should extend the client window for border tile
  // calculations.  It is not necessary on the light array.
  void render(WorldRenderData& renderData, unsigned borderTiles);
  List<AudioInstancePtr> pullPendingAudio();
  List<AudioInstancePtr> pullPendingMusic();

  bool playerCanReachEntity(EntityId entityId, bool preferInteractive = true) const;

  void disconnectAllWires(Vec2I wireEntityPosition, WireNode const& node);
  void connectWire(WireConnection const& output, WireConnection const& input);

  // Functions for sending broadcast messages to other players that can receive them,
  // on completely vanilla servers by smuggling it through a DamageNotification.
  // It's cursed as fuck, but it works.
  bool sendSecretBroadcast(StringView broadcast, bool raw = false, bool compress = true);
  bool handleSecretBroadcast(PlayerPtr player, StringView broadcast);

  List<ChatAction> pullPendingChatActions();

  WorldStructure const& centralStructure() const;

  DungeonId dungeonId(Vec2I const& pos) const;

  void collectLiquid(List<Vec2I> const& tilePositions, LiquidId liquidId);

  bool waitForLighting(WorldRenderData* renderData = nullptr);

  typedef std::function<bool(PlayerPtr, StringView)> BroadcastCallback;
  BroadcastCallback& broadcastCallback();



private:
  static const float DropDist;

  struct ClientRenderCallback : RenderCallback {
    void addDrawable(Drawable drawable, EntityRenderLayer renderLayer) override;
    void addLightSource(LightSource lightSource) override;
    void addParticle(Particle particle) override;
    void addAudio(AudioInstancePtr audio) override;
    void addTilePreview(PreviewTile preview) override;
    void addOverheadBar(OverheadBar bar) override;

    Map<EntityRenderLayer, List<Drawable>> drawables;
    List<LightSource> lightSources;
    List<Particle> particles;
    List<AudioInstancePtr> audios;
    List<PreviewTile> previewTiles;
    List<OverheadBar> overheadBars;
  };

  struct DamageNumber {
    float amount;
    Vec2F position;
    double timestamp;
  };

  struct DamageNumberKey {
    String damageNumberParticleKind;
    EntityId sourceEntityId;
    EntityId targetEntityId;

    bool operator<(DamageNumberKey const& other) const;
  };

  typedef function<ClientTile const& (Vec2I)> ClientTileGetter;

  void lightingTileGather();
  void lightingCalc();
  void lightingMain();

  void initWorld(WorldStartPacket const& packet);
  void clearWorld();
  void tryGiveMainPlayerItem(ItemPtr item, bool silent = false);

  void notifyEntityCreate(EntityPtr const& entity);

  // Queues pending (step based) updates to server,
  void queueUpdatePackets(bool sendEntityUpdates);
  void handleDamageNotifications();

  void sparkDamagedBlocks();

  Vec2I environmentBiomeTrackPosition() const;
  AmbientNoisesDescriptionPtr currentAmbientNoises() const;
  WeatherNoisesDescriptionPtr currentWeatherNoises() const;
  AmbientNoisesDescriptionPtr currentMusicTrack() const;
  AmbientNoisesDescriptionPtr currentAltMusicTrack() const;

  void playAltMusic(StringList const& newTracks, float fadeTime);
  void stopAltMusic(float fadeTime);

  BiomeConstPtr mainEnvironmentBiome() const;

  // Populates foregroundTransparent / backgroundTransparent flag on ClientTile
  // based on transparency rules.
  bool readNetTile(Vec2I const& pos, NetTile const& netTile, bool updateCollision = true);
  void dirtyCollision(RectI const& region);
  void freshenCollision(RectI const& region);
  void renderCollisionDebug();

  void informTilePrediction(Vec2I const& pos, TileModification const& modification);

  void setTileProtection(DungeonId dungeonId, bool isProtected);

  void setupForceRegions();

  Json m_clientConfig;
  WorldTemplatePtr m_worldTemplate;
  WorldStructure m_centralStructure;
  Vec2F m_playerStart;
  bool m_respawnInWorld;
  JsonObject m_worldProperties;

  EntityMapPtr m_entityMap;
  ClientTileSectorArrayPtr m_tileArray;
  ClientTileGetter m_tileGetterFunction;
  DamageManagerPtr m_damageManager;
  LuaRootPtr m_luaRoot;

  WorldGeometry m_geometry;
  uint64_t m_currentStep;
  double m_currentTime;
  bool m_fullBright;
  bool m_asyncLighting;
  CellularLightingCalculator m_lightingCalculator;
  mutable CellularLightIntensityCalculator m_lightIntensityCalculator;
  ThreadFunction<void> m_lightingThread;
  
  Mutex m_lightingMutex;
  ConditionVariable m_lightingCond;
  atomic<bool> m_stopLightingThread;

  Mutex m_lightMapPrepMutex;
  Mutex m_lightMapMutex;

  Lightmap m_pendingLightMap;
  Lightmap m_lightMap;
  List<LightSource> m_pendingLights;
  List<std::pair<Vec2F, Vec3F>> m_pendingParticleLights;
  RectI m_pendingLightRange;
  Vec2I m_lightMinPosition;
  List<PreviewTile> m_previewTiles;

  SkyPtr m_sky;

  CollisionGenerator m_collisionGenerator;

  WorldClientState m_clientState;
  Maybe<ConnectionId> m_clientId;

  PlayerPtr m_mainPlayer;

  bool m_collisionDebug;

  // Client side entity updates are not done until m_inWorld is true, which is
  // set to true after we have entered a world *and* the first batch of updates
  // are received.
  bool m_inWorld;

  GameTimer m_worldDimTimer;
  float m_worldDimLevel;
  Vec3B m_worldDimColor;

  bool m_interactiveHighlightMode;

  GameTimer m_parallaxFadeTimer;
  ParallaxPtr m_currentParallax;
  ParallaxPtr m_nextParallax;

  Maybe<float> m_overrideGravity;

  ClientWeather m_weather;
  ParticleManagerPtr m_particles;

  List<AudioInstancePtr> m_samples;
  List<AudioInstancePtr> m_music;

  HashMap<EntityId, uint64_t> m_masterEntitiesNetVersion;

  InterpolationTracker m_interpolationTracker;
  GameTimer m_entityUpdateTimer;

  List<PacketPtr> m_outgoingPackets;
  Maybe<int64_t> m_pingTime;
  int64_t m_latency;

  Set<EntityId> m_requestedDrops;

  Particle m_blockDamageParticle;
  Particle m_blockDamageParticleVariance;
  float m_blockDamageParticleProbability;

  Particle m_blockDingParticle;
  Particle m_blockDingParticleVariance;
  float m_blockDingParticleProbability;

  HashSet<Vec2I> m_damagedBlocks;

  AmbientManager m_ambientSounds;
  AmbientManager m_musicTrack;
  AmbientManager m_altMusicTrack;

  List<pair<float, WorldAction>> m_timers;

  Map<DamageNumberKey, DamageNumber> m_damageNumbers;
  float m_damageNotificationBatchDuration;

  AudioInstancePtr m_spaceSound;
  String m_activeSpaceSound;

  AmbientNoisesDescriptionPtr m_altMusicTrackDescription;
  bool m_altMusicActive;

  int m_modifiedTilePredictionTimeout;
  HashMap<Vec2I, PredictedTile> m_predictedTiles;
  HashSet<EntityId> m_startupHiddenEntities;

  HashMap<DungeonId, float> m_dungeonIdGravity;
  HashMap<DungeonId, bool> m_dungeonIdBreathable;
  Set<DungeonId> m_protectedDungeonIds;

  HashMap<String, List<RpcPromiseKeeper<Vec2F>>> m_findUniqueEntityResponses;
  HashMap<Uuid, RpcPromiseKeeper<Json>> m_entityMessageResponses;
  HashMap<Uuid, RpcPromiseKeeper<InteractAction>> m_entityInteractionResponses;

  List<PhysicsForceRegion> m_forceRegions;

  BroadcastCallback m_broadcastCallback;
};

}
