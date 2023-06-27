#include "StarStatusController.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarWorld.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarStatusEffectDatabase.hpp"
#include "StarStatusEffectEntity.hpp"
#include "StarLiquidsDatabase.hpp"

namespace Star {

StatusController::StatusController(Json const& config) : m_statCollection(config) {
  m_parentEntity = nullptr;
  m_movementController = nullptr;

  m_statusProperties.set(config.getObject("statusProperties", {}));
  m_minimumLiquidStatusEffectPercentage = config.getFloat("minimumLiquidStatusEffectPercentage");
  m_appliesEnvironmentStatusEffects = config.getBool("appliesEnvironmentStatusEffects");
  m_appliesWeatherStatusEffects = config.getBool("appliesWeatherStatusEffects");
  m_environmentStatusEffectUpdateTimer = GameTimer(config.getFloat("environmentStatusEffectUpdateTimer", 0.15f));

  m_primaryAnimationConfig = config.optString("primaryAnimationConfig");
  m_primaryScript.setScripts(jsonToStringList(config.get("primaryScriptSources", JsonArray())));
  m_primaryScript.setUpdateDelta(config.getUInt("primaryScriptDelta", 1));

  m_primaryScript.setUpdateDelta(config.getUInt("primaryScriptDelta", 1));

  uint64_t keepDamageSteps = config.getUInt("keepDamageNotificationSteps", 120);
  m_recentHitsGiven.setHistoryLimit(keepDamageSteps);
  m_recentDamageGiven.setHistoryLimit(keepDamageSteps);
  m_recentDamageTaken.setHistoryLimit(keepDamageSteps);

  m_netGroup.addNetElement(&m_statCollection);
  m_netGroup.addNetElement(&m_statusProperties);
  m_netGroup.addNetElement(&m_parentDirectives);
  m_netGroup.addNetElement(&m_uniqueEffectMetadata);
  m_netGroup.addNetElement(&m_effectAnimators);

  if (m_primaryAnimationConfig)
    m_primaryAnimatorId = m_effectAnimators.addNetElement(make_shared<EffectAnimator>(*m_primaryAnimationConfig));
  else
    m_primaryAnimatorId = EffectAnimatorGroup::NullElementId;
}

Json StatusController::diskStore() const {
  JsonObject resourceValues;
  JsonObject resourcesLocked;
  for (auto const& resourceName : resourceNames()) {
    resourceValues[resourceName] = resource(resourceName);
    resourcesLocked[resourceName] = resourceLocked(resourceName);
  }

  JsonObject persistentEffectCategories;
  for (auto const& pair : m_persistentEffects) {
    List<PersistentStatusEffect> persistentEffects;
    persistentEffects.appendAll(pair.second.statModifiers.transformed(construct<PersistentStatusEffect>()));
    persistentEffects.appendAll(pair.second.uniqueEffects.values().transformed(construct<PersistentStatusEffect>()));
    persistentEffectCategories[pair.first] = persistentEffects.transformed(jsonFromPersistentStatusEffect);
  }

  JsonArray ephemeralEffects;
  for (auto const& pair : m_uniqueEffects) {
    // Store ephemeral effects in the disk store based on remaining duration.
    // TODO: Need to store maximum duration as well in the store, otherwise the
    // effect will always appear "full" on reload (but just last less time)
    auto metadata = m_uniqueEffectMetadata.getNetElement(pair.second.metadataId);
    if (metadata->duration)
      ephemeralEffects.append(jsonFromEphemeralStatusEffect(EphemeralStatusEffect{pair.first, *metadata->duration}));
  }

  return JsonObject{
      {"statusProperties", m_statusProperties.get()},
      {"persistentEffectCategories", move(persistentEffectCategories)},
      {"ephemeralEffects", move(ephemeralEffects)},
      {"resourceValues", move(resourceValues)},
      {"resourcesLocked", move(resourcesLocked)},
  };
}

void StatusController::diskLoad(Json const& store) {
  clearAllPersistentEffects();
  clearEphemeralEffects();

  m_statusProperties.set(store.getObject("statusProperties"));

  for (auto const& p : store.getObject("persistentEffectCategories", {}))
    addPersistentEffects(p.first, p.second.toArray().transformed(jsonToPersistentStatusEffect));

  addEphemeralEffects(store.getArray("ephemeralEffects").transformed(jsonToEphemeralStatusEffect));

  for (auto const& p : store.getObject("resourceValues", {})) {
    if (isResource(p.first))
      setResource(p.first, p.second.toFloat());
  }

  for (auto const& p : store.getObject("resourcesLocked", {})) {
    if (isResource(p.first))
      setResourceLocked(p.first, p.second.toBool());
  }
}

Json StatusController::statusProperty(String const& name, Json const& def) const {
  return m_statusProperties.get().value(name, def);
}

void StatusController::setStatusProperty(String const& name, Json value) {
  m_statusProperties.update([&](JsonObject& statusProperties) {
    if (statusProperties[name] != value) {
      statusProperties[name] = move(value);
      return true;
    }
    return false;
  });
}

StringList StatusController::statNames() const {
  return m_statCollection.statNames();
}

float StatusController::stat(String const& statName) const {
  return m_statCollection.stat(statName);
}

bool StatusController::statPositive(String const& statName) const {
  return m_statCollection.statPositive(statName);
}

StringList StatusController::resourceNames() const {
  return m_statCollection.resourceNames();
}

bool StatusController::isResource(String const& resourceName) const {
  return m_statCollection.isResource(resourceName);
}

float StatusController::resource(String const& resourceName) const {
  return m_statCollection.resource(resourceName);
}

bool StatusController::resourcePositive(String const& resourceName) const {
  return m_statCollection.resourcePositive(resourceName);
}

void StatusController::setResource(String const& resourceName, float value) {
  m_statCollection.setResource(resourceName, value);
}

void StatusController::modifyResource(String const& resourceName, float amount) {
  m_statCollection.modifyResource(resourceName, amount);
}

float StatusController::giveResource(String const& resourceName, float amount) {
  return m_statCollection.giveResource(resourceName, amount);
}

bool StatusController::consumeResource(String const& resourceName, float amount) {
  if (m_statCollection.consumeResource(resourceName, amount)) {
    m_primaryScript.invoke("notifyResourceConsumed", resourceName, amount);
    return true;
  }
  return false;
}

bool StatusController::overConsumeResource(String const& resourceName, float amount) {
  if (m_statCollection.overConsumeResource(resourceName, amount)) {
    m_primaryScript.invoke("notifyResourceConsumed", resourceName, amount);
    return true;
  }
  return false;
}

bool StatusController::resourceLocked(String const& resourceName) const {
  return m_statCollection.resourceLocked(resourceName);
}

void StatusController::setResourceLocked(String const& resourceName, bool locked) {
  m_statCollection.setResourceLocked(resourceName, locked);
}

void StatusController::resetResource(String const& resourceName) {
  m_statCollection.resetResource(resourceName);
}

void StatusController::resetAllResources() {
  m_statCollection.resetAllResources();
}

Maybe<float> StatusController::resourceMax(String const& resourceName) const {
  return m_statCollection.resourceMax(resourceName);
}

Maybe<float> StatusController::resourcePercentage(String const& resourceName) const {
  return m_statCollection.resourcePercentage(resourceName);
}

float StatusController::setResourcePercentage(String const& resourceName, float resourcePercentage) {
  return m_statCollection.setResourcePercentage(resourceName, resourcePercentage);
}

float StatusController::modifyResourcePercentage(String const& resourceName, float resourcePercentage) {
  return m_statCollection.modifyResourcePercentage(resourceName, resourcePercentage);
}

List<PersistentStatusEffect> StatusController::getPersistentEffects(String const& statEffectCategory) const {
  auto category = m_persistentEffects.maybe(statEffectCategory).value();
  List<PersistentStatusEffect> persistentEffects =
      category.statModifiers.transformed(construct<PersistentStatusEffect>());
  persistentEffects.appendAll(
      List<UniqueStatusEffect>::from(category.uniqueEffects).transformed(construct<PersistentStatusEffect>()));
  return persistentEffects;
}

void StatusController::addPersistentEffect(
    String const& statusEffectCategory, PersistentStatusEffect const& persistentEffect) {
  addPersistentEffects(statusEffectCategory, {persistentEffect});
}

void StatusController::addPersistentEffects(
    String const& statusEffectCategory, List<PersistentStatusEffect> const& effectList) {
  auto& persistentEffectCategory = m_persistentEffects[statusEffectCategory];
  if (!persistentEffectCategory.modifierEffectsGroupId)
    persistentEffectCategory.modifierEffectsGroupId = m_statCollection.addStatModifierGroup();

  for (auto const& effect : effectList) {
    if (effect.is<StatModifier>())
      persistentEffectCategory.statModifiers.append(effect.get<StatModifier>());
    else if (effect.is<UniqueStatusEffect>())
      persistentEffectCategory.uniqueEffects.add(effect.get<UniqueStatusEffect>());
  }
  m_statCollection.setStatModifierGroup(
      *persistentEffectCategory.modifierEffectsGroupId, persistentEffectCategory.statModifiers);

  updatePersistentUniqueEffects();
}

void StatusController::setPersistentEffects(
    String const& statusEffectCategory, List<PersistentStatusEffect> const& effectList) {
  if (effectList.empty()) {
    if (auto groupId = m_persistentEffects[statusEffectCategory].modifierEffectsGroupId)
      m_statCollection.removeStatModifierGroup(*groupId);
    m_persistentEffects.remove(statusEffectCategory);

    updatePersistentUniqueEffects();
  } else {
    auto& persistentEffectCategory = m_persistentEffects[statusEffectCategory];
    persistentEffectCategory.statModifiers.clear();
    persistentEffectCategory.uniqueEffects.clear();
    addPersistentEffects(statusEffectCategory, effectList);
  }
}

void StatusController::clearPersistentEffects(String const& statusEffectCategory) {
  setPersistentEffects(statusEffectCategory, {});
}

void StatusController::clearAllPersistentEffects() {
  for (auto const& effectCategory : m_persistentEffects.keys())
    clearPersistentEffects(effectCategory);
}

void StatusController::addEphemeralEffect(EphemeralStatusEffect const& effect, Maybe<EntityId> sourceEntityId) {
  addEphemeralEffects({effect}, sourceEntityId);
}

void StatusController::addEphemeralEffects(
    List<EphemeralStatusEffect> const& effectList, Maybe<EntityId> sourceEntityId) {
  for (auto const& effect : effectList) {
    if (auto existingEffect = m_uniqueEffects.ptr(effect.uniqueEffect)) {
      auto metadata = m_uniqueEffectMetadata.getNetElement(existingEffect->metadataId);

      // If the effect exists and does not have a null duration, then refresh
      // the
      // duration to the max
      if (metadata->duration) {
        auto newDuration = effect.duration.value(defaultUniqueEffectDuration(effect.uniqueEffect));
        if (newDuration > *metadata->duration) {
          // Only overwrite the sourceEntityId if the duration is *extended*
          metadata->sourceEntityId.set(sourceEntityId);
          metadata->duration = newDuration;
        }
        metadata->maxDuration.set(max(metadata->maxDuration.get(), newDuration));
      }
    } else {
      addUniqueEffect(
          effect.uniqueEffect, effect.duration.value(defaultUniqueEffectDuration(effect.uniqueEffect)), sourceEntityId);
    }
  }
}

bool StatusController::removeEphemeralEffect(UniqueStatusEffect const& effect) {
  if (auto uniqueEffect = m_uniqueEffects.ptr(effect)) {
    auto metadata = m_uniqueEffectMetadata.getNetElement(uniqueEffect->metadataId);
    if (metadata->duration) {
      removeUniqueEffect(effect);
      return true;
    }
  }

  return false;
}

void StatusController::clearEphemeralEffects() {
  for (auto const& key : m_uniqueEffects.keys())
    removeEphemeralEffect(key);
}

bool StatusController::appliesEnvironmentStatusEffects() const {
  return m_appliesEnvironmentStatusEffects;
}

void StatusController::setAppliesEnvironmentStatusEffects(bool appliesEnvironmentStatusEffects) {
  m_appliesEnvironmentStatusEffects = appliesEnvironmentStatusEffects;
}

ActiveUniqueStatusEffectSummary StatusController::activeUniqueStatusEffectSummary() const {
  ActiveUniqueStatusEffectSummary summary;
  for (auto const& metadata : m_uniqueEffectMetadata.netElements()) {
    if (metadata->duration)
      summary.append({metadata->effect, *metadata->duration / metadata->maxDuration.get()});
    else
      summary.append({metadata->effect, 1.0f});
  }
  return summary;
}

bool StatusController::uniqueStatusEffectActive(String const& effectName) const {
  for (auto const& metadata : m_uniqueEffectMetadata.netElements()) {
    if (metadata->effect == effectName)
      return true;
  }

  return false;
}

const Directives& StatusController::primaryDirectives() const {
  return m_primaryDirectives;
}

void StatusController::setPrimaryDirectives(Directives const& directives) {
  m_primaryDirectives = directives;
}

List<DamageNotification> StatusController::applyDamageRequest(DamageRequest const& damageRequest) {
  if (auto damageNotifications = m_primaryScript.invoke<List<DamageNotification>>("applyDamageRequest", damageRequest)) {
    for (auto const& dn : *damageNotifications)
      m_recentDamageTaken.add(dn);
    return damageNotifications.take();
  }
  return {};
}

void StatusController::hitOther(EntityId targetEntityId, DamageRequest damageRequest) {
  m_recentHitsGiven.add({targetEntityId, move(damageRequest)});
}

void StatusController::damagedOther(DamageNotification damageNotification) {
  m_recentDamageGiven.add(move(damageNotification));
}

List<DamageNotification> StatusController::pullSelfDamageNotifications() {
  return take(m_pendingSelfDamageNotifications);
}

void StatusController::applySelfDamageRequest(DamageRequest dr) {
  auto damageNotifications = applyDamageRequest(move(dr));
  for (auto const& dn : damageNotifications)
    m_recentDamageTaken.add(dn);
  m_pendingSelfDamageNotifications.appendAll(move(damageNotifications));
}

pair<List<DamageNotification>, uint64_t> StatusController::damageTakenSince(uint64_t since) const {
  return m_recentDamageTaken.query(since);
}

pair<List<pair<EntityId, DamageRequest>>, uint64_t> StatusController::inflictedHitsSince(uint64_t since) const {
  return m_recentHitsGiven.query(since);
}

pair<List<DamageNotification>, uint64_t> StatusController::inflictedDamageSince(uint64_t since) const {
  return m_recentDamageGiven.query(since);
}

void StatusController::init(Entity* parentEntity, ActorMovementController* movementController) {
  uninit();

  m_parentEntity = parentEntity;
  m_movementController = movementController;

  if (m_parentEntity->isMaster()) {
    initPrimaryScript();
    for (auto& p : m_uniqueEffects)
      initUniqueEffectScript(p.second);
  }

  m_environmentStatusEffectUpdateTimer.reset();
}

void StatusController::uninit() {
  m_parentEntity = nullptr;
  m_movementController = nullptr;

  for (auto& p : m_uniqueEffects)
    uninitUniqueEffectScript(p.second);
  uninitPrimaryScript();

  m_recentHitsGiven.reset();
  m_recentDamageGiven.reset();
  m_recentDamageTaken.reset();
}

void StatusController::initNetVersion(NetElementVersion const* version) {
  m_netGroup.initNetVersion(version);
}

void StatusController::netStore(DataStream& ds) const {
  m_netGroup.netStore(ds);
}

void StatusController::netLoad(DataStream& ds) {
  clearAllPersistentEffects();
  clearEphemeralEffects();

  m_netGroup.netLoad(ds);
}

void StatusController::enableNetInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void StatusController::disableNetInterpolation() {
  m_netGroup.disableNetInterpolation();
}

void StatusController::tickNetInterpolation(float dt) {
  m_netGroup.tickNetInterpolation(dt);
}

bool StatusController::writeNetDelta(DataStream& ds, uint64_t fromStep) const {
  return m_netGroup.writeNetDelta(ds, fromStep);
}

void StatusController::readNetDelta(DataStream& ds, float interpolationTime) {
  m_netGroup.readNetDelta(ds, interpolationTime);
}

void StatusController::blankNetDelta(float interpolationTime) {
  m_netGroup.blankNetDelta(interpolationTime);
}

void StatusController::tickMaster() {
  m_statCollection.tickMaster();

  m_recentHitsGiven.tick(1);
  m_recentDamageGiven.tick(1);
  m_recentDamageTaken.tick(1);

  bool statusImmune = statPositive("statusImmunity");

  if (!statusImmune && m_movementController->liquidPercentage() > m_minimumLiquidStatusEffectPercentage) {
    auto liquidsDatabase = Root::singleton().liquidsDatabase();
    if (auto liquidSettings = liquidsDatabase->liquidSettings(m_movementController->liquidId())) {
      for (auto const& effect : liquidSettings->statusEffects)
        addEphemeralEffect(jsonToEphemeralStatusEffect(effect));
    }
  }

  if (m_environmentStatusEffectUpdateTimer.wrapTick()) {
    PolyF collisionBody = m_movementController->collisionBody();
    List<PersistentStatusEffect> entityEffects;
    if (!statusImmune) {
      m_parentEntity->world()->forEachEntity(collisionBody.boundBox(),
          [this, collisionBody, &entityEffects](EntityPtr const& e) {
            if (auto entity = as<StatusEffectEntity>(e)) {
              auto statusEffectArea = entity->statusEffectArea();
              statusEffectArea.translate(entity->position());
              if (m_parentEntity->world()->geometry().polyIntersectsPoly(statusEffectArea, collisionBody))
                entityEffects.appendAll(entity->statusEffects());
            }
          });
    }
    setPersistentEffects("entities", entityEffects);

    if (!statusImmune && m_appliesEnvironmentStatusEffects)
      setPersistentEffects("environment", m_parentEntity->world()->environmentStatusEffects(m_parentEntity->position()).transformed(jsonToPersistentStatusEffect));

    if (!statusImmune && m_appliesWeatherStatusEffects)
      addEphemeralEffects(m_parentEntity->world()->weatherStatusEffects(m_parentEntity->position()).transformed(jsonToEphemeralStatusEffect));
  }

  m_primaryScript.update(m_primaryScript.updateDt());
  for (auto& p : m_uniqueEffects) {
    p.second.script.update(p.second.script.updateDt());
    auto metadata = m_uniqueEffectMetadata.getNetElement(p.second.metadataId);
    if (metadata->duration)
      *metadata->duration -= WorldTimestep;
  }

  for (auto const& key : m_uniqueEffects.keys()) {
    auto& uniqueEffect = m_uniqueEffects[key];
    auto metadata = m_uniqueEffectMetadata.getNetElement(uniqueEffect.metadataId);
    if (metadata->duration && *metadata->duration <= 0.0f)
      removeUniqueEffect(key);
    else if ((metadata->duration && statPositive("statusImmunity")) || (uniqueEffect.effectConfig.blockingStat && statPositive(*uniqueEffect.effectConfig.blockingStat)))
      removeUniqueEffect(key);
  }

  DirectivesGroup parentDirectives;
  parentDirectives.append(m_primaryDirectives);
  for (auto const& pair : m_uniqueEffects)
    parentDirectives.append(pair.second.parentDirectives);

  m_parentDirectives.set(move(parentDirectives));

  updateAnimators();
}

void StatusController::tickSlave() {
  m_statCollection.tickSlave();
  updateAnimators();
}

const DirectivesGroup& StatusController::parentDirectives() const {
  return m_parentDirectives.get();
}

List<Drawable> StatusController::drawables() const {
  List<Drawable> drawables;
  for (auto const& animator : m_effectAnimators.netElements())
    drawables.appendAll(animator->animator.drawables(m_movementController->position()));
  return drawables;
}

List<LightSource> StatusController::lightSources() const {
  List<LightSource> lightSources;
  for (auto const& animator : m_effectAnimators.netElements())
    lightSources.appendAll(animator->animator.lightSources(m_movementController->position()));
  return lightSources;
}

List<OverheadBar> StatusController::overheadBars() {
  if (auto bars = m_primaryScript.invoke<JsonArray>("overheadBars"))
    return bars->transformed(construct<OverheadBar>());
  return {};
}

List<AudioInstancePtr> StatusController::pullNewAudios() {
  List<AudioInstancePtr> newAudios;
  for (auto const& animator : m_effectAnimators.netElements())
    newAudios.appendAll(animator->dynamicTarget.pullNewAudios());
  return newAudios;
}

List<Particle> StatusController::pullNewParticles() {
  List<Particle> newParticles;
  for (auto const& animator : m_effectAnimators.netElements())
    newParticles.appendAll(animator->dynamicTarget.pullNewParticles());
  return newParticles;
}

Maybe<Json> StatusController::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  Maybe<Json> result = m_primaryScript.handleMessage(message, localMessage, args);
  for (auto& p : m_uniqueEffects)
    result = result.orMaybe(p.second.script.handleMessage(message, localMessage, args));
  return result;
}

