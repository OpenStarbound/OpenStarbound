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

namespace Star {

Vehicle::Vehicle(Json baseConfig, String path, Json dynamicConfig)
  : m_baseConfig(move(baseConfig)), m_path(move(path)), m_dynamicConfig(move(dynamicConfig)) {

  m_typeName = m_baseConfig.getString("name");

  setPersistent(configValue("persistent", false).toBool());
  m_clientEntityMode = ClientEntityModeNames.getLeft(configValue("clientEntityMode", "ClientSlaveOnly").toString());

  m_scriptComponent.setScript(AssetPath::relativeTo(m_path, configValue("script").toString()));
  m_scriptComponent.setUpdateDelta(configValue("scriptDelta", 1).toUInt());
  m_boundBox = jsonToRectF(configValue("boundBox"));
  m_slaveControlTimeout = configValue("slaveControlTimeout").toFloat();
  m_slaveHeartbeatTimer = GameTimer(configValue("slaveControlHeartbeat").toFloat());
  m_damageTeam.set(configValue("damageTeam").opt().apply(construct<EntityDamageTeam>()).value());
  m_interactive.set(configValue("interactive", true).toBool());

  if (auto animationScript = configValue("animationScript").optString())
    m_scriptedAnimator.setScript(*animationScript);

  for (auto const& pair : configValue("loungePositions", JsonObject()).iterateObject()) {
    auto& loungePosition = m_loungePositions[pair.first];
    loungePosition.part = pair.second.getString("part");
    loungePosition.partAnchor = pair.second.getString("partAnchor");
    loungePosition.exitBottomOffset = pair.second.opt("exitBottomOffset").apply(jsonToVec2F);
    loungePosition.armorCosmeticOverrides = pair.second.getObject("armorCosmeticOverrides", JsonObject());
    loungePosition.cursorOverride = pair.second.optString("cursorOverride");
    loungePosition.cameraFocus = pair.second.getBool("cameraFocus", false);
    loungePosition.enabled.set(pair.second.getBool("enabled", true));
    if (auto orientation = pair.second.optString("orientation"))
      loungePosition.orientation.set(LoungeOrientationNames.getLeft(*orientation));
    loungePosition.emote.set(pair.second.optString("emote"));
    loungePosition.dance.set(pair.second.optString("dance"));
    loungePosition.directives.set(pair.second.optString("directives"));
    loungePosition.statusEffects.set(pair.second.getArray("statusEffects", {}).transformed(jsonToPersistentStatusEffect));
  }

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

  m_loungePositions.sortByKey();
  for (auto& p : m_loungePositions) {
    m_netGroup.addNetElement(&p.second.enabled);
    m_netGroup.addNetElement(&p.second.orientation);
    m_netGroup.addNetElement(&p.second.emote);
    m_netGroup.addNetElement(&p.second.dance);
    m_netGroup.addNetElement(&p.second.directives);
    m_netGroup.addNetElement(&p.second.statusEffects);
  }

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
  } else {
    m_slaveHeartbeatTimer.reset();
  }

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

pair<ByteArray, uint64_t> Vehicle::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void Vehicle::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(move(data), interpolationTime);
}

void Vehicle::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void Vehicle::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

void Vehicle::update(uint64_t) {
  setTeam(m_damageTeam.get());

  if (world()->isClient()) {
    m_networkedAnimator.update(WorldTimestep, &m_networkedAnimatorDynamicTarget);
    m_networkedAnimatorDynamicTarget.updatePosition(position());
  } else {
    m_networkedAnimator.update(WorldTimestep, nullptr);
  }

  if (isMaster()) {
    m_movementController.tickMaster();
    m_scriptComponent.update(m_scriptComponent.updateDt());

    eraseWhere(m_aliveMasterConnections, [](auto& p) {
        return p.second.tick(WorldTimestep);
      });

    for (auto& loungePositionPair : m_loungePositions) {
      for (auto& p : loungePositionPair.second.masterControlState) {
        p.second.masterHeld = false;
        filter(p.second.slavesHeld, [this](ConnectionId id) {
            return m_aliveMasterConnections.contains(id);
          });
      }
    }
  } else {
    m_netGroup.tickNetInterpolation(WorldTimestep);

    m_movementController.tickSlave();

    bool heartbeat = m_slaveHeartbeatTimer.wrapTick();

    for (auto& p : m_loungePositions) {
      if (heartbeat) {
        JsonArray allControlsHeld = transform<JsonArray>(p.second.slaveNewControls, [](LoungeControl control) {
            return LoungeControlNames.getRight(control);
          });
        world()->sendEntityMessage(entityId(), "control_all", {*m_loungePositions.indexOf(p.first), move(allControlsHeld)});
      } else {
        for (auto control : p.second.slaveNewControls.difference(p.second.slaveOldControls))
          world()->sendEntityMessage(entityId(), "control_on", {*m_loungePositions.indexOf(p.first), LoungeControlNames.getRight(control)});

        for (auto control : p.second.slaveOldControls.difference(p.second.slaveNewControls))
          world()->sendEntityMessage(entityId(), "control_off", {*m_loungePositions.indexOf(p.first), LoungeControlNames.getRight(control)});
      }

      if (p.second.slaveOldAimPosition != p.second.slaveNewAimPosition)
        world()->sendEntityMessage(entityId(), "aim", {*m_loungePositions.indexOf(p.first), p.second.slaveNewAimPosition[0], p.second.slaveNewAimPosition[1]});

      p.second.slaveOldControls = take(p.second.slaveNewControls);
      p.second.slaveOldAimPosition = p.second.slaveNewAimPosition;
    }
  }

  if (world()->isClient())
    m_scriptedAnimator.update();

  if (world()->isClient())
    SpatialLogger::logPoly("world", m_movementController.collisionBody(), {255, 255, 0, 255});
}

