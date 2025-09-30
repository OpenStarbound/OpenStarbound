#include "StarObject.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorld.hpp"
#include "StarLexicalCast.hpp"
#include "StarRoot.hpp"
#include "StarLogging.hpp"
#include "StarDamageManager.hpp"
#include "StarTreasure.hpp"
#include "StarItemDrop.hpp"
#include "StarItemDescriptor.hpp"
#include "StarObjectDatabase.hpp"
#include "StarMixer.hpp"
#include "StarEntityRendering.hpp"
#include "StarAssets.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarParticleDatabase.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarScriptedAnimatorLuaBindings.hpp"

namespace Star {

Object::Object(ObjectConfigConstPtr config, Json const& parameters) {
  m_config = config;
  if (!parameters.isNull())
    m_parameters.reset(parameters.toObject());

  auto jOrientations = m_parameters.ptr("customOrientations");
  if (jOrientations && jOrientations->isType(Json::Type::Array)) {
    JsonArray base = m_config->config.get("orientations").toArray();
    auto orientations = jOrientations->toArray();
    for (size_t i = 0; i != orientations.size(); ++i)
      base.set(i, jsonMergeNulling(base.get(i), orientations.get(i)));
    m_orientations = ObjectDatabase::parseOrientations(m_config->path, base);
  }

  m_animationTimer = 0.0f;
  m_currentFrame = 0;

  m_lightFlickering = m_config->lightFlickering;

  m_tileDamageStatus = make_shared<EntityTileDamageStatus>();

  m_orientationIndex = NPos;

  m_interactive.set(!configValue("interactAction", Json()).isNull());

  m_broken = false;
  m_unbreakable = m_config->unbreakable || configValue("unbreakable", false).toBool();
  m_direction.set(Direction::Left);

  m_health.set(m_config->health);

  m_currentFrame = -1;

  if (m_config->animationConfig)
    m_networkedAnimator = make_shared<NetworkedAnimator>(m_config->animationConfig, m_config->path);
  else
    m_networkedAnimator = make_shared<NetworkedAnimator>();

  if (m_config->damageTeam.type != TeamType::Null) {
    setTeam(m_config->damageTeam);
  } else {
    setTeam(EntityDamageTeam(TeamType::Environment));
  }

  auto inputNodes = configValue("inputNodes", JsonArray());
  auto inputNodeConfigs = configValue("inputNodesConfig", JsonArray());
  for (auto i = 0; i != inputNodes.size(); i++)
    m_inputNodes.append(InputNode(inputNodes.get(i), inputNodeConfigs.get(i, JsonObject())));

  auto outputNodes = configValue("outputNodes", JsonArray());
  auto outputNodeConfigs = configValue("outputNodesConfig", JsonArray());
  for (auto i = 0; i != outputNodes.size(); i++)
    m_outputNodes.append(OutputNode(outputNodes.get(i), outputNodeConfigs.get(i, JsonObject())));

  m_offeredQuests.set(configValue("offeredQuests", JsonArray()).toArray().transformed(&QuestArcDescriptor::fromJson));
  m_turnInQuests.set(jsonToStringSet(configValue("turnInQuests", JsonArray())));
  if (!m_offeredQuests.get().empty() || !m_turnInQuests.get().empty())
    m_interactive.set(true);

  setUniqueId(configValue("uniqueId").optString());

  m_netGroup.addNetElement(&m_parameters);
  m_netGroup.addNetElement(&m_uniqueIdNetState);
  m_netGroup.addNetElement(&m_interactive);
  m_netGroup.addNetElement(&m_materialSpaces);
  m_netGroup.addNetElement(&m_xTilePosition);
  m_netGroup.addNetElement(&m_yTilePosition);
  m_netGroup.addNetElement(&m_direction);
  m_netGroup.addNetElement(&m_health);
  m_netGroup.addNetElement(&m_orientationIndexNetState);
  m_netGroup.addNetElement(&m_netImageKeys);
  m_netGroup.addNetElement(&m_soundEffectEnabled);
  m_netGroup.addNetElement(&m_lightSourceColor);
  m_netGroup.addNetElement(&m_newChatMessageEvent);
  m_netGroup.addNetElement(&m_chatMessage);
  m_netGroup.addNetElement(&m_chatPortrait);
  m_netGroup.addNetElement(&m_chatConfig);

  for (auto& i : m_inputNodes) {
    m_netGroup.addNetElement(&i.connections);
    m_netGroup.addNetElement(&i.state);
  }
  for (auto& o : m_outputNodes) {
    m_netGroup.addNetElement(&o.connections);
    m_netGroup.addNetElement(&o.state);
  }

  m_netGroup.addNetElement(&m_offeredQuests);
  m_netGroup.addNetElement(&m_turnInQuests);
  m_netGroup.addNetElement(&m_damageSources);

  // don't interpolate scripted animation parameters
  m_netGroup.addNetElement(&m_scriptedAnimationParameters, false);

  m_netGroup.addNetElement(m_tileDamageStatus.get());
  m_netGroup.addNetElement(m_networkedAnimator.get());

  m_netGroup.setNeedsLoadCallback(bind(&Object::getNetStates, this, _1));
  m_netGroup.setNeedsStoreCallback(bind(&Object::setNetStates, this));

  m_clientEntityMode = ClientEntityModeNames.getLeft(configValue("clientEntityMode", "ClientSlaveOnly").toString());
}

Json Object::diskStore() const {
  return writeStoredData().setAll({{"name", m_config->name}, {"parameters", m_parameters.baseMap()}});
}

ByteArray Object::netStore(NetCompatibilityRules rules) {
  DataStreamBuffer ds;
  ds.setStreamCompatibilityVersion(rules);
  ds.write(m_config->name);
  ds.write<Json>(m_parameters.baseMap());
  return ds.takeData();
}

EntityType Object::entityType() const {
  return EntityType::Object;
}

ClientEntityMode Object::clientEntityMode() const {
  return m_clientEntityMode;
}

void Object::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);
  // Only try and find a new orientation if we do not already have one,
  // otherwise we may have a valid orientation that depends on non-tile data
  // that is not loaded yet.
  if (m_orientationIndex == NPos) {
    updateOrientation();
  } else if (auto orientation = currentOrientation()) {
    // update direction in case orientation config direction has changed
    if (orientation->directionAffinity)
      m_direction.set(*orientation->directionAffinity);
    m_materialSpaces.set(orientation->materialSpaces);
  }

  m_orientationDrawablesCache.reset();

  // This is stupid and we should only have to deal with the new directives parameter, but blah blah backwards compatibility.
  auto colorName = configValue("color", "default").toString().takeUtf8();
  auto colorEnd = colorName.find('?');
  if (colorEnd != NPos) {
    size_t suffixBegin = colorName.rfind('?');
    String colorDirectives;
    std::string colorSuffix = suffixBegin == NPos ? "" : colorName.substr(suffixBegin);
    if (colorSuffix.empty() && colorSuffix.rfind("?replace", 0) != 0)
      colorDirectives = colorName.substr(colorEnd);
    else
      colorDirectives = colorName.substr(colorEnd, suffixBegin - colorEnd);

    m_colorSuffix = std::move(colorSuffix);
    m_colorDirectives = std::move(colorDirectives);
  }
  else
    m_colorDirectives = m_colorSuffix = "";

  m_directives = "";
  if (auto directives = configValue("")) {
    if (directives.isType(Json::Type::String))
      m_directives.parse(directives.toString());
  }

  if (isMaster()) {
    setImageKey("color", colorName);
    for (auto p : configValue("defaultImageKeys", JsonObject()).toObject())
      setImageKey(p.first, p.second.toString());

    if (m_config->lightColors.contains(colorName))
      m_lightSourceColor.set(m_config->lightColors.get(colorName));
    else
      m_lightSourceColor.set(Color::Clear);

    m_soundEffectEnabled.set(true);

    m_liquidCheckTimer = GameTimer(m_config->liquidCheckInterval);
    m_liquidCheckTimer.setDone();

    setKeepAlive(configValue("keepAlive", false).toBool());

    auto jScripts = configValue("scripts", JsonArray());
    if (jScripts.isType(Json::Type::Array))
      m_scriptComponent.setScripts(jsonToStringList(jScripts).transformed(bind(AssetPath::relativeTo, m_config->path, _1)));
    else
      m_scriptComponent.setScripts(m_config->scripts);
    m_scriptComponent.setUpdateDelta(configValue("scriptDelta", 5).toInt());

    m_scriptComponent.addCallbacks("object", makeObjectCallbacks());
    m_scriptComponent.addCallbacks("config", LuaBindings::makeConfigCallbacks(bind(&Object::configValue, this, _1, _2)));
    m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptComponent.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(m_networkedAnimator.get()));
    m_scriptComponent.init(world);
  }

  if (world->isClient()) {
    m_scriptedAnimator.setScripts(m_config->animationScripts);

    m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(m_networkedAnimator.get(),
      [this](String const& name, Json const& defaultValue) -> Json {
        return m_scriptedAnimationParameters.value(name, defaultValue);
      }));
    m_scriptedAnimator.addCallbacks("objectAnimator", makeAnimatorObjectCallbacks());
    m_scriptedAnimator.addCallbacks("config", LuaBindings::makeConfigCallbacks(bind(&Object::configValue, this, _1, _2)));
    m_scriptedAnimator.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptedAnimator.init(world);
  }

  m_xTilePosition.set(world->geometry().xwrap((int)m_xTilePosition.get()));

  // Compute all the relevant animation information after the final orientation
  // has been selected and after the script is initialized

  for (auto const& pair : configValue("animationParts", JsonObject()).iterateObject())
    m_networkedAnimator->setPartTag(pair.first, "partImage", pair.second.toString());

  m_animationPosition = jsonToVec2F(configValue("animationPosition", JsonArray{0, 0})) / TilePixels;

  m_networkedAnimator->setFlipped(false);
  m_animationCenterLine = configValue("animationCenterLine", Drawable::boundBoxAll(m_networkedAnimator->drawables(), false).center()[0]).toFloat();
  m_networkedAnimator->setFlipped(direction() == Direction::Left, m_animationCenterLine);

  // Don't animate the initial state when first spawned IF you're dumb, which by default
  // you would be, and don't know how to use transition and static states properly. Someday
  // I'll be brave and delete shit garbage entirely and we'll see what breaks.
  if (configValue("forceFinishAnimationsInInit", true) != false)
    m_networkedAnimator->finishAnimations();
}