StatusController::EffectAnimator::EffectAnimator(Maybe<String> config) {
  animationConfig = move(config);
  animator = animationConfig ? NetworkedAnimator(*animationConfig) : NetworkedAnimator();
}

void StatusController::EffectAnimator::initNetVersion(NetElementVersion const* version) {
  animator.initNetVersion(version);
}

void StatusController::EffectAnimator::netStore(DataStream& ds) const {
  ds.write(animationConfig);
  animator.netStore(ds);
}

void StatusController::EffectAnimator::netLoad(DataStream& ds) {
  ds.read(animationConfig);
  animator = animationConfig ? NetworkedAnimator(*animationConfig) : NetworkedAnimator();
  animator.netLoad(ds);
}

void StatusController::EffectAnimator::enableNetInterpolation(float extrapolationHint) {
  animator.enableNetInterpolation(extrapolationHint);
}

void StatusController::EffectAnimator::disableNetInterpolation() {
  animator.disableNetInterpolation();
}

void StatusController::EffectAnimator::tickNetInterpolation(float dt) {
  animator.tickNetInterpolation(dt);
}

bool StatusController::EffectAnimator::writeNetDelta(DataStream& ds, uint64_t fromVersion) const {
  return animator.writeNetDelta(ds, fromVersion);
}

