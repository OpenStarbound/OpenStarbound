#include "StarVehicle.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarEntityRendering.hpp"
#include "StarMovementControllerLuaBindings.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarAssets.hpp"
#include "StarScriptedAnimatorLuaBindings.hpp"
#include "StarPlayer.hpp"

namespace Star {

Vehicle::Vehicle(Json baseConfig, String path, Json dynamicConfig)
  : m_baseConfig(std::move(baseConfig)), m_path(std::move(path)), m_dynamicConfig(std::move(dynamicConfig)) {

  m_typeName = m_baseConfig.getString("name");

  setPersistent(configValue("persistent", false).toBool());
  m_clientEntityMode = ClientEntityModeNames.getLeft(configValue("clientEntityMode", "ClientSlaveOnly").toString());

  m_scriptComponent.setScript(AssetPath::relativeTo(m_path, configValue("script").toString()));
  m_scriptComponent.setUpdateDelta(configValue("scriptDelta", 1).toUInt());
  m_boundBox = jsonToRectF(configValue("boundBox"));
  m_damageTeam.set(configValue("damageTeam").opt().apply(construct<EntityDamageTeam>()).value());
  m_interactive.set(configValue("interactive", true).toBool());
  m_baseRenderLayer = parseRenderLayer(configValue("baseRenderLayer","Vehicle").toString());
  if (!configValue("overrideRenderLayer").isNull()) {
    m_overrideRenderLayer = parseRenderLayer(configValue("overrideRenderLayer").toString());
  }

  if (auto animationScript = configValue("animationScript").optString())
    m_scriptedAnimator.setScript(*animationScript);

  setupLoungePositions(
    configValue("slaveControlTimeout").toFloat(),
    configValue("slaveControlHeartbeat").toFloat(),
    configValue("loungePositions", JsonObject()).toObject(),
    configValue("receiveExtraControls", false).toBool()
  );

  for (auto const& pair : configValue("physicsCollisions", JsonObject()).iterateObject()) {
    auto& collisionConfig = m_movingCollisions[pair.first];
    collisionConfig.movingCollision = PhysicsMovingCollision::fromJson(pair.second);
    collisionConfig.attachToPart = pair.second.optString("attachToPart");
    collisionConfig.enabled.set(pair.second.getBool("enabled", true));
  }

  for (auto const& pair : configValue("physicsForces", JsonObject()).iterateObject()) {
    auto& forceRegionConfig = m_forceRegions[pair.first];
    forceRegionConfig.forceRegion = jsonToPhysicsForceRegion(pair.second);
    forceRegionConfig.attachToPart = pair.second.optString("attachToPart");
    forceRegionConfig.enabled.set(pair.second.getBool("enabled", true));
  }

  for (auto const& pair : configValue("damageSources", JsonObject()).iterateObject()) {
    auto& damageSourceConfig = m_damageSources[pair.first];
    damageSourceConfig.damageSource = DamageSource(pair.second);
    damageSourceConfig.attachToPart = pair.second.optString("attachToPart");
    damageSourceConfig.enabled.set(pair.second.getBool("enabled", true));
  }

  auto assets = Root::singleton().assets();
  auto animationConfig = assets->fetchJson(configValue("animation"), m_path);
  if (auto customConfig = configValue("animationCustom"))
    animationConfig = jsonMerge(animationConfig, customConfig);

  m_networkedAnimator = NetworkedAnimator(animationConfig, m_path);

  for (auto const& p : configValue("animationGlobalTags", JsonObject()).iterateObject())
    m_networkedAnimator.setGlobalTag(p.first, p.second.toString());
  for (auto const& partPair : configValue("animationPartTags", JsonObject()).iterateObject()) {
    for (auto const& tagPair : partPair.second.iterateObject())
      m_networkedAnimator.setPartTag(partPair.first, tagPair.first, tagPair.second.toString());
  }

  auto movementParameters = MovementParameters(configValue("movementSettings"));
  if (!movementParameters.physicsEffectCategories)
    movementParameters.physicsEffectCategories = StringSet({"vehicle"});
  m_movementController.resetParameters(movementParameters);

  m_netGroup.addNetElement(&m_interactive);
  m_netGroup.addNetElement(&m_movementController);
  m_netGroup.addNetElement(&m_networkedAnimator);
  m_netGroup.addNetElement(&m_damageTeam);

  setupLoungeNetStates(&m_netGroup, 0);

  m_movingCollisions.sortByKey();
  for (auto& p : m_movingCollisions)
    m_netGroup.addNetElement(&p.second.enabled);

  m_forceRegions.sortByKey();
  for (auto& p : m_forceRegions)
    m_netGroup.addNetElement(&p.second.enabled);

  m_damageSources.sortByKey();
  for (auto& p : m_damageSources)
    m_netGroup.addNetElement(&p.second.enabled);

  // don't interpolate scripted animation parameters
  m_netGroup.addNetElement(&m_scriptedAnimationParameters, false);
}

String Vehicle::name() const {
  return m_typeName;
}

Json Vehicle::baseConfig() const {
  return m_baseConfig;
}

Json Vehicle::dynamicConfig() const {
  return m_dynamicConfig;
}

Json Vehicle::diskStore() const {
  return JsonObject{
    {"movement", m_movementController.storeState()},
    {"damageTeam", m_damageTeam.get().toJson()},
    {"persistent", persistent()},
    {"scriptStorage", m_scriptComponent.getScriptStorage()}
  };
}

void Vehicle::diskLoad(Json diskStore) {
  m_movementController.loadState(diskStore.get("movement"));
  m_damageTeam.set(EntityDamageTeam(diskStore.get("damageTeam")));
  setPersistent(diskStore.getBool("persistent"));
  m_scriptComponent.setScriptStorage(diskStore.getObject("scriptStorage"));
}

EntityType Vehicle::entityType() const {
  return EntityType::Vehicle;
}

ClientEntityMode Vehicle::clientEntityMode() const {
  return m_clientEntityMode;
}

Maybe<HitType> Vehicle::queryHit(DamageSource const& source) const {
  if (source.intersectsWithPoly(world()->geometry(), m_movementController.collisionBody()))
    return HitType::Hit;

  return {};
}

Maybe<PolyF> Vehicle::hitPoly() const {
  return m_movementController.collisionBody();
}

List<DamageNotification> Vehicle::applyDamage(DamageRequest const& damage) {
  if (!inWorld())
    return {};

  return m_scriptComponent.invoke<List<DamageNotification>>("applyDamage", damage).value();
}

List<DamageNotification> Vehicle::selfDamageNotifications() {
  return m_scriptComponent.invoke<List<DamageNotification>>("selfDamageNotifications").value();
}

void Vehicle::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);
  m_movementController.init(world);
  m_movementController.setIgnorePhysicsEntities({entityId});
  if (isMaster()) {
    m_scriptComponent.addCallbacks("vehicle", makeVehicleCallbacks());
    m_scriptComponent.addCallbacks(
        "config", LuaBindings::makeConfigCallbacks(bind(&Vehicle::configValue, this, _1, _2)));
    m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptComponent.addCallbacks("mcontroller", LuaBindings::makeMovementControllerCallbacks(&m_movementController));
    m_scriptComponent.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(&m_networkedAnimator));
    m_scriptComponent.init(world);
  }
  loungeInit();

  if (world->isClient()) {
    m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(&m_networkedAnimator,
      [this](String const& name, Json const& defaultValue) -> Json {
        return m_scriptedAnimationParameters.value(name, defaultValue);
      }));
    m_scriptedAnimator.addCallbacks("config", LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def) {
        return configValue(name, def);
      }));
    m_scriptedAnimator.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));

    m_scriptedAnimator.init(world);
  }
}