void Object::uninit() {
  if (isMaster()) {
    m_scriptComponent.uninit();
    m_scriptComponent.removeCallbacks("object");
    m_scriptComponent.removeCallbacks("config");
    m_scriptComponent.removeCallbacks("entity");
    m_scriptComponent.removeCallbacks("animator");
  }

  if (world()->isClient()) {
    m_scriptedAnimator.uninit();
    m_scriptedAnimator.removeCallbacks("animationConfig");
    m_scriptedAnimator.removeCallbacks("objectAnimator");
    m_scriptedAnimator.removeCallbacks("config");
    m_scriptedAnimator.removeCallbacks("entity");
  }

  if (m_soundEffect)
    m_soundEffect->stop();

  Entity::uninit();
}

List<LightSource> Object::lightSources() const {
  List<LightSource> lights;
  lights.appendAll(m_networkedAnimator->lightSources(position() + m_animationPosition));

  auto orientation = currentOrientation();
  if (!m_lightSourceColor.get().isClear() && orientation) {
    Color color = m_lightSourceColor.get();
    if (m_lightFlickering)
      color.setValue(clamp(color.value() * m_lightFlickering->value(SinWeightOperator<float>()), 0.0f, 1.0f));

    LightSource lightSource;
    lightSource.position = position() + centerOfTile(orientation->lightPosition);
    lightSource.color = color.toRgbF();
    lightSource.type = m_config->lightType;
    lightSource.pointBeam = m_config->pointBeam;
    lightSource.beamAngle = orientation->beamAngle;
    lightSource.beamAmbience = m_config->beamAmbience;

    lights.append(lightSource);
  }

  return lights;
}

Vec2F Object::position() const {
  return Vec2F(m_xTilePosition.get(), m_yTilePosition.get());
}

RectF Object::metaBoundBox() const {
  if (auto orientation = currentOrientation()) {
    // default metaboundbox extends the bounding box of the orientation's
    // spaces by one block
    return orientation->metaBoundBox.value(RectF(Vec2F(orientation->boundBox.min()) - Vec2F(1, 1), Vec2F(orientation->boundBox.max()) + Vec2F(2, 2)));
  } else {
    return RectF(-1, -1, 1, 1);
  }
}

pair<ByteArray, uint64_t> Object::writeNetState(uint64_t fromVersion, NetCompatibilityRules rules) {
  return m_netGroup.writeNetState(fromVersion, rules);
}

void Object::readNetState(ByteArray data, float interpolationTime, NetCompatibilityRules rules) {
  m_netGroup.readNetState(data, interpolationTime, rules);
}

