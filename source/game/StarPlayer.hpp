#pragma once

#include "StarUuid.hpp"
#include "StarNetElementSystem.hpp"
#include "StarItemDescriptor.hpp"
#include "StarHumanoid.hpp"
#include "StarToolUserEntity.hpp"
#include "StarLoungingEntities.hpp"
#include "StarChattyEntity.hpp"
#include "StarEmoteEntity.hpp"
#include "StarDamageBarEntity.hpp"
#include "StarNametagEntity.hpp"
#include "StarPortraitEntity.hpp"
#include "StarInspectableEntity.hpp"
#include "StarInventoryTypes.hpp"
#include "StarActorMovementController.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarAiTypes.hpp"
#include "StarItemBag.hpp"
#include "StarArmorWearer.hpp"
#include "StarEntityRendering.hpp"
#include "StarToolUser.hpp"
#include "StarPlayerTypes.hpp"
#include "StarRadioMessageDatabase.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaActorMovementComponent.hpp"

namespace Star {

STAR_STRUCT(PlayerConfig);
STAR_CLASS(Songbook);
STAR_CLASS(WireConnector);
STAR_CLASS(PlayerInventory);
STAR_CLASS(PlayerBlueprints);
STAR_CLASS(PlayerTech);
STAR_CLASS(PlayerCompanions);
STAR_CLASS(PlayerDeployment);
STAR_CLASS(PlayerLog);
STAR_CLASS(TechController);
STAR_CLASS(ClientContext);
STAR_CLASS(Statistics);
STAR_CLASS(StatusController);
STAR_CLASS(PlayerCodexes);
STAR_CLASS(QuestManager);
STAR_CLASS(InteractiveEntity);
STAR_CLASS(PlayerUniverseMap);
STAR_CLASS(UniverseClient);

STAR_CLASS(Player);

class Player :
  public virtual ToolUserEntity,
  public virtual LoungingEntity,
  public virtual ChattyEntity,
  public virtual InspectableEntity,
  public virtual DamageBarEntity,
  public virtual PortraitEntity,
  public virtual NametagEntity,
  public virtual PhysicsEntity,
  public virtual EmoteEntity {

public:
  enum class State {
    Idle,
    Walk,
    Run,
    Jump,
    Fall,
    Swim,
    SwimIdle,
    TeleportIn,
    TeleportOut,
    Crouch,
    Lounge
  };
  static EnumMap<State> const StateNames;

  Player(PlayerConfigPtr config, Uuid uuid = Uuid());
  Player(PlayerConfigPtr config, ByteArray const& netStore, NetCompatibilityRules rules = {});
  Player(PlayerConfigPtr config, Json const& diskStore);

  void diskLoad(Json const& diskStore);

  ClientContextPtr clientContext() const;
  void setClientContext(ClientContextPtr clientContext);

  StatisticsPtr statistics() const;
  void setStatistics(StatisticsPtr statistics);

  void setUniverseClient(UniverseClient* universeClient);
  UniverseClient* universeClient() const;

  QuestManagerPtr questManager() const;

  Json diskStore();
  ByteArray netStore(NetCompatibilityRules rules = {});

  EntityType entityType() const override;
  ClientEntityMode clientEntityMode() const override;

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  Vec2F position() const override;
  Vec2F velocity() const override;

  Vec2F mouthPosition() const override;
  Vec2F mouthPosition(bool ignoreAdjustments) const override;
  Vec2F throwItemPosition() const;
  Vec2F interactPosition() const;
  Vec2F mouthOffset(bool ignoreAdjustments = true) const;
  Vec2F feetOffset() const;
  Vec2F headArmorOffset() const;
  Vec2F chestArmorOffset() const;
  Vec2F legsArmorOffset() const;
  Vec2F backArmorOffset() const;

  // relative to current position
  RectF metaBoundBox() const override;

  // relative to current position
  RectF collisionArea() const override;

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0, NetCompatibilityRules rules = {}) override;
  void readNetState(ByteArray data, float interpolationStep = 0.0f, NetCompatibilityRules rules = {}) override;

  void enableInterpolation(float extrapolationHint = 0.0f) override;
  void disableInterpolation() override;

  virtual Maybe<HitType> queryHit(DamageSource const& source) const override;
  Maybe<PolyF> hitPoly() const override;

  List<DamageNotification> applyDamage(DamageRequest const& damage) override;
  List<DamageNotification> selfDamageNotifications() override;