void StatusController::EffectAnimator::readNetDelta(DataStream& ds, float interpolationTime) {
  animator.readNetDelta(ds, interpolationTime);
}

void StatusController::EffectAnimator::blankNetDelta(float interpolationTime) {
  animator.blankNetDelta(interpolationTime);
}

StatusController::UniqueEffectMetadata::UniqueEffectMetadata() {
  addNetElement(&durationNetState);
  addNetElement(&maxDuration);
  addNetElement(&sourceEntityId);
  durationNetState.setFixedPointBase(0.01f);
  durationNetState.setInterpolator(lerp<float, float>);
}

StatusController::UniqueEffectMetadata::UniqueEffectMetadata(UniqueStatusEffect effect, Maybe<float> duration, Maybe<EntityId> sourceEntityId)
  : UniqueEffectMetadata() {
  this->effect = move(effect);
  this->duration = move(duration);
  this->maxDuration.set(this->duration.value());
  this->sourceEntityId.set(sourceEntityId);
}

void StatusController::UniqueEffectMetadata::netElementsNeedLoad(bool) {
  duration = durationNetState.get() >= 0.0f ? Maybe<float>(durationNetState.get()) : Maybe<float>();
}

void StatusController::UniqueEffectMetadata::netElementsNeedStore() {
  durationNetState.set(duration ? *duration : -1.0f);
}