Vec2I Object::tilePosition() const {
  return Vec2I(m_xTilePosition.get(), m_yTilePosition.get());
}

void Object::setTilePosition(Vec2I const& pos) {
  if (m_xTilePosition.get() != pos[0] || m_yTilePosition.get() != pos[1]) {
    m_xTilePosition.set(pos[0]);
    m_yTilePosition.set(pos[1]);
    if (inWorld())
      updateOrientation();
  }
}

Direction Object::direction() const {
  return m_direction.get();
}

void Object::setDirection(Direction direction) {
  m_direction.set(direction);
}

void Object::updateOrientation() {
  setOrientationIndex(m_config->findValidOrientation(world(), tilePosition(), m_direction.get()));
  if (auto orientation = currentOrientation()) {
    if (orientation->directionAffinity)
      m_direction.set(*orientation->directionAffinity);
    m_materialSpaces.set(orientation->materialSpaces);
  }
  resetEmissionTimers();
}

List<Vec2I> Object::anchorPositions() const {
  if (auto orientation = currentOrientation()) {
    List<Vec2I> positions;
    for (auto anchor : orientation->anchors)
      positions.append(anchor.position + tilePosition());
    return positions;
  } else {
    return {};
  }
}

List<Vec2I> Object::spaces() const {
  if (auto orientation = currentOrientation())
    return orientation->spaces;
  else
    return {};
}

List<MaterialSpace> Object::materialSpaces() const {
  return m_materialSpaces.get();
}

List<Vec2I> Object::roots() const {
  if (m_config->rooting) {
    if (auto orientation = currentOrientation()) {
      List<Vec2I> res;
      for (auto anchor : orientation->anchors)
        res.append(anchor.position);
      return res;
    }
  }
  return {};
}

void Object::update(float dt, uint64_t) {
  if (!inWorld())
    return;

  if (isMaster()) {
    m_tileDamageStatus->recover(m_config->tileDamageParameters, dt);

    if (m_liquidCheckTimer.wrapTick())
      checkLiquidBroken();

    if (auto orientation = currentOrientation()) {
      auto frame = clamp<int>(std::floor(m_animationTimer / orientation->animationCycle * orientation->frames), 0, orientation->frames - 1);
      if (m_currentFrame != frame) {
        m_currentFrame = frame;
        setImageKey("frame", toString(frame));
      }

      m_animationTimer = std::fmod(m_animationTimer + dt, orientation->animationCycle);
    }

    m_networkedAnimator->update(dt, nullptr);
    m_networkedAnimator->setFlipped(direction() == Direction::Left, m_animationCenterLine);

    m_scriptComponent.update(m_scriptComponent.updateDt(dt));

  } else {
    m_networkedAnimator->update(dt, &m_networkedAnimatorDynamicTarget);
    m_networkedAnimatorDynamicTarget.updatePosition(position() + m_animationPosition);
  }

  if (m_lightFlickering)
    m_lightFlickering->update(dt);

  for (auto& timer : m_emissionTimers)
    timer.tick(dt);

  if (world()->isClient())
    m_scriptedAnimator.update();
}

void Object::render(RenderCallback* renderCallback) {
  renderParticles(renderCallback);
  renderSounds(renderCallback);

  for (auto const& imageKeyPair : m_imageKeys)
    m_networkedAnimator->setGlobalTag(imageKeyPair.first, imageKeyPair.second);

  renderCallback->addAudios(m_networkedAnimatorDynamicTarget.pullNewAudios());
  renderCallback->addParticles(m_networkedAnimatorDynamicTarget.pullNewParticles());

  if (m_networkedAnimator->constParts().size() > 0) {
    renderCallback->addDrawables(m_networkedAnimator->drawables(position() + m_animationPosition + damageShake()), renderLayer());
  } else {
    if (m_orientationIndex != NPos)
      renderCallback->addDrawables(orientationDrawables(m_orientationIndex), renderLayer(), position());
  }

  for (auto drawablePair : m_scriptedAnimator.drawables())
    renderCallback->addDrawable(drawablePair.first, drawablePair.second.value(renderLayer()));
  renderCallback->addParticles(m_scriptedAnimator.pullNewParticles());
  renderCallback->addAudios(m_scriptedAnimator.pullNewAudios());
}

void Object::renderLightSources(RenderCallback* renderCallback) {
  renderLights(renderCallback);
  renderCallback->addLightSources(m_scriptedAnimator.lightSources());
}

bool Object::damageTiles(List<Vec2I> const&, Vec2F const&, TileDamage const& tileDamage) {
  if (m_unbreakable)
    return false;
  m_tileDamageStatus->damage(m_config->tileDamageParameters, tileDamage);
  if (m_tileDamageStatus->dead())
    m_broken = true;
  return m_broken;
}

bool Object::canBeDamaged() const {
  return !m_unbreakable;
}

bool Object::checkBroken() {
  if (!m_broken && !m_unbreakable) {
    auto orientation = currentOrientation();
    if (orientation) {
      if (!orientation->anchorsValid(world(), tilePosition()))
        m_broken = true;
    } else {
      m_broken = true;
    }
  }
  return m_broken;
}

bool Object::shouldDestroy() const {
  return m_broken || (m_health.get() <= 0);
}

