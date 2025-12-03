#pragma once

#include "StarNpcDatabase.hpp"
#include "StarEntity.hpp"
#include "StarNetElementSystem.hpp"
#include "StarActorMovementController.hpp"
#include "StarHumanoid.hpp"
#include "StarEffectEmitter.hpp"
#include "StarEntitySplash.hpp"
#include "StarDamageBarEntity.hpp"
#include "StarNametagEntity.hpp"
#include "StarPortraitEntity.hpp"
#include "StarScriptedEntity.hpp"
#include "StarChattyEntity.hpp"
#include "StarEmoteEntity.hpp"
#include "StarInteractiveEntity.hpp"
#include "StarLoungingEntities.hpp"
#include "StarToolUserEntity.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaActorMovementComponent.hpp"
#include "StarItemBag.hpp"
#include "StarArmorWearer.hpp"
#include "StarToolUser.hpp"
#include "StarPhysicsEntity.hpp"
#include "StarLuaAnimationComponent.hpp"

namespace Star {

STAR_CLASS(Songbook);
STAR_CLASS(Item);
STAR_CLASS(RenderCallback);
STAR_CLASS(Npc);
STAR_CLASS(StatusController);

class Npc
  : public virtual DamageBarEntity,
    public virtual PortraitEntity,
    public virtual NametagEntity,
    public virtual ScriptedEntity,
    public virtual ChattyEntity,
    public virtual InteractiveEntity,
    public virtual LoungingEntity,
    public virtual ToolUserEntity,
    public virtual PhysicsEntity,
    public virtual EmoteEntity {
public:
  Npc(ByteArray const& netStore, NetCompatibilityRules rules = {});
  Npc(NpcVariant const& npcVariant);
  Npc(NpcVariant const& npcVariant, Json const& initialState);

  Json diskStore() const;
  ByteArray netStore(NetCompatibilityRules rules = {});

  EntityType entityType() const override;
  ClientEntityMode clientEntityMode() const override;

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  Vec2F position() const override;
  RectF metaBoundBox() const override;

  Vec2F mouthOffset(bool ignoreAdjustments = true) const;
  Vec2F feetOffset() const;
  Vec2F headArmorOffset() const;
  Vec2F chestArmorOffset() const;
  Vec2F legsArmorOffset() const;
  Vec2F backArmorOffset() const;

  RectF collisionArea() const override;

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0, NetCompatibilityRules rules = {}) override;
  void readNetState(ByteArray data, float interpolationTime = 0.0f, NetCompatibilityRules rules = {}) override;

  void enableInterpolation(float extrapolationHint = 0.0f) override;
  void disableInterpolation() override;

  String description() const override;
  String species() const override;
  Gender gender() const;
  String npcType() const;

  Json scriptConfigParameter(String const& parameterName, Json const& defaultValue = Json()) const;

  Maybe<HitType> queryHit(DamageSource const& source) const override;
  Maybe<PolyF> hitPoly() const override;

  void damagedOther(DamageNotification const& damage) override;

  List<DamageNotification> applyDamage(DamageRequest const& damage) override;
  List<DamageNotification> selfDamageNotifications() override;

  bool shouldDestroy() const override;
  void destroy(RenderCallback* renderCallback) override;

  void update(float dt, uint64_t currentVersion) override;

  void render(RenderCallback* renderCallback) override;
  List<Drawable> drawables(Vec2F position = Vec2F()) override;

  void renderLightSources(RenderCallback* renderCallback) override;

  void setPosition(Vec2F const& pos);

  float maxHealth() const override;
  float health() const override;
  DamageBarType damageBar() const override;

  List<Drawable> portrait(PortraitMode mode) const override;
  String name() const override;
  Maybe<String> statusText() const override;
  bool displayNametag() const override;
  Vec3B nametagColor() const override;
  Vec2F nametagOrigin() const override;
  String nametag() const override;

  bool aggressive() const;

  Maybe<LuaValue> callScript(String const& func, LuaVariadic<LuaValue> const& args) override;
  Maybe<LuaValue> evalScript(String const& code) override;

  Vec2F mouthPosition() const override;
  Vec2F mouthPosition(bool ignoreAdjustments) const override;
  List<ChatAction> pullPendingChatActions() override;

  bool isInteractive() const override;
  InteractAction interact(InteractRequest const& request) override;
  RectF interactiveBoundBox() const override;

  Maybe<EntityAnchorState> loungingIn() const override;

  List<QuestArcDescriptor> offeredQuests() const override;
  StringSet turnInQuests() const override;
  Vec2F questIndicatorPosition() const override;