void Vehicle::uninit() {
  m_scriptComponent.uninit();
  m_scriptComponent.removeCallbacks("vehicle");
  m_scriptComponent.removeCallbacks("config");
  m_scriptComponent.removeCallbacks("entity");
  m_scriptComponent.removeCallbacks("mcontroller");
  m_scriptComponent.removeCallbacks("animator");
  m_movementController.uninit();

  if (world()->isClient()) {
    m_scriptedAnimator.removeCallbacks("animationConfig");
    m_scriptedAnimator.removeCallbacks("config");
    m_scriptedAnimator.removeCallbacks("entity");
  }

  Entity::uninit();
}

Vec2F Vehicle::position() const {
  return m_movementController.position();
}

RectF Vehicle::metaBoundBox() const {
  return m_boundBox;
}

RectF Vehicle::collisionArea() const {
  return m_movementController.collisionPoly().boundBox();
}

Vec2F Vehicle::velocity() const {
  return m_movementController.velocity();
}

pair<ByteArray, uint64_t> Vehicle::writeNetState(uint64_t fromVersion, NetCompatibilityRules rules) {
  return m_netGroup.writeNetState(fromVersion, rules);
}

void Vehicle::readNetState(ByteArray data, float interpolationTime, NetCompatibilityRules rules) {
  m_netGroup.readNetState(data, interpolationTime, rules);
}