void StatusController::updateAnimators() {
  for (auto const& animator : m_effectAnimators.netElements()) {
    if (m_parentEntity->world()->isServer()) {
      animator->animator.update(WorldTimestep, nullptr);
    } else {
      animator->animator.update(WorldTimestep, &animator->dynamicTarget);
      animator->dynamicTarget.updatePosition(m_movementController->position());
    }
  }
}

void StatusController::updatePersistentUniqueEffects() {
  Set<UniqueStatusEffect> activePersistentUniqueEffects;
  for (auto const& categoryPair : m_persistentEffects)
    activePersistentUniqueEffects.addAll(categoryPair.second.uniqueEffects);

  for (auto const& uniqueEffectName : activePersistentUniqueEffects) {
    // It is important to note here that if a unique effect exists, it *may*
    // not come from a persistent effect, it *may* be from an ephemeral effect.
    // Here, when a persistent effect overrides an ephemeral effect, it is
    // clearing the duration making it into a solely persistent effect.  This
    // means that by applying a persistent effect and then clearing it, you can
    // remove an ephemeral effect.
    if (auto existingEffect = m_uniqueEffects.ptr(uniqueEffectName))
      m_uniqueEffectMetadata.getNetElement(existingEffect->metadataId)->duration.reset();
    else
      addUniqueEffect(uniqueEffectName, {}, {});
  }

  // Again, here we are using "durationless" to mean "persistent"
  for (auto const& key : m_uniqueEffects.keys()) {
    auto metadata = m_uniqueEffectMetadata.getNetElement(m_uniqueEffects[key].metadataId);
    if (!metadata->duration && !activePersistentUniqueEffects.contains(key))
      removeUniqueEffect(key);
  }
}