  List<LightSource> lightSources() const override;

  Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) override;

  Vec2F armPosition(ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset = {}) const override;
  Vec2F handOffset(ToolHand hand, Direction facingDirection) const override;
  Vec2F handPosition(ToolHand hand, Vec2F const& handOffset = {}) const override;
  ItemPtr handItem(ToolHand hand) const override;
  Vec2F armAdjustment() const override;
  Vec2F velocity() const override;
  Vec2F aimPosition() const override;
  float interactRadius() const override;
  Direction facingDirection() const override;
  Direction walkingDirection() const override;
  bool isAdmin() const override;
  Color favoriteColor() const override;
  float beamGunRadius() const override;
  void addParticles(List<Particle> const& particles) override;
  void addSound(String const& sound, float volume = 1.0f, float pitch = 1.0f) override;
  bool inToolRange() const override;
  bool inToolRange(Vec2F const& position) const override;
  void addEphemeralStatusEffects(List<EphemeralStatusEffect> const& statusEffects) override;
  ActiveUniqueStatusEffectSummary activeUniqueStatusEffectSummary() const override;
  float powerMultiplier() const override;
  bool fullEnergy() const override;
  float energy() const override;
  bool energyLocked() const override;
  bool consumeEnergy(float energy) override;
  void queueUIMessage(String const& message) override;
  bool instrumentPlaying() override;
  void instrumentEquipped(String const& instrumentKind) override;
  void interact(InteractAction const& action) override;
  void addEffectEmitters(StringSet const& emitters) override;
  void requestEmote(String const& emote) override;
  ActorMovementController* movementController() override;
  StatusController* statusController() override;
  Songbook* songbook();
  void setCameraFocusEntity(Maybe<EntityId> const& cameraFocusEntity) override;

  void playEmote(HumanoidEmote emote) override;

  List<DamageSource> damageSources() const override;

  List<PhysicsForceRegion> forceRegions() const override;

  using Entity::setUniqueId;

  HumanoidIdentity const& identity() const;
  void updateIdentity();
  void setIdentity(HumanoidIdentity identity);

  void setHumanoidParameter(String key, Maybe<Json> value);
  Maybe<Json> getHumanoidParameter(String key);
  void setHumanoidParameters(JsonObject parameters);
  JsonObject getHumanoidParameters();

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

  void setSpecies(String const& species);
  void setGender(Gender const& gender);
  void setPersonality(Personality const& personality);
  void setImagePath(Maybe<String> const& imagePath);

  void setFavoriteColor(Color color);
  void setName(String const& name);
  void setDescription(String const& description);

  HumanoidPtr humanoid();
  HumanoidPtr humanoid() const;

  bool forceNude() const;

private:
  Vec2F getAbsolutePosition(Vec2F relativePosition) const;

  void tickShared(float dt);
  LuaCallbacks makeNpcCallbacks();

  void setupNetStates();
  void getNetStates(bool initial);
  void setNetStates();

  void addChatMessage(String const& message, Json const& config, String const& portrait = "");
  void addEmote(HumanoidEmote const& emote);
  void setDance(Maybe<String> const& danceName);

  bool setItemSlot(String const& slot, ItemDescriptor itemDescriptor);

  bool canUseTool() const;

  void disableWornArmor(bool disable);

  void refreshHumanoidParameters();

  NetElementDynamicGroup<NetHumanoid> m_netHumanoid;
  LuaAnimationComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>> m_scriptedAnimator;
  NetElementHashMap<String, Json> m_scriptedAnimationParameters;
  NetworkedAnimator::DynamicTarget m_humanoidDynamicTarget;

  NpcVariant m_npcVariant;
  NetElementTopGroup m_netGroup;
  NetElementData<StringList> m_dropPools;

  NetElementData<Maybe<String>> m_uniqueIdNetState;
  NetElementData<EntityDamageTeam> m_teamNetState;

  ClientEntityMode m_clientEntityMode;

  NetElementEnum<Humanoid::State> m_humanoidStateNetState;
  NetElementEnum<HumanoidEmote> m_humanoidEmoteStateNetState;
  NetElementData<Maybe<String>> m_humanoidDanceNetState;

  NetElementData<HumanoidIdentity> m_identityNetState;
  NetElementEvent m_refreshedHumanoidParameters;
  bool m_identityUpdated;

  NetElementData<Maybe<String>> m_deathParticleBurst;

  ActorMovementControllerPtr m_movementController;
  StatusControllerPtr m_statusController;
  EffectEmitterPtr m_effectEmitter;

  NetElementBool m_aggressive;

  List<BehaviorStatePtr> m_behaviors;
  mutable LuaMessageHandlingComponent<LuaStorableComponent<LuaActorMovementComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>>> m_scriptComponent;

  List<ChatAction> m_pendingChatActions;
  NetElementEvent m_newChatMessageEvent;
  NetElementString m_chatMessage;
  NetElementString m_chatPortrait;
  NetElementData<Json> m_chatConfig;
  bool m_chatMessageUpdated;

  NetElementData<Maybe<String>> m_statusText;
  NetElementBool m_displayNametag;

  HumanoidEmote m_emoteState;
  GameTimer m_emoteCooldownTimer;
  Maybe<String> m_dance;
  GameTimer m_danceCooldownTimer;
  GameTimer m_blinkCooldownTimer;
  Vec2F m_blinkInterval;

  NetElementBool m_isInteractive;

  NetElementData<List<QuestArcDescriptor>> m_offeredQuests;
  NetElementData<StringSet> m_turnInQuests;

  Vec2F m_questIndicatorOffset;

  ArmorWearerPtr m_armor;
  ToolUserPtr m_tools;
  SongbookPtr m_songbook;

  NetElementBool m_disableWornArmor;

  NetElementFloat m_xAimPosition;
  NetElementFloat m_yAimPosition;

  NetElementBool m_shifting;
  NetElementBool m_damageOnTouch;

  int m_hitDamageNotificationLimiter;
  int m_hitDamageNotificationLimit;

  HashSet<LoungeControl> m_LoungeControlsHeld;
};
}