void Object::destroy(RenderCallback* renderCallback) {
  bool doSmash = m_health.get() <= 0 || configValue("smashOnBreak", m_config->smashOnBreak).toBool();

  if (isMaster()) {
    m_scriptComponent.invoke("die", m_health.get() <= 0);

    try {
      if (doSmash) {
        auto smashDropPool = configValue("smashDropPool", "").toString();
        if (!smashDropPool.empty()) {
          for (auto const& treasureItem : Root::singleton().treasureDatabase()->createTreasure(smashDropPool, world()->threatLevel()))
            world()->addEntity(ItemDrop::createRandomizedDrop(treasureItem, position()));
        } else if (!m_config->smashDropOptions.empty()) {
          List<ItemDescriptor> drops;
          auto dropOption = Random::randFrom(m_config->smashDropOptions);
          for (auto o : dropOption)
            world()->addEntity(ItemDrop::createRandomizedDrop(o, position()));
        }
      } else {
        auto breakDropPool = configValue("breakDropPool", "").toString();
        if (!breakDropPool.empty()) {
          for (auto const& treasureItem : Root::singleton().treasureDatabase()->createTreasure(breakDropPool, world()->threatLevel()))
            world()->addEntity(ItemDrop::createRandomizedDrop(treasureItem, position()));
        } else if (!m_config->breakDropOptions.empty()) {
          List<ItemDescriptor> drops;
          auto dropOption = Random::randFrom(m_config->breakDropOptions);
          for (auto o : dropOption)
            world()->addEntity(ItemDrop::createRandomizedDrop(o, position()));
        } else if (m_config->hasObjectItem) {
          ItemDescriptor objectItem(m_config->name, 1);
          if (configValue("retainObjectParametersInItem", m_config->retainObjectParametersInItem).optBool().value()) {
            auto parameters = m_parameters.baseMap();
            parameters.remove("owner");
            parameters["scriptStorage"] = m_scriptComponent.getScriptStorage();
            objectItem = objectItem.applyParameters(parameters);
          }
          world()->addEntity(ItemDrop::createRandomizedDrop(objectItem, position()));
        }
      }
    } catch (StarException const& e) {
      Logger::warn("Invalid dropID in entity death. {}", outputException(e, false));
    }
  }

  if (renderCallback && doSmash && !m_config->smashSoundOptions.empty()) {
    auto audio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(Random::randFrom(m_config->smashSoundOptions)));
    renderCallback->addAudios({std::move(audio)}, position());
  }

  if (renderCallback && doSmash && !m_config->smashParticles.empty()) {
    auto& root = Root::singleton();
    List<Particle> particles;
    for (auto const& config : m_config->smashParticles) {
      auto creator = root.particleDatabase()->particleCreator(config.get("particle"), m_config->path);
      unsigned count = config.getUInt("count", 1);
      Vec2F offset = jsonToVec2F(config.get("offset", JsonArray{0, 0}));
      bool flip = config.getBool("flip", false);
      for (unsigned i = 0; i < count; ++i) {
        Particle particle = creator();
        particle.position += offset;
        if (flip)
          particle.flip = !particle.flip;
        if (m_direction.get() == Direction::Left) {
          particle.position[0] *= -1;
          particle.velocity[0] *= -1;
          particle.flip = !particle.flip;
        }
        particle.translate(position() + volume().center());
        particles.append(std::move(particle));
      }
    }
    renderCallback->addParticles(particles);
  }

  if (m_soundEffect)
    m_soundEffect->stop(1.0f);
}

String Object::name() const {
  return m_config->name;
}

String Object::shortDescription() const {
  return configValue("shortdescription", name()).toString();
}

String Object::description() const {
  return configValue("description", shortDescription()).toString();
}

bool Object::inspectable() const {
  return m_config->scannable;
}

Maybe<String> Object::inspectionLogName() const {
  return configValue("inspectionLogName").optString().value(m_config->name);
}

Maybe<String> Object::inspectionDescription(String const& species) const {
  return configValue("inspectionDescription").optString()
    .orMaybe(configValue(strf("{}Description", species)).optString())
    .value(description());
}

String Object::category() const {
  return m_config->category;
}

ObjectOrientationPtr Object::currentOrientation() const {
  if (m_orientationIndex != NPos)
    return const_cast<ObjectOrientationPtr&>(getOrientations().at(m_orientationIndex));
  else
    return {};
}

List<Drawable> Object::cursorHintDrawables() const {
  if (configValue("placementImage")) {
    String placementImage = configValue("placementImage").toString();
    if (m_direction.get() == Direction::Left)
      placementImage += "?flipx";
    Drawable imageDrawable = Drawable::makeImage(AssetPath::relativeTo(m_config->path, placementImage),
        1.0 / TilePixels, false, jsonToVec2F(configValue("placementImagePosition", jsonFromVec2F(Vec2F()))) / TilePixels);
    return {imageDrawable};
  } else {
    if (m_orientationIndex != NPos) {
      return orientationDrawables(m_orientationIndex);
    } else {
      // If we aren't in a valid orientation, still need to draw something at
      // the cursor.  Draw the first orientation whose direction affinity
      // matches our current direction, or if that fails just the first
      // orientation.
      List<Drawable> result;
      auto& orientations = getOrientations();
      for (size_t i = 0; i < orientations.size(); ++i) {
        if (orientations[i]->directionAffinity && *orientations[i]->directionAffinity == m_direction.get()) {
          result = orientationDrawables(i);
          break;
        }
      }
      if (result.empty())
        result = orientationDrawables(0);
      return result;
    }
  }
}

void Object::getNetStates(bool initial) {
  setUniqueId(m_uniqueIdNetState.get());
  if (m_orientationIndex != m_orientationIndexNetState.get())
    setOrientationIndex(m_orientationIndexNetState.get());

  if (m_newChatMessageEvent.pullOccurred() && !initial) {
    if (m_chatPortrait.get().empty())
      m_pendingChatActions.append(SayChatAction{entityId(), m_chatMessage.get(), mouthPosition()});
    else
      m_pendingChatActions.append(PortraitChatAction{entityId(), m_chatPortrait.get(), m_chatMessage.get(), mouthPosition(), m_chatConfig.get()});
  }

  if (m_netImageKeys.pullUpdated()) {
    m_imageKeys.merge(m_netImageKeys.baseMap(), true);
    m_orientationDrawablesCache.reset();
  }
}

void Object::setNetStates() {
  m_uniqueIdNetState.set(uniqueId());
  m_orientationIndexNetState.set(m_orientationIndex);
}

List<QuestArcDescriptor> Object::offeredQuests() const {
  return m_offeredQuests.get();
}

StringSet Object::turnInQuests() const {
  return m_turnInQuests.get();
}

Vec2F Object::questIndicatorPosition() const {
  if (auto orientation = currentOrientation()) {
    auto pos = position() + Vec2F(orientation->boundBox.center()[0], orientation->boundBox.max()[1] + 2.5);
    if (!(orientation->boundBox.size()[0] % 2))
      pos[0] += 0.5;
    if (auto configPosition = configValue("questIndicatorPosition", Json())) {
      auto indicatorOffset = jsonToVec2F(configPosition);
      if (m_direction.get() == Direction::Left)
        indicatorOffset[0] = -indicatorOffset[0];
      pos += indicatorOffset;
    }
    return pos;
  } else {
    return position();
  }
}

Maybe<Json> Object::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  return m_scriptComponent.handleMessage(message, sendingConnection == world()->connection(), args);
}

Json Object::configValue(String const& name, Json const& def) const {
  if (auto orientation = currentOrientation())
    return jsonMergeQueryDef(name, def, m_config->config, orientation->config, m_parameters.baseMap());
  else
    return jsonMergeQueryDef(name, def, m_config->config, m_parameters.baseMap());
}

ObjectConfigConstPtr Object::config() const {
  return m_config;
}