  void hitOther(EntityId targetEntityId, DamageRequest const& damageRequest) override;
  void damagedOther(DamageNotification const& damage) override;

  List<DamageSource> damageSources() const override;

  bool shouldDestroy() const override;
  void destroy(RenderCallback* renderCallback) override;

  Maybe<EntityAnchorState> loungingIn() const override;
  bool lounge(EntityId loungeableEntityId, size_t anchorIndex);
  void stopLounging();

  void revive(Vec2F const& footPosition);

  List<Drawable> portrait(PortraitMode mode) const override;
  bool underwater() const;

  void setShifting(bool shifting);
  void special(int specialKey);

  void setMoveVector(Vec2F const& vec);
  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();
  void jump();

  void dropItem();

  float toolRadius() const;
  float interactRadius() const override;
  void setInteractRadius(float interactRadius);
  List<InteractAction> pullInteractActions();

  uint64_t currency(String const& currencyType) const;

  float health() const override;
  float maxHealth() const override;
  DamageBarType damageBar() const override;
  float healthPercentage() const;

  float energy() const override;
  float maxEnergy() const;
  float energyPercentage() const;

  float energyRegenBlockPercent() const;

  bool energyLocked() const override;
  bool fullEnergy() const override;
  bool consumeEnergy(float energy) override;

  float foodPercentage() const;

  float breath() const;
  float maxBreath() const;

  float protection() const;

  bool forceNude() const;

  String description() const override;
  void setDescription(String const& description);

  List<LightSource> lightSources() const override;