float StatusController::defaultUniqueEffectDuration(UniqueStatusEffect const& effect) const {
  return Root::singleton().statusEffectDatabase()->uniqueEffectConfig(effect).defaultDuration;
}

void StatusController::addUniqueEffect(
    UniqueStatusEffect const& effect, Maybe<float> duration, Maybe<EntityId> sourceEntityId) {
  auto statusEffectDatabase = Root::singleton().statusEffectDatabase();
  if (statusEffectDatabase->isUniqueEffect(effect)) {
    auto effectConfig = statusEffectDatabase->uniqueEffectConfig(effect);
    if ((duration && statPositive("statusImmunity")) || (effectConfig.blockingStat && statPositive(*effectConfig.blockingStat)))
      return;

    auto& uniqueEffect = m_uniqueEffects[effect];
    uniqueEffect.effectConfig = effectConfig;
    uniqueEffect.script.setScripts(uniqueEffect.effectConfig.scripts);
    uniqueEffect.script.setUpdateDelta(uniqueEffect.effectConfig.scriptDelta);

    uniqueEffect.metadataId =
        m_uniqueEffectMetadata.addNetElement(make_shared<UniqueEffectMetadata>(effect, duration, sourceEntityId));

    uniqueEffect.animatorId = UniqueEffectMetadataGroup::NullElementId;
    if (uniqueEffect.effectConfig.animationConfig)
      uniqueEffect.animatorId =
          m_effectAnimators.addNetElement(make_shared<EffectAnimator>(uniqueEffect.effectConfig.animationConfig));

    if (m_parentEntity)
      initUniqueEffectScript(uniqueEffect);
  } else {
    Logger::warn("Unique status effect '{}' not found in status effect database", effect);
  }
}