void Object::readStoredData(Json const& diskStore) {
  setUniqueId(diskStore.optString("uniqueId"));
  setTilePosition(jsonToVec2I(diskStore.get("tilePosition")));
  setOrientationIndex(jsonToSize(diskStore.get("orientationIndex")));
  m_direction.set(DirectionNames.getLeft(diskStore.getString("direction")));

  m_interactive.set(diskStore.getBool("interactive", !configValue("interactAction", Json()).isNull()));

  m_scriptComponent.setScriptStorage(diskStore.getObject("scriptStorage", JsonObject()));

  JsonArray inputNodes = diskStore.getArray("inputWireNodes");
  for (size_t i = 0; i < m_inputNodes.size(); ++i) {
    if (i < inputNodes.size()) {
      auto& in = m_inputNodes[i];
      List<WireConnection> connections;
      for (auto const& conn : inputNodes[i].getArray("connections"))
        connections.append(WireConnection{jsonToVec2I(conn.get(0)), (size_t)conn.get(1).toUInt()});
      in.connections.set(std::move(connections));
      in.state.set(inputNodes[i].getBool("state"));
    }
  }

  JsonArray outputNodes = diskStore.getArray("outputWireNodes");
  for (size_t i = 0; i < m_outputNodes.size(); ++i) {
    if (i < outputNodes.size()) {
      auto& in = m_outputNodes[i];
      List<WireConnection> connections;
      for (auto const& conn : outputNodes[i].getArray("connections"))
        connections.append(WireConnection{jsonToVec2I(conn.get(0)), (size_t)conn.get(1).toUInt()});
      in.connections.set(std::move(connections));
      in.state.set(outputNodes[i].getBool("state"));
    }
  }
}

Json Object::writeStoredData() const {
  JsonArray inputNodes;
  for (size_t i = 0; i < m_inputNodes.size(); ++i) {
    auto const& in = m_inputNodes[i];
    JsonArray connections;
    for (auto const& node : in.connections.get())
      connections.append(JsonArray{jsonFromVec2I(node.entityLocation), node.nodeIndex});

    inputNodes.append(JsonObject{
        {"connections", std::move(connections)},
        {"state", in.state.get()}
      });
  }

  JsonArray outputNodes;
  for (size_t i = 0; i < m_outputNodes.size(); ++i) {
    auto const& in = m_outputNodes[i];
    JsonArray connections;
    for (auto const& node : in.connections.get())
      connections.append(JsonArray{jsonFromVec2I(node.entityLocation), node.nodeIndex});

    outputNodes.append(JsonObject{
        {"connections", std::move(connections)},
        {"state", in.state.get()}
      });
  }

  return JsonObject{
    {"uniqueId", jsonFromMaybe(uniqueId())},
    {"tilePosition", jsonFromVec2I(tilePosition())},
    {"orientationIndex", jsonFromSize(m_orientationIndex)},
    {"direction", DirectionNames.getRight(m_direction.get())},
    {"scriptStorage", m_scriptComponent.getScriptStorage()},
    {"interactive", m_interactive.get()},
    {"inputWireNodes", std::move(inputNodes)},
    {"outputWireNodes", std::move(outputNodes)}
  };
}

void Object::breakObject(bool smash) {
  m_broken = true;
  if (smash)
    m_health.set(0.0f);
}

size_t Object::nodeCount(WireDirection direction) const {
  if (direction == WireDirection::Input)
    return m_inputNodes.size();
  else
    return m_outputNodes.size();
}

Vec2I Object::nodePosition(WireNode wireNode) const {
  if (wireNode.direction == WireDirection::Input)
    return m_inputNodes.at(wireNode.nodeIndex).position;
  else
    return m_outputNodes.at(wireNode.nodeIndex).position;
}

List<WireConnection> Object::connectionsForNode(WireNode wireNode) const {
  if (wireNode.direction == WireDirection::Input)
    return m_inputNodes.at(wireNode.nodeIndex).connections.get();
  else
    return m_outputNodes.at(wireNode.nodeIndex).connections.get();
}

bool Object::nodeState(WireNode wireNode) const {
  if (wireNode.direction == WireDirection::Input)
    return m_inputNodes.at(wireNode.nodeIndex).state.get();
  else
    return m_outputNodes.at(wireNode.nodeIndex).state.get();
}
String Object::nodeIcon(WireNode wireNode) const {
  if (wireNode.direction == WireDirection::Input)
    return m_inputNodes.at(wireNode.nodeIndex).icon;
  else
    return m_outputNodes.at(wireNode.nodeIndex).icon;
}

Color Object::nodeColor(WireNode wireNode) const { // only output nodes determine color
  if (wireNode.direction == WireDirection::Input)
    return m_inputNodes.at(wireNode.nodeIndex).color;
  else
    return m_outputNodes.at(wireNode.nodeIndex).color;
}

void Object::addNodeConnection(WireNode wireNode, WireConnection nodeConnection) {
  if (wireNode.direction == WireDirection::Input) {
    if (m_inputNodes.empty()) {
      Logger::info("Tried to add wire connection to input node on object with no input nodes");
      return;
    }
    m_inputNodes.at(wireNode.nodeIndex).connections.update([&](auto& list) {
        if (list.contains(nodeConnection))
          return false;
        list.append(nodeConnection);
        return true;
      });
  } else {
    if (m_outputNodes.empty()) {
      Logger::info("Tried to add wire connection to output node on object with no output nodes");
      return;
    }
    m_outputNodes.at(wireNode.nodeIndex).connections.update([&](auto& list) {
        if (list.contains(nodeConnection))
          return false;
        list.append(nodeConnection);
        return true;
      });
  }
  m_scriptComponent.invoke("onNodeConnectionChange");
}

void Object::removeNodeConnection(WireNode wireNode, WireConnection nodeConnection) {
  if (wireNode.direction == WireDirection::Input) {
    m_inputNodes.at(wireNode.nodeIndex).connections.update([&](auto& list) {
        return list.remove(nodeConnection);
      });
  } else {
    m_outputNodes.at(wireNode.nodeIndex).connections.update([&](auto& list) {
        return list.remove(nodeConnection);
      });
  }
  m_scriptComponent.invoke("onNodeConnectionChange");
}

void Object::evaluate(WireCoordinator* coordinator) {
  for (size_t i = 0; i < m_inputNodes.size(); ++i) {
    auto& in = m_inputNodes[i];
    bool nextState = false;
    for (auto const& connection : in.connections.get())
      nextState |= coordinator->readInputConnection(connection);

    if (in.state.get() != nextState) {
      in.state.set(nextState);
      m_scriptComponent.invoke("onInputNodeChange", JsonObject{
          {"node", i},
          {"level", nextState}
        });
    }
  }
}

