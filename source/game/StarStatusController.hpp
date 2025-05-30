#pragma once

#include "StarObserverStream.hpp"
#include "StarNetElementSystem.hpp"
#include "StarNetElementExt.hpp"
#include "StarStatCollection.hpp"
#include "StarStatusEffectDatabase.hpp"
#include "StarDamage.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaActorMovementComponent.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarEntityRenderingTypes.hpp"

namespace Star {

STAR_CLASS(StatusController);

class StatusController : public NetElement {
public:
  StatusController(Json const& config);

  Json diskStore() const;
  void diskLoad(Json const& store);

  Json statusProperty(String const& name, Json const& def = Json()) const;
  void setStatusProperty(String const& name, Json value);

  StringList statNames() const;
  float stat(String const& statName) const;
  // Returns true if the stat is strictly greater than zero
  bool statPositive(String const& statName) const;

  StringList resourceNames() const;
  bool isResource(String const& resourceName) const;
  float resource(String const& resourceName) const;
  // Returns true if the resource is strictly greater than zero
  bool resourcePositive(String const& resourceName) const;

  void setResource(String const& resourceName, float value);
  void modifyResource(String const& resourceName, float amount);

  float giveResource(String const& resourceName, float amount);

  bool consumeResource(String const& resourceName, float amount);
  bool overConsumeResource(String const& resourceName, float amount);

  bool resourceLocked(String const& resourceName) const;
  void setResourceLocked(String const& resourceName, bool locked);

  // Resetting a resource also clears any locked states
  void resetResource(String const& resourceName);
  void resetAllResources();

  Maybe<float> resourceMax(String const& resourceName) const;
  Maybe<float> resourcePercentage(String const& resourceName) const;
  float setResourcePercentage(String const& resourceName, float resourcePercentage);
  float modifyResourcePercentage(String const& resourceName, float resourcePercentage);

  List<PersistentStatusEffect> getPersistentEffects(String const& statEffectCategory) const;
  void addPersistentEffect(String const& statEffectCategory, PersistentStatusEffect const& persistentEffect);
  void addPersistentEffects(String const& statEffectCategory, List<PersistentStatusEffect> const& persistentEffects);
  void setPersistentEffects(String const& statEffectCategory, List<PersistentStatusEffect> const& persistentEffects);
  void clearPersistentEffects(String const& statEffectCategory);
  void clearAllPersistentEffects();

  void addEphemeralEffect(EphemeralStatusEffect const& effect, Maybe<EntityId> sourceEntityId = {});
  void addEphemeralEffects(List<EphemeralStatusEffect> const& effectList, Maybe<EntityId> sourceEntityId = {});
  // Will have no effect if the unique effect is not applied ephemerally
  bool removeEphemeralEffect(UniqueStatusEffect const& uniqueEffect);
  void clearEphemeralEffects();

  bool appliesEnvironmentStatusEffects() const;
  void setAppliesEnvironmentStatusEffects(bool appliesEnvironmentStatusEffects);

  // All unique stat effects, whether applied ephemerally or persistently, and
  // their remaining durations.
  ActiveUniqueStatusEffectSummary activeUniqueStatusEffectSummary() const;

  bool uniqueStatusEffectActive(String const& effectName) const;

  const Directives& primaryDirectives() const;
  void setPrimaryDirectives(Directives const& directives);

  // damage request and notification methods should only be called on the master controller.
  List<DamageNotification> applyDamageRequest(DamageRequest const& damageRequest);
  void hitOther(EntityId targetEntityId, DamageRequest damageRequest);
  void damagedOther(DamageNotification damageNotification);
  List<DamageNotification> pullSelfDamageNotifications();
  void applySelfDamageRequest(DamageRequest dr);

  // Pulls recent incoming and outgoing damage notifications.  In order for
  // multiple viewers keep track of notifications and avoid duplicates, the
  // damage notifications are indexed by a monotonically increasing 'step'
  // value.  Every call will return the recent damage notifications, along with
  // another step value to pass into the function on the next call to get
  // damage notifications SINCE the first call.  If since is 0, returns all
  // recent notifications available.
  pair<List<DamageNotification>, uint64_t> damageTakenSince(uint64_t since = 0) const;
  pair<List<pair<EntityId, DamageRequest>>, uint64_t> inflictedHitsSince(uint64_t since = 0) const;
  pair<List<DamageNotification>, uint64_t> inflictedDamageSince(uint64_t since = 0) const;

  void init(Entity* parentEntity, ActorMovementController* movementController);
  void uninit();

  void initNetVersion(NetElementVersion const* version = nullptr) override;

  void netStore(DataStream& ds, NetCompatibilityRules rules = {}) const override;
  void netLoad(DataStream& ds, NetCompatibilityRules rules) override;

  void enableNetInterpolation(float extrapolationHint = 0.0f) override;
  void disableNetInterpolation() override;
  void tickNetInterpolation(float dt) override;

  bool writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules = {}) const override;
  void readNetDelta(DataStream& ds, float interpolationTime = 0.0f, NetCompatibilityRules rules = {}) override;
  void blankNetDelta(float interpolationTime) override;