void StatusController::removeUniqueEffect(UniqueStatusEffect const& effect) {
  auto& uniqueEffect = m_uniqueEffects[effect];

  uniqueEffect.script.invoke("onExpire");

  uninitUniqueEffectScript(uniqueEffect);

  m_uniqueEffectMetadata.removeNetElement(uniqueEffect.metadataId);

  if (uniqueEffect.animatorId != EffectAnimatorGroup::NullElementId)
    m_effectAnimators.removeNetElement(uniqueEffect.animatorId);

  m_uniqueEffects.remove(effect);
}

void StatusController::initPrimaryScript() {
  m_primaryScript.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(this));
  m_primaryScript.addCallbacks("entity", LuaBindings::makeEntityCallbacks(m_parentEntity));
  if (m_primaryAnimatorId != EffectAnimatorGroup::NullElementId) {
    auto animator = m_effectAnimators.getNetElement(m_primaryAnimatorId);
    m_primaryScript.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(&animator->animator));
  }
  m_primaryScript.addActorMovementCallbacks(m_movementController);
  m_primaryScript.init(m_parentEntity->world());
}

void StatusController::uninitPrimaryScript() {
  m_primaryScript.uninit();
  m_primaryScript.removeCallbacks("status");
  m_primaryScript.removeCallbacks("entity");
  m_primaryScript.removeCallbacks("animator");
  m_primaryScript.removeActorMovementCallbacks();
}