void Object::setImageKey(String const& name, String const& value) {
  if (!isSlave())
    m_netImageKeys.set(name, value);

  if (auto p = m_imageKeys.ptr(name)) {
    if (*p != value) {
      *p = value;
      m_orientationDrawablesCache.reset();
    }
  } else {
    m_imageKeys.set(name, value);
    m_orientationDrawablesCache.reset();
  }
}

void Object::resetEmissionTimers() {
  m_emissionTimers.clear();
  if (auto orientation = currentOrientation())
    for (size_t i = 0; i < orientation->particleEmitters.size(); i++)
      m_emissionTimers.append(GameTimer());
}

size_t Object::orientationIndex() const {
  return m_orientationIndex;
}

void Object::setOrientationIndex(size_t orientationIndex) {
  m_orientationIndex = orientationIndex;
}

PolyF Object::volume() const {
  if (auto orientation = currentOrientation()) {
    RectF box = RectF(orientation->boundBox);
    box.max()[0]++;
    box.max()[1]++;
    return PolyF(box);
  } else {
    return PolyF(RectF(0, 0, 1, 1));
  }
}

float Object::liquidFillLevel() const {
  if (auto orientation = currentOrientation())
    return spacesLiquidFillLevel(orientation->spaces);

  return 0;
}

bool Object::biomePlaced() const {
  return m_config->biomePlaced;
}

NetworkedAnimator const* Object::networkedAnimator() const {
  return m_networkedAnimator.get();
}
NetworkedAnimator * Object::networkedAnimator()  {
  return m_networkedAnimator.get();
}

LuaCallbacks Object::makeObjectCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("name", [this]() {
      return name();
    });

  callbacks.registerCallback("direction", [this]() {
      return numericalDirection(direction());
    });

  callbacks.registerCallback("position", [this]() {
      return position();
    });

  callbacks.registerCallback("setInteractive", [this](bool interactive) {
      m_interactive.set(interactive);
    });

  callbacks.registerCallbackWithSignature<Maybe<String>>("uniqueId", bind(&Object::uniqueId, this));
  callbacks.registerCallbackWithSignature<void, Maybe<String>>("setUniqueId", bind(&Object::setUniqueId, this, _1));

  callbacks.registerCallback("boundBox", [this]() {
      return metaBoundBox().translated(position());
    });

  callbacks.registerCallback("spaces", [this]() {
      return spaces();
    });

  callbacks.registerCallback("setProcessingDirectives", [this](String const& directives) {
      m_networkedAnimator->setProcessingDirectives(directives);
    });

  callbacks.registerCallback("setSoundEffectEnabled", [this](bool soundEffectEnabled) {
      m_soundEffectEnabled.set(soundEffectEnabled);
    });

  callbacks.registerCallback("smash", [this](Maybe<bool> smash) {
      breakObject(smash.value(false));
    });

  callbacks.registerCallback("level", [this]() {
      return configValue("level", this->world()->threatLevel());
    });

  callbacks.registerCallback("toAbsolutePosition", [this](Vec2F const& p) {
      return p + position();
    });

  callbacks.registerCallback("say", [this](String line, Maybe<StringMap<String>> const& tags, Json const& config) {
      if (tags)
        line = line.replaceTags(*tags, false);

      if (!line.empty()) {
        addChatMessage(line, config);
        return true;
      }

      return false;
    });

  callbacks.registerCallback("sayPortrait", [this](String line, String portrait, Maybe<StringMap<String>> const& tags, Json const& config) {
      if (tags)
        line = line.replaceTags(*tags, false);

      if (!line.empty()) {
        addChatMessage(line, config, portrait);
        return true;
      }

      return false;
    });

  callbacks.registerCallback("isTouching", [this](EntityId entityId) {
      if (auto entity = this->world()->entity(entityId))
        return !entity->collisionArea().overlap(volume().boundBox()).isEmpty();
      return false;
    });

  callbacks.registerCallback("setLightColor", [this](Color const& color) {
      m_lightSourceColor.set(color);
    });

  callbacks.registerCallback("getLightColor", [this]() {
      return m_lightSourceColor.get();
    });

  callbacks.registerCallback("inputNodeCount", [this]() {
      return m_inputNodes.size();
    });

  callbacks.registerCallback("outputNodeCount", [this]() {
      return m_outputNodes.size();
    });

  callbacks.registerCallback("getInputNodePosition", [this](size_t i) {
      return m_inputNodes.at(i).position;
    });

  callbacks.registerCallback("getOutputNodePosition", [this](size_t i) {
      return m_outputNodes.at(i).position;
    });

  callbacks.registerCallback("getInputNodeLevel", [this](size_t i) {
      return m_inputNodes.at(i).state.get();
    });

  callbacks.registerCallback("getOutputNodeLevel", [this](size_t i) {
      return m_outputNodes.at(i).state.get();
    });

  callbacks.registerCallback("isInputNodeConnected", [this](size_t i) {
      return !m_inputNodes.at(i).connections.get().empty();
    });

  callbacks.registerCallback("isOutputNodeConnected", [this](size_t i) {
      return !m_outputNodes.at(i).connections.get().empty();
    });

  callbacks.registerCallback("getInputNodeIds", [this](LuaEngine& engine, size_t i) {
      auto result = engine.createTable();
      for (auto const& conn : m_inputNodes.at(i).connections.get()) {
        for (auto const& entity : worldPtr()->atTile<WireEntity>(conn.entityLocation))
          result.set(entity->entityId(), conn.nodeIndex);
      }
      return result;
    });

  callbacks.registerCallback("getOutputNodeIds", [this](LuaEngine& engine, size_t i) {
      auto result = engine.createTable();
      for (auto const& conn : m_outputNodes.at(i).connections.get()) {
        for (auto const& entity : worldPtr()->atTile<WireEntity>(conn.entityLocation))
          result.set(entity->entityId(), conn.nodeIndex);
      }
      return result;
    });

  callbacks.registerCallback("setOutputNodeLevel", [this](size_t i, bool l) {
      m_outputNodes.at(i).state.set(l);
    });

  callbacks.registerCallback("setAllOutputNodes", [this](bool l) {
      for (auto& out : m_outputNodes)
        out.state.set(l);
    });

  callbacks.registerCallback("setOfferedQuests", [this](Maybe<JsonArray> const& offeredQuests) {
      m_offeredQuests.set(offeredQuests.value().transformed(&QuestArcDescriptor::fromJson));
    });

  callbacks.registerCallback("setTurnInQuests", [this](Maybe<StringList> const& turnInQuests) {
      m_turnInQuests.set(StringSet::from(turnInQuests.value()));
    });

  callbacks.registerCallback("setConfigParameter", [this](String key, Json value) {
      m_parameters.set(std::move(key), std::move(value));
    });

  callbacks.registerCallback("setAnimationParameter", [this](String key, Json value) {
      m_scriptedAnimationParameters.set(std::move(key), std::move(value));
    });

  callbacks.registerCallback("setMaterialSpaces", [this](Maybe<JsonArray> const& newSpaces) {
      List<MaterialSpace> materialSpaces;
      auto materialDatabase = Root::singleton().materialDatabase();
      for (auto space : newSpaces.value())
        materialSpaces.append({jsonToVec2I(space.get(0)), materialDatabase->materialId(space.get(1).toString())});
      m_materialSpaces.set(materialSpaces);
    });

  callbacks.registerCallback("setDamageSources", [this](Maybe<JsonArray> damageSources) {
      m_damageSources.set(damageSources.value().transformed(construct<DamageSource>()));
    });

  callbacks.registerCallback("health", [this]() {
      return m_health.get();
    });

  callbacks.registerCallback("setHealth", [this](float health) {
      m_health.set(health);
    });

  return callbacks;
}