void Vehicle::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void Vehicle::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

void Vehicle::update(float dt, uint64_t) {
  setTeam(m_damageTeam.get());

  if (world()->isClient()) {
    m_networkedAnimator.update(dt, &m_networkedAnimatorDynamicTarget);
    m_networkedAnimatorDynamicTarget.updatePosition(position());
  } else {
    m_networkedAnimator.update(dt, nullptr);
  }

  if (isMaster()) {
    m_movementController.tickMaster(dt);
    m_scriptComponent.update(m_scriptComponent.updateDt(dt));

    loungeTickMaster(dt);
  } else {
    m_netGroup.tickNetInterpolation(dt);

    m_movementController.tickSlave(dt);

    loungeTickSlave(dt);

  }

  if (world()->isClient())
    m_scriptedAnimator.update();

  if (world()->isClient())
    SpatialLogger::logPoly("world", m_movementController.collisionBody(), {255, 255, 0, 255});
}

void Vehicle::render(RenderCallback* renderer) {
  clearLoungingDrawables();
  setupLoungingDrawables();
  for (auto& drawable : m_networkedAnimator.drawablesWithZLevel(position())) {
    if (drawable.second < 0.0f)
      renderer->addDrawable(std::move(drawable.first), renderLayer(VehicleLayer::Back));
    else
      renderer->addDrawable(std::move(drawable.first), renderLayer(VehicleLayer::Front));
  }

  renderer->addAudios(m_networkedAnimatorDynamicTarget.pullNewAudios());
  renderer->addParticles(m_networkedAnimatorDynamicTarget.pullNewParticles());

  for (auto drawablePair : m_scriptedAnimator.drawables())
    renderer->addDrawable(drawablePair.first, drawablePair.second.value(renderLayer(VehicleLayer::Front)));
  renderer->addAudios(m_scriptedAnimator.pullNewAudios());
  renderer->addParticles(m_scriptedAnimator.pullNewParticles());
}

void Vehicle::renderLightSources(RenderCallback* renderer) {
  renderer->addLightSources(m_networkedAnimator.lightSources(position()));
  renderer->addLightSources(m_scriptedAnimator.lightSources());
}

List<LightSource> Vehicle::lightSources() const {
  auto lightSources = m_networkedAnimator.lightSources(position());
  return lightSources;
}

bool Vehicle::shouldDestroy() const {
  return m_shouldDestroy;
}

void Vehicle::destroy(RenderCallback* renderCallback) {
  if (renderCallback) {
    m_networkedAnimator.update(0.0, &m_networkedAnimatorDynamicTarget);

    renderCallback->addAudios(m_networkedAnimatorDynamicTarget.pullNewAudios());
    renderCallback->addParticles(m_networkedAnimatorDynamicTarget.pullNewParticles());
  }
}

Maybe<Json> Vehicle::receiveMessage(ConnectionId connectionId, String const& message, JsonArray const& args) {
  if (receiveLoungeMessage(connectionId, message, args).isValid())
    return Json();
  return m_scriptComponent.handleMessage(message, connectionId == world()->connection(), args);
}

RectF Vehicle::interactiveBoundBox() const {
  return collisionArea();
}

bool Vehicle::isInteractive() const {
  return m_interactive.get();
}

InteractAction Vehicle::interact(InteractRequest const& request) {
  auto result = m_scriptComponent.invoke<Json>("onInteraction", JsonObject{
      {"sourceId", request.sourceId},
      {"sourcePosition", jsonFromVec2F(request.sourcePosition)},
      {"interactPosition", jsonFromVec2F(request.interactPosition)}
    }).value();

  if (result.isType(Json::Type::String))
    return InteractAction(result.toString(), entityId(), Json());
  else if (!result.isNull())
    return InteractAction(result.getString(0), entityId(), result.get(1));

  Maybe<size_t> index = loungeInteract(request);
  if (index)
    return InteractAction(InteractActionType::SitDown, entityId(), *index);

  return InteractAction();
}

List<PhysicsForceRegion> Vehicle::forceRegions() const {
  List<PhysicsForceRegion> forces;
  for (auto const& p : m_forceRegions) {
    if (p.second.enabled.get()) {
      PhysicsForceRegion forceRegion = p.second.forceRegion;

      Vec2F translatePos = position();
      if (p.second.attachToPart) {
        Mat3F partTransformation = m_networkedAnimator.finalPartTransformation(p.second.attachToPart.get());
        Vec2F localTranslation = partTransformation.transformVec2(Vec2F());
        translatePos += localTranslation;
      }

      forceRegion.call([translatePos](auto& fr) { fr.translate(translatePos); });
      forces.append(std::move(forceRegion));
    }
  }
  return forces;
}