void StatusController::initUniqueEffectScript(UniqueEffectInstance& uniqueEffect) {
  uniqueEffect.script.addCallbacks("effect", makeUniqueEffectCallbacks(uniqueEffect));
  uniqueEffect.script.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(this));
  uniqueEffect.script.addCallbacks("config", LuaBindings::makeConfigCallbacks([&uniqueEffect](String const& name, Json const& def) {
      return uniqueEffect.effectConfig.effectConfig.query(name, def);
    }));
  uniqueEffect.script.addCallbacks("entity", LuaBindings::makeEntityCallbacks(m_parentEntity));
  if (uniqueEffect.animatorId != EffectAnimatorGroup::NullElementId) {
    auto animator = m_effectAnimators.getNetElement(uniqueEffect.animatorId);
    uniqueEffect.script.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(&animator->animator));
  }
  uniqueEffect.script.addActorMovementCallbacks(m_movementController);
  uniqueEffect.script.init(m_parentEntity->world());
}

void StatusController::uninitUniqueEffectScript(UniqueEffectInstance& uniqueEffect) {
  uniqueEffect.script.uninit();
  uniqueEffect.script.removeCallbacks("effect");
  uniqueEffect.script.removeCallbacks("status");
  uniqueEffect.script.removeCallbacks("config");
  uniqueEffect.script.removeCallbacks("entity");
  uniqueEffect.script.removeCallbacks("animator");
  uniqueEffect.script.removeActorMovementCallbacks();

  for (auto modifierGroup : uniqueEffect.modifierGroups)
    m_statCollection.removeStatModifierGroup(modifierGroup);
  uniqueEffect.modifierGroups.clear();
}