  void tickMaster(float dt);
  void tickSlave(float dt);

  const DirectivesGroup& parentDirectives() const;
  List<Drawable> drawables() const;
  List<LightSource> lightSources() const;
  List<OverheadBar> overheadBars();

  // new audios and particles will only be generated on the client
  List<AudioInstancePtr> pullNewAudios();
  List<Particle> pullNewParticles();

  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args = {});

  void setScale(float scale);

private:
  typedef LuaMessageHandlingComponent<LuaActorMovementComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>> StatScript;

  struct EffectAnimator : public NetElement {
    EffectAnimator(Maybe<String> animationConfig = {});

    void initNetVersion(NetElementVersion const* version = nullptr) override;

    void netStore(DataStream& ds, NetCompatibilityRules rules = {}) const override;
    void netLoad(DataStream& ds, NetCompatibilityRules rules) override;

    void enableNetInterpolation(float extrapolationHint = 0.0f) override;
    void disableNetInterpolation() override;
    void tickNetInterpolation(float dt) override;

    bool writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules = {}) const override;
    void readNetDelta(DataStream& ds, float interpolationTime = 0.0f, NetCompatibilityRules rules = {}) override;
    void blankNetDelta(float interpolationTime) override;

    Maybe<String> animationConfig;
    NetworkedAnimator animator;
    NetworkedAnimator::DynamicTarget dynamicTarget;
  };
  typedef NetElementDynamicGroup<EffectAnimator> EffectAnimatorGroup;

  struct UniqueEffectMetadata : public NetElementSyncGroup {
    UniqueEffectMetadata();
    UniqueEffectMetadata(UniqueStatusEffect effect, Maybe<float> duration, Maybe<EntityId> sourceEntityId);

    void netElementsNeedLoad(bool full) override;
    void netElementsNeedStore() override;

    UniqueStatusEffect effect;
    Maybe<float> duration;
    NetElementFloat durationNetState;
    NetElementFloat maxDuration;

    // If the sourceEntityId is not set here, this implies that the cause of
    // the unique effect was the owning entity.
    NetElementData<Maybe<EntityId>> sourceEntityId;
  };
  typedef NetElementDynamicGroup<UniqueEffectMetadata> UniqueEffectMetadataGroup;

  struct PersistentEffectCategory {
    Maybe<StatModifierGroupId> modifierEffectsGroupId;
    List<StatModifier> statModifiers;
    HashSet<UniqueStatusEffect> uniqueEffects;
  };

  struct UniqueEffectInstance {
    UniqueStatusEffectConfig effectConfig;
    Directives parentDirectives;
    HashSet<StatModifierGroupId> modifierGroups;
    StatScript script;
    UniqueEffectMetadataGroup::ElementId metadataId;
    EffectAnimatorGroup::ElementId animatorId;
  };

  void updateAnimators(float dt);
  void updatePersistentUniqueEffects();

  float defaultUniqueEffectDuration(UniqueStatusEffect const& name) const;
  bool addUniqueEffect(UniqueStatusEffect const& effect, Maybe<float> duration, Maybe<EntityId> sourceEntityId);
  void removeUniqueEffect(UniqueStatusEffect const& name);

  void initPrimaryScript();
  void uninitPrimaryScript();

  void initUniqueEffectScript(UniqueEffectInstance& uniqueEffect);
  void uninitUniqueEffectScript(UniqueEffectInstance& uniqueEffect);

  LuaCallbacks makeUniqueEffectCallbacks(UniqueEffectInstance& uniqueEffect);

  NetElementGroup m_netGroup;
  StatCollection m_statCollection;
  NetElementOverride<NetElementHashMap<String, Json>> m_statusProperties;
  NetElementData<DirectivesGroup> m_parentDirectives;

  UniqueEffectMetadataGroup m_uniqueEffectMetadata;
  EffectAnimatorGroup m_effectAnimators;

  Entity* m_parentEntity;
  ActorMovementController* m_movementController;

  // Members below are only valid on the master entity

  // there are two magic keys used for this map: 'entities' and 'environment' for StatusEffectEntity
  // and environmentally applied persistent status effects, respectively
  StringMap<PersistentEffectCategory> m_persistentEffects;
  StableHashMap<UniqueStatusEffect, UniqueEffectInstance> m_uniqueEffects;
  float m_minimumLiquidStatusEffectPercentage;
  bool m_appliesEnvironmentStatusEffects;
  bool m_appliesWeatherStatusEffects;
  GameTimer m_environmentStatusEffectUpdateTimer;

  Maybe<String> m_primaryAnimationConfig;
  StatScript m_primaryScript;
  Directives m_primaryDirectives;
  EffectAnimatorGroup::ElementId m_primaryAnimatorId;

  List<DamageNotification> m_pendingSelfDamageNotifications;

  ObserverStream<pair<EntityId, DamageRequest>> m_recentHitsGiven;
  ObserverStream<DamageNotification> m_recentDamageGiven;
  ObserverStream<DamageNotification> m_recentDamageTaken;
};

}