List<DamageSource> Vehicle::damageSources() const {
  List<DamageSource> sources;
  for (auto const& p : m_damageSources) {
    if (p.second.enabled.get()) {
      DamageSource damageSource = p.second.damageSource;

      if (p.second.attachToPart) {
        Mat3F partTransformation = m_networkedAnimator.finalPartTransformation(p.second.attachToPart.get());
        damageSource.damageArea.call([partTransformation](auto& da) { da.transform(partTransformation); });
      }

      damageSource.team = m_damageTeam.get();
      damageSource.sourceEntityId = entityId();

      sources.append(std::move(damageSource));
    }
  }
  return sources;
}

size_t Vehicle::movingCollisionCount() const {
  return m_movingCollisions.size();
}

Maybe<PhysicsMovingCollision> Vehicle::movingCollision(size_t positionIndex) const {
  auto const& collisionConfig = m_movingCollisions.valueAt(positionIndex);
  if (!collisionConfig.enabled.get())
    return {};

  PhysicsMovingCollision collision = collisionConfig.movingCollision;

  if (collisionConfig.attachToPart) {
    Mat3F partTransformation = m_networkedAnimator.finalPartTransformation(*collisionConfig.attachToPart);

    Vec2F localTranslation = partTransformation.transformVec2(Vec2F());
    collision.position += localTranslation;

    Mat3F localTransform = Mat3F::translation(-localTranslation) * partTransformation;
    collision.collision.transform(localTransform);
  }

  collision.position += position();

  return collision;
}

Maybe<LuaValue> Vehicle::callScript(String const& func, LuaVariadic<LuaValue> const& args) {
  return m_scriptComponent.invoke(func, args);
}

Maybe<LuaValue> Vehicle::evalScript(String const& code) {
  return m_scriptComponent.eval(code);
}

void Vehicle::setPosition(Vec2F const& position) {
  m_movementController.setPosition(position);
}

EntityRenderLayer Vehicle::loungeRenderLayer(size_t anchorPositionIndex) const {
  return renderLayer(VehicleLayer::Passenger);
}

NetworkedAnimator const* Vehicle::networkedAnimator() const {
  return &m_networkedAnimator;
}
NetworkedAnimator * Vehicle::networkedAnimator()  {
  return &m_networkedAnimator;
}

LoungeableEntity::LoungePositions * Vehicle::loungePositions(){
  return &m_loungePositions;
}

LoungeableEntity::LoungePositions const* Vehicle::loungePositions() const {
  return &m_loungePositions;
}


EntityRenderLayer Vehicle::renderLayer(VehicleLayer vehicleLayer) const {
  // Z-offset based on entity id, so vehicles don't overlap strangely.
  return m_overrideRenderLayer ? (*m_overrideRenderLayer + (unsigned)vehicleLayer) : (m_baseRenderLayer + ((EntityRenderLayer)(entityId() * 4 + (unsigned)vehicleLayer) & RenderLayerLowerMask));
}

LuaCallbacks Vehicle::makeVehicleCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("setPersistent", [this](bool persistent) {
      setPersistent(persistent);
    });

  callbacks.registerCallback("setInteractive", [this](bool interactive) {
      m_interactive.set(interactive);
    });

  callbacks.registerCallback("setDamageTeam", [this](Json damageTeam) {
      m_damageTeam.set(EntityDamageTeam(damageTeam));
    });

  callbacks.registerCallback("setDamageSourceEnabled", [this](String const& name, bool enabled) {
      m_damageSources.get(name).enabled.set(enabled);
    });

  callbacks.registerCallback("setMovingCollisionEnabled", [this](String const& name, bool enabled) {
      m_movingCollisions.get(name).enabled.set(enabled);
    });

  callbacks.registerCallback("setForceRegionEnabled", [this](String const& name, bool enabled) {
      m_forceRegions.get(name).enabled.set(enabled);
    });

  callbacks.registerCallback("destroy", [this]() {
      m_shouldDestroy = true;
    });

  callbacks.registerCallback("setAnimationParameter", [this](String name, Json value) {
      m_scriptedAnimationParameters.set(std::move(name), std::move(value));
    });

  return addLoungeableCallbacks(callbacks);
}

Json Vehicle::configValue(String const& name, Json def) const {
  return jsonMergeQueryDef(name, std::move(def), m_baseConfig, m_dynamicConfig);
}

}