  Direction walkingDirection() const override;
  Direction facingDirection() const override;

  Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args = {}) override;

  void update(float dt, uint64_t currentStep) override;

  void render(RenderCallback* renderCallback) override;

  void renderLightSources(RenderCallback* renderCallback) override;

  Json getGenericProperty(String const& name, Json const& defaultValue = Json()) const;
  void setGenericProperty(String const& name, Json const& value);

  PlayerInventoryPtr inventory() const;
  // Returns the number of items from this stack that could be
  // picked up from the world, using inventory tab filtering
  uint64_t itemsCanHold(ItemPtr const& items) const;
  // Adds items to the inventory, returning the overflow.
  // The items parameter is invalid after use.
  ItemPtr pickupItems(ItemPtr const& items, bool silent = false);
  // Pick up all of the given items as possible, dropping the overflow.
  // The item parameter is invalid after use.
  void giveItem(ItemPtr const& item);

  void triggerPickupEvents(ItemPtr const& item);

  ItemPtr essentialItem(EssentialItem essentialItem) const;
  bool hasItem(ItemDescriptor const& descriptor, bool exactMatch = false) const;
  uint64_t hasCountOfItem(ItemDescriptor const& descriptor, bool exactMatch = false) const;
  // altough multiple entries may match, they might have different
  // serializations
  ItemDescriptor takeItem(ItemDescriptor const& descriptor, bool consumePartial = false, bool exactMatch = false);
  void giveItem(ItemDescriptor const& descriptor);

  // Clear the item swap slot.
  void clearSwap();

  void refreshArmor();
  void refreshItems();

  // Refresh worn equipment from the inventory
  void refreshEquipment();

  PlayerBlueprintsPtr blueprints() const;
  bool addBlueprint(ItemDescriptor const& descriptor, bool showFailure = false);
  bool blueprintKnown(ItemDescriptor const& descriptor) const;

  bool addCollectable(String const& collectionName, String const& collectableName);

  PlayerUniverseMapPtr universeMap() const;

  PlayerCodexesPtr codexes() const;

  PlayerTechPtr techs() const;
  void overrideTech(Maybe<StringList> const& techModules);
  bool techOverridden() const;

  PlayerCompanionsPtr companions() const;

  PlayerLogPtr log() const;

  InteractiveEntityPtr bestInteractionEntity(bool includeNearby);
  void interactWithEntity(InteractiveEntityPtr entity);

  // Aim this player's target at the given world position.
  void aim(Vec2F const& position);
  Vec2F aimPosition() const override;

  Vec2F armPosition(ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset = {}) const override;
  Vec2F handOffset(ToolHand hand, Direction facingDirection) const override;

  Vec2F handPosition(ToolHand hand, Vec2F const& handOffset = {}) const override;
  ItemPtr handItem(ToolHand hand) const override;

  Vec2F armAdjustment() const override;

  void setCameraFocusEntity(Maybe<EntityId> const& cameraFocusEntity) override;

  void playEmote(HumanoidEmote emote) override;

  bool canUseTool() const;

  // "Fires" whatever is in the primary (left) item slot, or the primary fire
  // of the 2H item, at whatever the current aim position is.  Will auto-repeat
  // depending on the item auto repeat setting.
  void beginPrimaryFire();
  // "Fires" whatever is in the alternate (right) item slot, or the alt fire of
  // the 2H item, at whatever the current aim position is.  Will auto-repeat
  // depending on the item auto repeat setting.
  void beginAltFire();

  void endPrimaryFire();
  void endAltFire();

  // Triggered whenever the use key is pressed
  void beginTrigger();
  void endTrigger();

  ItemPtr primaryHandItem() const;
  ItemPtr altHandItem() const;

  Uuid uuid() const;

  PlayerMode modeType() const;
  void setModeType(PlayerMode mode);
  PlayerModeConfig modeConfig() const;

  ShipUpgrades shipUpgrades();
  void setShipUpgrades(ShipUpgrades shipUpgrades);
  void applyShipUpgrades(Json const& upgrades);

  String name() const override;
  void setName(String const& name);

  Maybe<String> statusText() const override;
  bool displayNametag() const override;
  Vec3B nametagColor() const override;
  Vec2F nametagOrigin() const override;

  void updateIdentity();

  void setBodyDirectives(String const& directives);
  void setEmoteDirectives(String const& directives);

  void setHairGroup(String const& group);
  void setHairType(String const& type);
  void setHairDirectives(String const& directives);

  void setFacialHairGroup(String const& group);
  void setFacialHairType(String const& type);
  void setFacialHairDirectives(String const& directives);

  void setFacialMaskGroup(String const& group);
  void setFacialMaskType(String const& type);
  void setFacialMaskDirectives(String const& directives);

  void setHair      (String const& group, String const& type, String const& directives);
  void setFacialHair(String const& group, String const& type, String const& directives);
  void setFacialMask(String const& group, String const& type, String const& directives);

  String species() const override;
  void setSpecies(String const& species);
  Gender gender() const;
  void setGender(Gender const& gender);
  void setPersonality(Personality const& personality);
  void setImagePath(Maybe<String> const& imagePath);

  HumanoidPtr humanoid();
  HumanoidIdentity const& identity() const;

  void setIdentity(HumanoidIdentity identity);

  void setAdmin(bool isAdmin);
  bool isAdmin() const override;

  bool inToolRange() const override;
  bool inToolRange(Vec2F const& aimPos) const override;
  bool inInteractionRange() const;
  bool inInteractionRange(Vec2F aimPos) const;

  void addParticles(List<Particle> const& particles) override;
  void addSound(String const& sound, float volume = 1.0f, float pitch = 1.0f) override;

  bool wireToolInUse() const;
  void setWireConnector(WireConnector* wireConnector) const;

  void addEphemeralStatusEffects(List<EphemeralStatusEffect> const& statusEffects) override;
  ActiveUniqueStatusEffectSummary activeUniqueStatusEffectSummary() const override;

  float powerMultiplier() const override;

  bool isDead() const;
  void kill();

  void setFavoriteColor(Color color);
  Color favoriteColor() const override;

  // Starts the teleport animation sequence, locking player movement and
  // preventing some update code
  void teleportOut(String const& animationType = "default", bool deploy = false);
  void teleportIn();
  void teleportAbort();

  bool isTeleporting() const;
  bool isTeleportingOut() const;
  bool canDeploy();
  void deployAbort(String const& animationType = "default");
  bool isDeploying() const;
  bool isDeployed() const;

  void setBusyState(PlayerBusyState busyState);

  // A hard move to a specified location
  void moveTo(Vec2F const& footPosition);

  List<String> pullQueuedMessages();
  List<ItemPtr> pullQueuedItemDrops();

  void queueUIMessage(String const& message) override;
  void queueItemPickupMessage(ItemPtr const& item);

  void addChatMessage(String const& message, Json const& config = {});
  void addEmote(HumanoidEmote const& emote, Maybe<float> emoteCooldown = {});
  pair<HumanoidEmote, float> currentEmote() const;

  State currentState() const;

  List<ChatAction> pullPendingChatActions() override;

  Maybe<String> inspectionLogName() const override;
  Maybe<String> inspectionDescription(String const& species) const override;

  float beamGunRadius() const override;

  bool instrumentPlaying() override;
  void instrumentEquipped(String const& instrumentKind) override;
  void interact(InteractAction const& action) override;
  void addEffectEmitters(StringSet const& emitters) override;
  void requestEmote(String const& emote) override;

  ActorMovementController* movementController() override;
  StatusController* statusController() override;

  List<PhysicsForceRegion> forceRegions() const override;

  StatusControllerPtr statusControllerPtr();
  ActorMovementControllerPtr movementControllerPtr();

  PlayerConfigPtr config();

  SongbookPtr songbook() const;

  void finalizeCreation();

  float timeSinceLastGaveDamage() const;
  EntityId lastDamagedTarget() const;

  bool invisible() const;

  void animatePortrait(float dt);

  bool isOutside();

  void dropSelectedItems(function<bool(ItemPtr)> filter);
  void dropEverything();

  bool isPermaDead() const;

  bool interruptRadioMessage();
  Maybe<RadioMessage> pullPendingRadioMessage();
  void queueRadioMessage(Json const& messageConfig, float delay = 0);
  void queueRadioMessage(RadioMessage message);

  // If a cinematic should play, returns it and clears it.  May stop cinematics
  // by returning a null Json.
  Maybe<Json> pullPendingCinematic();
  void setPendingCinematic(Json const& cinematic, bool unique = false);

  void setInCinematic(bool inCinematic);

  Maybe<pair<Maybe<StringList>, float>> pullPendingAltMusic();

  Maybe<PlayerWarpRequest> pullPendingWarp();
  void setPendingWarp(String const& action, Maybe<String> const& animation = {}, bool deploy = false);

  Maybe<pair<Json, RpcPromiseKeeper<Json>>> pullPendingConfirmation();
  void queueConfirmation(Json const& dialogConfig, RpcPromiseKeeper<Json> const& resultPromise);

  AiState const& aiState() const;
  AiState& aiState();

  // In inspection mode, scannable, scanned, and interesting objects will be
  // rendered with special highlighting.
  bool inspecting() const;

  // Will return the highlight effect to give an inspectable entity when inspecting
  EntityHighlightEffect inspectionHighlight(InspectableEntityPtr const& inspectableEntity) const;

  Vec2F cameraPosition();

  using Entity::setTeam;

  NetworkedAnimatorPtr effectsAnimator();

  // We need to store ephemeral/large/always-changing networked properties that other clients can read. Candidates:
  // genericProperties:
  //   Non-starter, is not networked.
  // statusProperties:
  //   Nope! Changes to the status properties aren't networked efficiently - one change resends the whole map.
  //   We can't fix that because it would break compatibility with vanilla servers.
  // effectsAnimator's globalTags:
  //   Cursed, but viable.
  //   Efficient networking due to using a NetElementMapWrapper.
  //   Unfortunately values are Strings, so to work with Json we need to serialize/deserialize. Whatever.
  //   Additionally, this is compatible with vanilla networking.
  // I call this a 'secret property'.

  // If the secret property exists as a serialized Json string, returns a view to it without deserializing.
  Maybe<StringView> getSecretPropertyView(String const& name) const;
  // Gets a secret Json property. It will be de-serialized.
  Json getSecretProperty(String const& name, Json defaultValue = Json()) const;
  // Sets a secret Json property. It will be serialized.
  void setSecretProperty(String const& name, Json const& value);