LuaCallbacks Object::makeAnimatorObjectCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("getParameter", [this](String const& name, Json const& def) {
      return configValue(name, def);
    });

  callbacks.registerCallback("direction", [this]() {
      return numericalDirection(direction());
    });

  callbacks.registerCallback("position", [this]() {
      return position();
    });

  return callbacks;
}

List<DamageSource> Object::damageSources() const {
  auto damageSources = m_damageSources.get();

  if (auto orientation = currentOrientation()) {
    Json touchDamageConfig = jsonMerge(m_config->touchDamageConfig, orientation->touchDamageConfig);
    if (!touchDamageConfig.isNull()) {
      DamageSource ds(touchDamageConfig);
      ds.sourceEntityId = entityId();
      ds.team = getTeam();
      damageSources.append(ds);
    }
  }

  return damageSources;
}

List<PersistentStatusEffect> Object::statusEffects() const {
  return m_config->statusEffects;
}

PolyF Object::statusEffectArea() const {
  if (auto orientation = currentOrientation()) {
    if (orientation->statusEffectArea)
      return orientation->statusEffectArea.get();
  }
  return volume();
}

Maybe<HitType> Object::queryHit(DamageSource const& source) const {
  if (!m_config->smashable || !inWorld() || m_health.get() <= 0.0f || m_unbreakable)
    return {};

  if (source.intersectsWithPoly(world()->geometry(), hitPoly().get()))
    return HitType::Hit;

  return {};
}

Maybe<PolyF> Object::hitPoly() const {
  auto poly = volume();
  poly.translate(position());
  return poly;
}

List<DamageNotification> Object::applyDamage(DamageRequest const& damage) {
  if (!m_config->smashable || !inWorld() || m_health.get() <= 0.0f)
    return {};

  if (m_scriptComponent.context()->getPath("applyDamageRequest") != LuaNil) {
    auto notifications = m_scriptComponent.invoke<List<DamageNotification>>("applyDamageRequest", damage);
    float totalDamage = 0.0f;
    for (auto const& notification : *notifications)
      totalDamage += notification.healthLost;

    float dmg = std::min(m_health.get(), totalDamage);
    m_health.set(m_health.get() - dmg);
    return *notifications;
  }

  float dmg = std::min(m_health.get(), damage.damage);
  m_health.set(m_health.get() - dmg);

  return {{
    damage.sourceEntityId,
    entityId(),
    position(),
    damage.damage,
    dmg,
    m_health.get() <= 0 ? HitType::Kill : HitType::Hit,
    damage.damageSourceKind,
    m_config->damageMaterialKind
  }};
}

RectF Object::interactiveBoundBox() const {
  if (auto orientation = currentOrientation()) {
    auto rect = RectF(orientation->boundBox);
    rect.setMax(Vec2F(orientation->boundBox.xMax() + 1, orientation->boundBox.yMax() + 1));
    return rect;
  } else {
    return RectF::null();
  }
}

bool Object::isInteractive() const {
  return m_interactive.get();
}

InteractAction Object::interact(InteractRequest const& request) {
  Vec2F diff = world()->geometry().diff(request.sourcePosition, position());
  auto result = m_scriptComponent.invoke<Json>(
      "onInteraction", JsonObject{{"source", JsonArray{diff[0], diff[1]}}, {"sourceId", request.sourceId}});

  if (result) {
    if (result->isNull())
      return {};
    else if (result->isType(Json::Type::String))
      return InteractAction(result->toString(), entityId(), Json());
    else
      return InteractAction(result->getString(0), entityId(), result->get(1));
  } else if (!configValue("interactAction", Json()).isNull()) {
    return InteractAction(configValue("interactAction").toString(), entityId(), configValue("interactData", Json()));
  }

  return {};
}

List<Vec2I> Object::interactiveSpaces() const {
  if (auto orientation = currentOrientation()) {
    if (auto iSpaces = orientation->interactiveSpaces)
      return *iSpaces;
  }
  return spaces();
}

Maybe<LuaValue> Object::callScript(String const& func, LuaVariadic<LuaValue> const& args) {
  return m_scriptComponent.invoke(func, args);
}

Maybe<LuaValue> Object::evalScript(String const& code) {
  return m_scriptComponent.eval(code);
}

Vec2F Object::mouthPosition() const {
  if (auto orientation = currentOrientation()) {
    auto pos = position() + Vec2F(orientation->boundBox.center()[0], orientation->boundBox.max()[1]);
    if (!(orientation->boundBox.size()[0] % 2))
      pos[0] += 0.5;
    if (auto configPosition = configValue("mouthPosition", Json())) {
      auto mouthOffset = jsonToVec2F(configPosition);
      if (m_direction.get() == Direction::Left)
        mouthOffset[0] = -mouthOffset[0];
      pos += mouthOffset;
    }
    return pos;
  } else {
    return position();
  }
}

Vec2F Object::mouthPosition(bool) const {
  return mouthPosition();
}

List<ChatAction> Object::pullPendingChatActions() {
  return std::move(m_pendingChatActions);
}

