#ifndef STAR_MONSTER_HPP
#define STAR_MONSTER_HPP

#include "StarEntity.hpp"
#include "StarNetElementSystem.hpp"
#include "StarEntityRendering.hpp"
#include "StarActorMovementController.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarEffectEmitter.hpp"
#include "StarMonsterDatabase.hpp"
#include "StarDamageBarEntity.hpp"
#include "StarNametagEntity.hpp"
#include "StarPortraitEntity.hpp"
#include "StarAggressiveEntity.hpp"
#include "StarScriptedEntity.hpp"
#include "StarChattyEntity.hpp"
#include "StarPhysicsEntity.hpp"
#include "StarBehaviorState.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaAnimationComponent.hpp"
#include "StarLuaActorMovementComponent.hpp"

namespace Star {

STAR_CLASS(Monster);
STAR_CLASS(StatusController);

class Monster
  : public virtual DamageBarEntity,
    public virtual AggressiveEntity,
    public virtual ScriptedEntity,
    public virtual PhysicsEntity,
    public virtual NametagEntity,
    public virtual ChattyEntity,
    public virtual InteractiveEntity {
public:
  struct SkillInfo {
    String label;
    String image;
  };

  Monster(MonsterVariant const& variant, Maybe<float> level = {});
  Monster(Json const& diskStore);

  Json diskStore() const;
  ByteArray netStore();

  EntityType entityType() const override;
  ClientEntityMode clientEntityMode() const override;

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  Vec2F position() const override;
  RectF metaBoundBox() const override;

  Vec2F velocity() const;

  Vec2F mouthOffset() const;
  Vec2F feetOffset() const;

  RectF collisionArea() const override;

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0) override;
  void readNetState(ByteArray data, float interpolationTime = 0.0f) override;

  void enableInterpolation(float extrapolationHint) override;
  void disableInterpolation() override;

  String description() const override;

  List<LightSource> lightSources() const override;

  Maybe<HitType> queryHit(DamageSource const& source) const override;
  Maybe<PolyF> hitPoly() const override;

  void hitOther(EntityId targetEntityId, DamageRequest const& damageRequest) override;
  void damagedOther(DamageNotification const& damage) override;

  List<DamageNotification> applyDamage(DamageRequest const& damage) override;
  List<DamageNotification> selfDamageNotifications() override;

  List<DamageSource> damageSources() const override;

  bool shouldDie();
  void knockout();

  bool shouldDestroy() const override;
  void destroy(RenderCallback* renderCallback) override;

  void update(uint64_t currentStep) override;

  void render(RenderCallback* renderCallback) override;

  void setPosition(Vec2F const& pos);

  Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) override;

  float maxHealth() const override;
  float health() const override;
  DamageBarType damageBar() const override;

  float monsterLevel() const;
  SkillInfo activeSkillInfo() const;

  List<Drawable> portrait(PortraitMode mode) const override;
  String name() const override;
  String typeName() const;
  MonsterVariant monsterVariant() const;

  Maybe<String> statusText() const override;
  bool displayNametag() const override;
  Vec3B nametagColor() const override;
  Vec2F nametagOrigin() const override;

  bool aggressive() const override;

  Maybe<LuaValue> callScript(String const& func, LuaVariadic<LuaValue> const& args) override;
  Maybe<LuaValue> evalScript(String const& code) override;

  virtual Vec2F mouthPosition() const override;
  virtual Vec2F mouthPosition(bool ignoreAdjustments) const override;
  virtual List<ChatAction> pullPendingChatActions() override;

  List<PhysicsForceRegion> forceRegions() const override;

  InteractAction interact(InteractRequest const& request) override;
  bool isInteractive() const override;

  Vec2F questIndicatorPosition() const override;

  using Entity::setKeepAlive;
  using Entity::setUniqueId;

private:
  Vec2F getAbsolutePosition(Vec2F relativePosition) const;

  void updateStatus();
  LuaCallbacks makeMonsterCallbacks();

  void addChatMessage(String const& message, String const& portrait = "");

  void setupNetStates();
  void getNetStates(bool initial);
  void setNetStates();

  NetElementTopGroup m_netGroup;

  NetElementData<Maybe<String>> m_uniqueIdNetState;
  NetElementData<EntityDamageTeam> m_teamNetState;
  MonsterVariant m_monsterVariant;
  Maybe<float> m_monsterLevel;

  NetworkedAnimator m_networkedAnimator;
  NetworkedAnimator::DynamicTarget m_networkedAnimatorDynamicTarget;

  ActorMovementControllerPtr m_movementController;
  StatusControllerPtr m_statusController;

  EffectEmitter m_effectEmitter;

  // The set of damage source kinds that were used to kill this entity.
  StringSet m_deathDamageSourceKinds;

  bool m_damageOnTouch;
  bool m_aggressive;

  bool m_knockedOut;
  double m_knockoutTimer;
  String m_deathParticleBurst;
  String m_deathSound;

  String m_activeSkillName;
  Json m_dropPool;

  Vec2F m_questIndicatorOffset;

  List<BehaviorStatePtr> m_behaviors;
  mutable LuaMessageHandlingComponent<LuaStorableComponent<LuaActorMovementComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>>> m_scriptComponent;
  LuaAnimationComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>> m_scriptedAnimator;

  NetElementData<List<PhysicsForceRegion>> m_physicsForces;

  NetElementData<Maybe<float>> m_monsterLevelNetState;
  NetElementBool m_damageOnTouchNetState;
  NetElementData<StringSet> m_animationDamageParts;
  NetElementData<List<DamageSource>> m_damageSources;
  NetElementData<Json> m_dropPoolNetState;
  NetElementBool m_aggressiveNetState;
  NetElementBool m_knockedOutNetState;
  NetElementString m_deathParticleBurstNetState;
  NetElementString m_deathSoundNetState;
  NetElementString m_activeSkillNameNetState;
  NetElementData<Maybe<String>> m_name;
  NetElementBool m_displayNametag;
  NetElementBool m_interactive;

  List<ChatAction> m_pendingChatActions;
  NetElementEvent m_newChatMessageEvent;
  NetElementString m_chatMessage;
  NetElementString m_chatPortrait;

  NetElementData<DamageBarType> m_damageBar;

  NetElementHashMap<String, Json> m_scriptedAnimationParameters;
};

}

#endif