LuaCallbacks StatusController::makeUniqueEffectCallbacks(UniqueEffectInstance& uniqueEffect) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("duration", [this, &uniqueEffect]() {
      return m_uniqueEffectMetadata.getNetElement(uniqueEffect.metadataId)->duration;
    });
  callbacks.registerCallback("modifyDuration", [this, &uniqueEffect](float duration) {
      auto metadata = m_uniqueEffectMetadata.getNetElement(uniqueEffect.metadataId);
      if (metadata->duration)
        *metadata->duration += duration;
    });
  callbacks.registerCallback("expire", [this, &uniqueEffect]() {
      auto metadata = m_uniqueEffectMetadata.getNetElement(uniqueEffect.metadataId);
      if (metadata->duration)
        metadata->duration = 0.0f;
    });
  callbacks.registerCallback("sourceEntity", [this, &uniqueEffect]() -> Maybe<EntityId> {
      auto metadata = m_uniqueEffectMetadata.getNetElement(uniqueEffect.metadataId);
      auto sourceEntityId = metadata->sourceEntityId.get();
      if (!sourceEntityId)
        return m_parentEntity->entityId();
      if (sourceEntityId == NullEntityId)
        return {};
      return sourceEntityId;
    });
  callbacks.registerCallback("setParentDirectives", [&uniqueEffect](Maybe<String> const& directives) {
      uniqueEffect.parentDirectives = directives.value();
    });
  callbacks.registerCallback("getParameter", [&uniqueEffect](String const& name, Json const& def) -> Json {
      return uniqueEffect.effectConfig.effectConfig.query(name, def);
    });
  callbacks.registerCallback("addStatModifierGroup", [this, &uniqueEffect](List<StatModifier> const& modifiers) -> StatModifierGroupId {
      auto newGroupId = m_statCollection.addStatModifierGroup(modifiers);
      uniqueEffect.modifierGroups.add(newGroupId);
      return newGroupId;
    });
  callbacks.registerCallback("setStatModifierGroup", [this, &uniqueEffect](StatModifierGroupId groupId, List<StatModifier> const& modifiers) {
      if (!uniqueEffect.modifierGroups.contains(groupId))
        throw StatusException("Cannot set stat modifier group that was not added from this effect");
      m_statCollection.setStatModifierGroup(groupId, modifiers);
    });
  callbacks.registerCallback("removeStatModifierGroup", [this, &uniqueEffect](StatModifierGroupId groupId) {
      if (!uniqueEffect.modifierGroups.contains(groupId))
        throw StatusException("Cannot remove stat modifier group that was not added from this effect");
      m_statCollection.removeStatModifierGroup(groupId);
      uniqueEffect.modifierGroups.remove(groupId);
    });

  return callbacks;
}

}