void Object::addChatMessage(String const& message, Json const& config, String const& portrait) {
  assert(!isSlave());
  m_chatMessage.set(message);
  m_chatPortrait.set(portrait);
  m_chatConfig.set(config);
  m_newChatMessageEvent.trigger();
  if (portrait.empty())
    m_pendingChatActions.append(SayChatAction{entityId(), message, mouthPosition()});
  else
    m_pendingChatActions.append(PortraitChatAction{entityId(), portrait, message, mouthPosition()});
}

List<Drawable> Object::orientationDrawables(size_t orientationIndex) const {
  if (orientationIndex == NPos)
    return {};

  auto& orientation = getOrientations().at(orientationIndex);

  if (!m_orientationDrawablesCache || orientationIndex != m_orientationDrawablesCache->first) {
    m_orientationDrawablesCache = make_pair(orientationIndex, List<Drawable>());
    for (auto const& layer : orientation->imageLayers) {
      Drawable drawable = layer;

      auto& imagePart = drawable.imagePart();
      imagePart.image.directives.clear();
      String imagePath = AssetPath::join(imagePart.image);
      if ((m_colorDirectives || !m_colorSuffix.empty()) && m_imageKeys.contains("color")) { // We had to leave color untouched despite separating its directives for server-side compatibility reasons, temporarily substr it in the image key
        String& color = m_imageKeys.find("color")->second;
        String backup = std::move(color);
        color = backup.substr(0, backup.find('?'));

        // backwards compatibility for this is really annoying, need to append text after the <color> tag to the last directive for a rare use-case
        auto& image = imagePath.utf8();
        size_t suffix = NPos;
        if (!m_colorSuffix.empty() && (suffix = image.rfind("<color>")) != NPos)
          imagePart.image = String(image.substr(0, (suffix += 7))).replaceTags(m_imageKeys, true, "default");
        else
          imagePart.image = imagePath.replaceTags(m_imageKeys, true, "default");

        color = std::move(backup);

        imagePart.image.directives = layer.imagePart().image.directives;
        if (m_colorDirectives)
          imagePart.addDirectives(m_colorDirectives);
        if (suffix != NPos)
          imagePart.addDirectives(m_colorSuffix + String(image.substr(suffix)).replaceTags(m_imageKeys, true, "default"));
      }
      else {
        imagePart.image = imagePath.replaceTags(m_imageKeys, true, "default");
        imagePart.image.directives = layer.imagePart().image.directives;
      }

      imagePart.addDirectives(m_directives);

      if (orientation->flipImages)
        drawable.scale(Vec2F(-1, 1), drawable.boundBox(false).center() - drawable.position);

      m_orientationDrawablesCache->second.append(std::move(drawable));
    }
  }

  auto drawables = m_orientationDrawablesCache->second;
  Drawable::translateAll(drawables, orientation->imagePosition + damageShake());
  return drawables;
}

EntityRenderLayer Object::renderLayer() const {
  if (auto orientation = currentOrientation())
    return orientation->renderLayer;
  else
    return RenderLayerObject;
}

void Object::renderLights(RenderCallback* renderCallback) const {
  renderCallback->addLightSources(lightSources());
}

void Object::renderParticles(RenderCallback* renderCallback) {
  if (!inWorld())
    return;

  if (auto orientation = currentOrientation()) {
    if (m_emissionTimers.size() != orientation->particleEmitters.size())
      resetEmissionTimers();

    for (size_t i = 0; i < orientation->particleEmitters.size(); i++) {
      auto& particleEmitter = orientation->particleEmitters.at(i);

      if (particleEmitter.particleEmissionRate <= 0.0f)
        continue;

      auto& timer = m_emissionTimers.at(i);
      if (timer.ready()) {
        auto particle = particleEmitter.particle;
        particle.applyVariance(particleEmitter.particleVariance);
        if (particleEmitter.placeInSpaces)
          particle.translate(Vec2F(Random::randFrom(orientation->spaces)) + Vec2F(0.5, 0.5));
        particle.translate(position());
        renderCallback->addParticle(std::move(particle));
        timer = GameTimer(1.0f / (particleEmitter.particleEmissionRate + Random::randf(-particleEmitter.particleEmissionRateVariance, particleEmitter.particleEmissionRateVariance)));
      }
    }
  }
}

void Object::renderSounds(RenderCallback* renderCallback) {
  if (m_soundEffectEnabled.get()) {
    if (!m_config->soundEffect.empty() && (!m_soundEffect || m_soundEffect->finished())) {
      auto& root = Root::singleton();

      m_soundEffect = make_shared<AudioInstance>(*root.assets()->audio(m_config->soundEffect));
      m_soundEffect->setLoops(-1);
      // Randomize the start position of the looping persistent audio
      m_soundEffect->seekTime(Random::randf() * m_soundEffect->totalTime());
      m_soundEffect->setRangeMultiplier(m_config->soundEffectRangeMultiplier);
      m_soundEffect->setPosition(metaBoundBox().center() + position());
      // Fade the audio in slowly
      m_soundEffect->setVolume(0);
      m_soundEffect->setVolume(1, 1.0);

      renderCallback->addAudio(m_soundEffect);
    }
  } else {
    if (m_soundEffect)
      m_soundEffect->stop();
  }
}

List<ObjectOrientationPtr> const& Object::getOrientations() const {
  return m_orientations ? *m_orientations : m_config->orientations;
}

Vec2F Object::damageShake() const {
  if (m_tileDamageStatus->damaged() && !m_tileDamageStatus->damageProtected())
    return Vec2F(Random::randf(-1, 1), Random::randf(-1, 1)) * m_tileDamageStatus->damageEffectPercentage() * m_config->damageShakeMagnitude;
  return Vec2F();
}

void Object::checkLiquidBroken() {
  if (m_config->minimumLiquidLevel || m_config->maximumLiquidLevel) {
    float currentLiquidLevel = liquidFillLevel();
    if (m_config->minimumLiquidLevel && currentLiquidLevel < *m_config->minimumLiquidLevel)
      m_broken = true;
    if (m_config->maximumLiquidLevel && currentLiquidLevel > *m_config->maximumLiquidLevel)
      m_broken = true;
  }
}

Object::OutputNode::OutputNode(Json positionConfig, Json config) {
  position = jsonToVec2I(positionConfig);
  color = jsonToColor(config.get("color", jsonFromColor(Color::Red)));
  icon = config.getString("icon", "/interface/wires/outbound.png");
}

Object::InputNode::InputNode(Json positionConfig, Json config) {
  position = jsonToVec2I(positionConfig);
  color = jsonToColor(config.get("color", jsonFromColor(Color::Red)));
  icon = config.getString("icon", "/interface/wires/inbound.png");
}

}// namespace Star