private:
  typedef LuaMessageHandlingComponent<LuaStorableComponent<LuaActorMovementComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>>> GenericScriptComponent;
  typedef shared_ptr<GenericScriptComponent> GenericScriptComponentPtr;

  typedef std::function<void(ItemPtr)> ItemSetFunc;

  // handle input and other events (master only) that happen BEFORE movement/tech controller updates
  void processControls();

  // state changes and effect animations (master and slave) that happen AFTER movement/tech controller updates
  void processStateChanges(float dt);

  void getNetStates(bool initial);
  void setNetStates();

  List<Drawable> drawables() const;
  List<OverheadBar> bars() const;
  List<Particle> particles();
  String getFootstepSound(Vec2I const& sensor) const;

  void tickShared(float dt);

  HumanoidEmote detectEmotes(String const& chatter);

  PlayerConfigPtr m_config;

  NetElementTopGroup m_netGroup;

  ClientContextPtr m_clientContext;
  StatisticsPtr m_statistics;
  QuestManagerPtr m_questManager;

  HumanoidPtr m_humanoid;
  PlayerInventoryPtr m_inventory;
  PlayerBlueprintsPtr m_blueprints;
  PlayerUniverseMapPtr m_universeMap;
  PlayerCodexesPtr m_codexes;
  PlayerTechPtr m_techs;
  PlayerCompanionsPtr m_companions;
  PlayerDeploymentPtr m_deployment;
  PlayerLogPtr m_log;

  UniverseClient* m_client; // required for celestial callbacks in scripts
  StringMap<GenericScriptComponentPtr> m_genericScriptContexts;
  JsonObject m_genericProperties;

  State m_state;
  HumanoidEmote m_emoteState;

  float m_footstepTimer;
  float m_teleportTimer;
  float m_emoteCooldownTimer;
  float m_blinkCooldownTimer;
  float m_lastDamagedOtherTimer;
  EntityId m_lastDamagedTarget;
  GameTimer m_ageItemsTimer;

  float m_footstepVolumeVariance;
  float m_landingVolume;
  bool m_landingNoisePending;
  bool m_footstepPending;

  String m_teleportAnimationType;
  NetworkedAnimatorPtr m_effectsAnimator;
  NetworkedAnimator::DynamicTarget m_effectsAnimatorDynamicTarget;

  float m_emoteCooldown;
  Vec2F m_blinkInterval;

  HashSet<MoveControlType> m_pendingMoves;
  Vec2F m_moveVector;
  bool m_shifting;
  ActorMovementParameters m_zeroGMovementParameters;

  List<DamageSource> m_damageSources;

  String m_description;

  PlayerMode m_modeType;
  PlayerModeConfig m_modeConfig;
  ShipUpgrades m_shipUpgrades;

  ToolUserPtr m_tools;
  ArmorWearerPtr m_armor;

  bool m_useDown;
  bool m_edgeTriggeredUse;

  Vec2F m_aimPosition;

  Maybe<EntityId> m_cameraFocusEntity;

  ActorMovementControllerPtr m_movementController;
  TechControllerPtr m_techController;
  StatusControllerPtr m_statusController;

  float m_foodLowThreshold;
  List<PersistentStatusEffect> m_foodLowStatusEffects;
  List<PersistentStatusEffect> m_foodEmptyStatusEffects;

  List<PersistentStatusEffect> m_inCinematicStatusEffects;

  HumanoidIdentity m_identity;
  bool m_identityUpdated;

  bool m_isAdmin;
  float m_interactRadius; // hand interact radius
  Vec2F m_walkIntoInteractBias; // offset on position to find an interactable
  // when not pointing at
  // an interactable with the mouse

  List<RpcPromise<InteractAction>> m_pendingInteractActions;

  List<Particle> m_callbackParticles;
  List<tuple<String, float, float>> m_callbackSounds;

  List<String> m_queuedMessages;
  List<ItemPtr> m_queuedItemPickups;

  List<ChatAction> m_pendingChatActions;

  StringSet m_missionRadioMessages;
  bool m_interruptRadioMessage;
  List<pair<GameTimer, RadioMessage>> m_delayedRadioMessages;
  Deque<RadioMessage> m_pendingRadioMessages;
  Maybe<Json> m_pendingCinematic;
  Maybe<pair<Maybe<StringList>, float>> m_pendingAltMusic;
  Maybe<PlayerWarpRequest> m_pendingWarp;
  Deque<pair<Json, RpcPromiseKeeper<Json>>> m_pendingConfirmations;

  AiState m_aiState;

  String m_chatMessage;
  bool m_chatMessageChanged;
  bool m_chatMessageUpdated;

  EffectEmitterPtr m_effectEmitter;

  SongbookPtr m_songbook;

  int m_hitDamageNotificationLimiter;
  int m_hitDamageNotificationLimit;

  StringSet m_interestingObjects;

  NetElementUInt m_stateNetState;
  NetElementBool m_shiftingNetState;
  NetElementFloat m_xAimPositionNetState;
  NetElementFloat m_yAimPositionNetState;
  NetElementData<HumanoidIdentity> m_identityNetState;
  NetElementData<EntityDamageTeam> m_teamNetState;
  NetElementEvent m_landedNetState;
  NetElementString m_chatMessageNetState;
  NetElementEvent m_newChatMessageNetState;
  NetElementString m_emoteNetState;
};

}