void Vehicle::render(RenderCallback* renderer) {
  for (auto& drawable : m_networkedAnimator.drawablesWithZLevel(position())) {
    if (drawable.second < 0.0f)
      renderer->addDrawable(move(drawable.first), renderLayer(VehicleLayer::Back));
    else
      renderer->addDrawable(move(drawable.first), renderLayer(VehicleLayer::Front));
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
  m_aliveMasterConnections[connectionId] = GameTimer(m_slaveControlTimeout);
  if (message.equalsIgnoreCase("control_on")) {
    auto& loungePosition = m_loungePositions.valueAt(args.at(0).toUInt());
    loungePosition.masterControlState[LoungeControlNames.getLeft(args.at(1).toString())].slavesHeld.add(connectionId);
    return Json();
  } else if (message.equalsIgnoreCase("control_off")) {
    auto& loungePosition = m_loungePositions.valueAt(args.at(0).toUInt());
    loungePosition.masterControlState[LoungeControlNames.getLeft(args.at(1).toString())].slavesHeld.remove(connectionId);
    return Json();
  } else if (message.equalsIgnoreCase("control_all")) {
    auto& loungePosition = m_loungePositions.valueAt(args.at(0).toUInt());
    Set<LoungeControl> allControlsHeld;
    for (auto const& s : args.at(1).iterateArray())
      allControlsHeld.add(LoungeControlNames.getLeft(s.toString()));
    for (auto& p : loungePosition.masterControlState) {
      if (allControlsHeld.contains(p.first))
        p.second.slavesHeld.add(connectionId);
      else
        p.second.slavesHeld.remove(connectionId);
    }
    return Json();
  } else if (message.equalsIgnoreCase("aim")) {
    auto& loungePosition = m_loungePositions.valueAt(args.at(0).toUInt());
    loungePosition.masterAimPosition = {args.at(1).toFloat(), args.at(2).toFloat()};
    return Json();
  } else {
    return m_scriptComponent.handleMessage(message, connectionId == world()->connection(), args);
  }
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

  Maybe<size_t> index;
  for (size_t i = 0; i < m_loungePositions.size(); ++i) {
    if (!index) {
      index = i;
    } else {
      auto const& thisLounge = m_loungePositions.valueAt(i);
      if (!thisLounge.enabled.get())
        continue;

      Vec2F thisLoungePosition = *m_networkedAnimator.partPoint(thisLounge.part, thisLounge.partAnchor) + position();

      auto const& selectedLounge = m_loungePositions.valueAt(*index);
      Vec2F selectedLoungePosition = *m_networkedAnimator.partPoint(selectedLounge.part, selectedLounge.partAnchor) + position();

      if (vmagSquared(thisLoungePosition - request.interactPosition) < vmagSquared(selectedLoungePosition - request.interactPosition))
        index = i;
    }
  }

  if (index)
    return InteractAction(InteractActionType::SitDown, entityId(), *index);

  return InteractAction();
}

size_t Vehicle::anchorCount() const {
  return m_loungePositions.size();
}

LoungeAnchorConstPtr Vehicle::loungeAnchor(size_t positionIndex) const {
  auto const& positionConfig = m_loungePositions.valueAt(positionIndex);
  if (!positionConfig.enabled.get())
    return {};

  Mat3F partTransformation = m_networkedAnimator.finalPartTransformation(positionConfig.part);
  Vec2F partAnchor = jsonToVec2F(m_networkedAnimator.partProperty(positionConfig.part, positionConfig.partAnchor));

  auto loungePosition = make_shared<LoungeAnchor>();
  loungePosition->position = partTransformation.transformVec2(partAnchor) + position();
  if (positionConfig.exitBottomOffset)
    loungePosition->exitBottomPosition = partTransformation.transformVec2(partAnchor + positionConfig.exitBottomOffset.value()) + position();
  loungePosition->direction = partTransformation.determinant() > 0 ? Direction::Right : Direction::Left;
  loungePosition->angle = partTransformation.transformAngle(0.0f);
  if (loungePosition->direction == Direction::Left)
    loungePosition->angle += Constants::pi;
  loungePosition->controllable = true;
  loungePosition->loungeRenderLayer = renderLayer(VehicleLayer::Passenger);
  loungePosition->orientation = positionConfig.orientation.get();
  loungePosition->emote = positionConfig.emote.get();
  loungePosition->dance = positionConfig.dance.get();
  loungePosition->directives = positionConfig.directives.get();
  loungePosition->statusEffects = positionConfig.statusEffects.get();
  loungePosition->armorCosmeticOverrides = positionConfig.armorCosmeticOverrides;
  loungePosition->cursorOverride = positionConfig.cursorOverride;
  loungePosition->cameraFocus = positionConfig.cameraFocus;
  return loungePosition;
}

void Vehicle::loungeControl(size_t index, LoungeControl loungeControl) {
  auto& loungePosition = m_loungePositions.valueAt(index);
  if (isSlave())
    loungePosition.slaveNewControls.add(loungeControl);
  else
    loungePosition.masterControlState[loungeControl].masterHeld = true;
}

void Vehicle::loungeAim(size_t index, Vec2F const& aimPosition) {
  auto& loungePosition = m_loungePositions.valueAt(index);
  if (isSlave())
    loungePosition.slaveNewAimPosition = aimPosition;
  else
    loungePosition.masterAimPosition = aimPosition;
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
      forces.append(move(forceRegion));
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

      sources.append(move(damageSource));
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

EntityRenderLayer Vehicle::renderLayer(VehicleLayer vehicleLayer) const {
  // Z-offset based on entity id, so vehicles don't overlap strangely.
  return RenderLayerVehicle + ((EntityRenderLayer)(entityId() * 4 + (unsigned)vehicleLayer) & RenderLayerLowerMask);
}

LuaCallbacks Vehicle::makeVehicleCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("controlHeld", [this](String const& loungeName, String const& controlName) {
      auto const& mc = m_loungePositions.get(loungeName).masterControlState[LoungeControlNames.getLeft(controlName)];
      return mc.masterHeld || !mc.slavesHeld.empty();
    });

  callbacks.registerCallback( "aimPosition", [this](String const& loungeName) {
      return m_loungePositions.get(loungeName).masterAimPosition;
    });

  callbacks.registerCallback("entityLoungingIn", [this](String const& name) -> LuaValue {
      auto entitiesIn = entitiesLoungingIn(*m_loungePositions.indexOf(name));
      if (entitiesIn.empty())
        return LuaNil;
      return LuaInt(entitiesIn.first());
    });

  callbacks.registerCallback("setLoungeEnabled", [this](String const& name, bool enabled) {
      m_loungePositions.get(name).enabled.set(enabled);
    });

  callbacks.registerCallback("setLoungeOrientation", [this](String const& name, String const& orientation) {
      m_loungePositions.get(name).orientation.set(LoungeOrientationNames.getLeft(orientation));
    });

  callbacks.registerCallback("setLoungeEmote", [this](String const& name, Maybe<String> emote) {
      m_loungePositions.get(name).emote.set(move(emote));
    });

  callbacks.registerCallback("setLoungeDance", [this](String const& name, Maybe<String> dance) {
      m_loungePositions.get(name).dance.set(move(dance));
    });

  callbacks.registerCallback("setLoungeDirectives", [this](String const& name, Maybe<String> directives) {
      m_loungePositions.get(name).directives.set(move(directives));
    });

  callbacks.registerCallback("setLoungeStatusEffects", [this](String const& name, JsonArray const& statusEffects) {
      m_loungePositions.get(name).statusEffects.set(statusEffects.transformed(jsonToPersistentStatusEffect));
    });

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
      m_scriptedAnimationParameters.set(move(name), move(value));
    });

  return callbacks;
}

Json Vehicle::configValue(String const& name, Json def) const {
  return jsonMergeQueryDef(name, move(def), m_baseConfig, m_dynamicConfig);
}

}
