#include "StarLoungingEntities.hpp"
#include "StarWorld.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"

namespace Star {

EnumMap<LoungeOrientation> const LoungeOrientationNames{{LoungeOrientation::None, "none"},
    {LoungeOrientation::Sit, "sit"},
    {LoungeOrientation::Lay, "lay"},
    {LoungeOrientation::Stand, "stand"}};

EnumMap<LoungeControl> const LoungeControlNames{{LoungeControl::Left, "Left"},
    {LoungeControl::Right, "Right"},
    {LoungeControl::Down, "Down"},
    {LoungeControl::Up, "Up"},
    {LoungeControl::Jump, "Jump"},
    {LoungeControl::PrimaryFire, "PrimaryFire"},
    {LoungeControl::AltFire, "AltFire"},
    {LoungeControl::Special1, "Special1"},
    {LoungeControl::Special2, "Special2"},
    {LoungeControl::Special3, "Special3"},
    {LoungeControl::Walk, "Walk"},
};

size_t LoungeableEntity::anchorCount() const {
  return loungePositions()->size();
}

EntityAnchorConstPtr LoungeableEntity::anchor(size_t anchorPositionIndex) const {
  return loungeAnchor(anchorPositionIndex);
}

LoungeAnchorConstPtr LoungeableEntity::loungeAnchor(size_t positionIndex) const {
  auto const& positionConfig = loungePositions()->valueAt(positionIndex);
  if (!positionConfig.enabled.get())
    return {};

  Mat3F partTransformation = networkedAnimator()->finalPartTransformation(positionConfig.part);
  Vec2F partAnchor = jsonToVec2F(networkedAnimator()->partProperty(positionConfig.part, positionConfig.partAnchor));

  auto loungePosition = make_shared<LoungeAnchor>();
  loungePosition->position = partTransformation.transformVec2(partAnchor) + position();
  if (positionConfig.exitBottomOffset)
    loungePosition->exitBottomPosition = partTransformation.transformVec2(partAnchor + positionConfig.exitBottomOffset.value()) + position();
  loungePosition->direction = partTransformation.determinant() > 0 ? Direction::Right : Direction::Left;
  loungePosition->angle = partTransformation.transformAngle(0.0f);
  if (loungePosition->direction == Direction::Left)
    loungePosition->angle += Constants::pi;
  loungePosition->controllable = true;
  loungePosition->loungeRenderLayer = loungeRenderLayer(positionIndex);
  loungePosition->orientation = positionConfig.orientation.get();
  loungePosition->emote = positionConfig.emote.get();
  loungePosition->dance = positionConfig.dance.get();
  loungePosition->directives = positionConfig.directives.get();
  loungePosition->statusEffects = positionConfig.statusEffects.get();
  loungePosition->armorCosmeticOverrides = positionConfig.armorCosmeticOverrides;
  loungePosition->cursorOverride = positionConfig.cursorOverride;
  loungePosition->cameraFocus = positionConfig.cameraFocus;
  loungePosition->suppressTools = positionConfig.suppressTools;
  loungePosition->usePartZLevel = positionConfig.usePartZLevel;
  loungePosition->hidden = positionConfig.hidden.get();
  loungePosition->dismountable = positionConfig.dismountable.get();
  return loungePosition;
}

void LoungeableEntity::loungeControl(size_t index, LoungeControl loungeControl) {
  auto& loungePosition = loungePositions()->valueAt(index);
  if (isSlave())
    loungePosition.slaveNewControls.add(loungeControl);
  else
    loungePosition.masterControlState[loungeControl].masterHeld = true;
}

void LoungeableEntity::loungeAim(size_t index, Vec2F const& aimPosition) {
  auto& loungePosition = loungePositions()->valueAt(index);
  if (isSlave())
    loungePosition.slaveNewAimPosition = aimPosition;
  else
    loungePosition.masterAimPosition = aimPosition;
}

Set<EntityId> LoungeableEntity::entitiesLoungingIn(size_t positionIndex) const {
  Set<EntityId> loungingInEntities;
  for (auto const& p : entitiesLounging()) {
    if (p.second == positionIndex)
      loungingInEntities.add(p.first);
  }
  return loungingInEntities;
}

Set<pair<EntityId, size_t>> LoungeableEntity::entitiesLounging() const {
  Set<pair<EntityId, size_t>> loungingInEntities;
  world()->forEachEntity(metaBoundBox().translated(position()),
      [&](EntityPtr const& entity) {
        if (auto lounger = as<LoungingEntity>(entity)) {
          if (auto anchorStatus = lounger->loungingIn()) {
            if (anchorStatus->entityId == entityId())
              loungingInEntities.add({entity->entityId(), anchorStatus->positionIndex});
          }
        }
      });
  return loungingInEntities;
}

void LoungeableEntity::setupLoungePositions(float timeout, float heartbeat, JsonObject positions, bool extraControls){
  m_slaveControlTimeout = timeout;
  m_receiveExtraControls = extraControls;
  m_slaveHeartbeatTimer = GameTimer(heartbeat);
  for (auto const& pair : positions) {
    loungePositions()->set(pair.first, LoungePositionConfig(pair.second));
  }
}

void LoungeableEntity::setupLoungeNetStates(NetElementTopGroup * netGroup, uint8_t minimumVersion) {
  loungePositions()->sortByKey();
  for (auto& p : *loungePositions()) {
    p.second.setupNetStates(netGroup, minimumVersion);
  }
}


void LoungeableEntity::loungeInit(){
  if (isMaster()) {

  } else {
    m_slaveHeartbeatTimer.reset();
  }
}

void LoungeableEntity::loungeTickMaster(float dt){
  eraseWhere(m_aliveMasterConnections, [](auto& p) {
    return p.second.tick(GlobalTimestep);
  });

  for (auto& loungePositionPair : *loungePositions()) {
    for (auto& p : loungePositionPair.second.masterControlState) {
      p.second.masterHeld = false;
      filter(p.second.slavesHeld, [this](ConnectionId id) {
          return m_aliveMasterConnections.contains(id);
        });
    }
  }

}

void LoungeableEntity::loungeTickSlave(float dt){
  bool heartbeat = m_slaveHeartbeatTimer.wrapTick();

  for (auto& p : *loungePositions()) {
    if (heartbeat) {
      JsonArray allControlsHeld;
      for (LoungeControl control : p.second.slaveNewControls) {
        if (control > LoungeControl::Special3 && !m_receiveExtraControls)
          continue;
        allControlsHeld.append(LoungeControlNames.getRight(control));
      }
      world()->sendEntityMessage(entityId(), "control_all", {*loungePositions()->indexOf(p.first), std::move(allControlsHeld)});
    } else {
      for (auto control : p.second.slaveNewControls.difference(p.second.slaveOldControls)) {
        if (control > LoungeControl::Special3 && !m_receiveExtraControls)
          continue;
        world()->sendEntityMessage(entityId(), "control_on", {*loungePositions()->indexOf(p.first), LoungeControlNames.getRight(control)});
      }
      for (auto control : p.second.slaveOldControls.difference(p.second.slaveNewControls)) {
        if (control > LoungeControl::Special3 && !m_receiveExtraControls)
          continue;
        world()->sendEntityMessage(entityId(), "control_off", {*loungePositions()->indexOf(p.first), LoungeControlNames.getRight(control)});
      }
    }

    if (p.second.slaveOldAimPosition != p.second.slaveNewAimPosition)
      world()->sendEntityMessage(entityId(), "aim", {*loungePositions()->indexOf(p.first), p.second.slaveNewAimPosition[0], p.second.slaveNewAimPosition[1]});

    p.second.slaveOldControls = take(p.second.slaveNewControls);
    p.second.slaveOldAimPosition = p.second.slaveNewAimPosition;
  }
}

Maybe<Json> LoungeableEntity::receiveLoungeMessage(ConnectionId connectionId, String const& message, JsonArray const& args){
  m_aliveMasterConnections[connectionId] = GameTimer(m_slaveControlTimeout);
  if (message.equalsIgnoreCase("control_on")) {
    auto& loungePosition = loungePositions()->valueAt(args.at(0).toUInt());
    loungePosition.masterControlState[LoungeControlNames.getLeft(args.at(1).toString())].slavesHeld.add(connectionId);
    return Json();
  } else if (message.equalsIgnoreCase("control_off")) {
    auto& loungePosition = loungePositions()->valueAt(args.at(0).toUInt());
    loungePosition.masterControlState[LoungeControlNames.getLeft(args.at(1).toString())].slavesHeld.remove(connectionId);
    return Json();
  } else if (message.equalsIgnoreCase("control_all")) {
    auto& loungePosition = loungePositions()->valueAt(args.at(0).toUInt());
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
    auto& loungePosition = loungePositions()->valueAt(args.at(0).toUInt());
    loungePosition.masterAimPosition = {args.at(1).toFloat(), args.at(2).toFloat()};
    return Json();
  }
  return {};
}

Maybe<size_t> LoungeableEntity::loungeInteract(InteractRequest const& request){
  Maybe<size_t> index;
  for (size_t i = 0; i < loungePositions()->size(); ++i) {
    auto const& thisLounge = loungePositions()->valueAt(i);
    if (!thisLounge.enabled.get())
      continue;
    if (!index) {
      index = i;
    } else {
      Vec2F thisLoungePosition = *networkedAnimator()->partPoint(thisLounge.part, thisLounge.partAnchor) + position();

      auto const& selectedLounge = loungePositions()->valueAt(*index);
      Vec2F selectedLoungePosition = *networkedAnimator()->partPoint(selectedLounge.part, selectedLounge.partAnchor) + position();

      if (vmagSquared(thisLoungePosition - request.interactPosition) < vmagSquared(selectedLoungePosition - request.interactPosition))
        index = i;
    }
  }
  return index;
}

LuaCallbacks LoungeableEntity::addLoungeableCallbacks(LuaCallbacks callbacks){
  callbacks.registerCallback("controlHeld", [this](String const& loungeName, String const& controlName) {
      auto const& mc = loungePositions()->get(loungeName).masterControlState[LoungeControlNames.getLeft(controlName)];
      return mc.masterHeld || !mc.slavesHeld.empty();
    });

  callbacks.registerCallback("shiftingHeld", [this](String const& loungeName) {
      auto const& mc = loungePositions()->get(loungeName).masterControlState[LoungeControl::Walk];
      if (mc.masterHeld || !mc.slavesHeld.empty())
        return true;
      else {
        for (EntityId entity : entitiesLoungingIn(*loungePositions()->indexOf(loungeName))) {
          if (auto player = world()->get<Player>(entity))
            return player->shifting();
        }
      }
      return false;
    });

  callbacks.registerCallback( "aimPosition", [this](String const& loungeName) {
      return loungePositions()->get(loungeName).masterAimPosition;
    });

  callbacks.registerCallback("entityLoungingIn", [this](String const& name) -> LuaValue {
      auto entitiesIn = entitiesLoungingIn(*loungePositions()->indexOf(name));
      if (entitiesIn.empty())
        return LuaNil;
      return LuaInt(entitiesIn.first());
    });

  callbacks.registerCallback("setLoungeEnabled", [this](String const& name, bool enabled) {
      loungePositions()->get(name).enabled.set(enabled);
    });

  callbacks.registerCallback("setLoungeOrientation", [this](String const& name, String const& orientation) {
      loungePositions()->get(name).orientation.set(LoungeOrientationNames.getLeft(orientation));
    });

  callbacks.registerCallback("setLoungeEmote", [this](String const& name, Maybe<String> emote) {
      loungePositions()->get(name).emote.set(std::move(emote));
    });

  callbacks.registerCallback("setLoungeDance", [this](String const& name, Maybe<String> dance) {
      loungePositions()->get(name).dance.set(std::move(dance));
    });

  callbacks.registerCallback("setLoungeDirectives", [this](String const& name, Maybe<String> directives) {
      loungePositions()->get(name).directives.set(std::move(directives));
    });

  callbacks.registerCallback("setLoungeStatusEffects", [this](String const& name, JsonArray const& statusEffects) {
      loungePositions()->get(name).statusEffects.set(statusEffects.transformed(jsonToPersistentStatusEffect));
    });
  callbacks.registerCallback("setLoungeHidden", [this](String const& name, bool hidden) {
      loungePositions()->get(name).hidden.set(hidden);
    });
  callbacks.registerCallback("setLoungeDismountable", [this](String const& name, bool dismountable) {
      loungePositions()->get(name).dismountable.set(dismountable);
    });
  callbacks.registerCallback("getLoungeIndex", [this](String const& name) -> Maybe<int> {
      return loungePositions()->indexOf(name);
    });
  callbacks.registerCallback("getLoungeName", [this](int const& index) -> Maybe<String> {
      return loungePositions()->keyAt(index);
    });

  return callbacks;
}

void LoungeableEntity::clearLoungingDrawables() {
  for (size_t i = 0; i < loungePositions()->size(); ++i) {
    auto const& thisLounge = loungePositions()->valueAt(i);
    if (thisLounge.usePartZLevel && !thisLounge.hidden.get()) {
      networkedAnimator()->setPartDrawables(thisLounge.part, {});
    }
  }
}

void LoungeableEntity::setupLoungingDrawables(Vec2F scale) {
  for (size_t i = 0; i < loungePositions()->size(); ++i) {
    auto const& thisLounge = loungePositions()->valueAt(i);
    if (thisLounge.usePartZLevel && !thisLounge.hidden.get()) {
      for (auto id : entitiesLoungingIn(i)) {
        if (auto entity = world()->get<LoungingEntity>(id)) {
          Mat3F partTransformation = networkedAnimator()->finalPartTransformation(thisLounge.part);
          auto drawables = entity->drawables();
          Drawable::scaleAll(drawables, Vec2F(scale[0] * (partTransformation.determinant() > 0 ? 1 : -1), scale[1]));
          Drawable::translateAll(drawables, jsonToVec2F(networkedAnimator()->partProperty(thisLounge.part, thisLounge.partAnchor)));
          networkedAnimator()->setPartDrawables(thisLounge.part, drawables);
        }
      }
    }
  }
}

bool LoungingEntity::inConflictingLoungeAnchor() const {
  if (auto loungeAnchorState = loungingIn()) {
    if (auto loungeableEntity = world()->get<LoungeableEntity>(loungeAnchorState->entityId)) {
      auto entitiesLoungingIn = loungeableEntity->entitiesLoungingIn(loungeAnchorState->positionIndex);
      return entitiesLoungingIn.size() > 1 || !entitiesLoungingIn.contains(entityId());
    }
  }
  return false;
}

LoungeableEntity::LoungePositionConfig::LoungePositionConfig(Json config){
  part = config.getString("part");
  partAnchor = config.getString("partAnchor");
  exitBottomOffset = config.opt("exitBottomOffset").apply(jsonToVec2F);
  armorCosmeticOverrides = config.getObject("armorCosmeticOverrides", JsonObject());
  cursorOverride = config.optString("cursorOverride");
  cameraFocus = config.getBool("cameraFocus", false);
  enabled.set(config.getBool("enabled", true));
  if (auto loungeOrientation = config.optString("orientation"))
    orientation.set(LoungeOrientationNames.getLeft(*loungeOrientation));
  emote.set(config.optString("emote"));
  dance.set(config.optString("dance"));
  directives.set(config.optString("directives"));
  statusEffects.set(config.getArray("statusEffects", {}).transformed(jsonToPersistentStatusEffect));
  suppressTools = config.optBool("suppressTools");
  usePartZLevel = config.getBool("usePartZLevel", false);
  hidden.set(config.getBool("hidden", false));
  dismountable.set(config.getBool("dismountable", true));
}

void LoungeableEntity::LoungePositionConfig::setupNetStates(NetElementTopGroup * netGroup, uint8_t minimumVersion){
  enabled.setCompatibilityVersion(minimumVersion);
  netGroup->addNetElement(&enabled);
  orientation.setCompatibilityVersion(minimumVersion);
  netGroup->addNetElement(&orientation);
  emote.setCompatibilityVersion(minimumVersion);
  netGroup->addNetElement(&emote);
  dance.setCompatibilityVersion(minimumVersion);
  netGroup->addNetElement(&dance);
  directives.setCompatibilityVersion(minimumVersion);
  netGroup->addNetElement(&directives);
  statusEffects.setCompatibilityVersion(minimumVersion);
  netGroup->addNetElement(&statusEffects);
  hidden.setCompatibilityVersion(max<uint8_t>(minimumVersion, 10));
  netGroup->addNetElement(&hidden);
  dismountable.setCompatibilityVersion(max<uint8_t>(minimumVersion, 10));
  netGroup->addNetElement(&dismountable);
}
JsonObject LoungeAnchor::toJson() const {
  return {
    {"orientation", LoungeOrientationNames.getRight(orientation)},
    {"loungeRenderLayer", loungeRenderLayer},
    {"controllable", controllable},
    {"statusEffects", statusEffects.transformed(jsonFromPersistentStatusEffect)},
    {"effectEmitters", jsonFromStringSet(effectEmitters)},
    {"emote", emote.isValid() ? emote.value(): Json()},
    {"dance", dance.isValid() ? dance.value(): Json()},
    {"directives", directives.isValid() ? directives.value().string() : Json()},
    {"cursorOverride", cursorOverride.isValid() ? cursorOverride.value(): Json()},
    {"suppressTools", suppressTools.isValid() ? suppressTools.value(): Json()},
    {"armorCosmeticOverrides", armorCosmeticOverrides},
    {"cameraFocus", cameraFocus},
    {"usePartZLevel", usePartZLevel},
    {"hidden", hidden},
    {"dismountable", dismountable}
  };
}
}// namespace Star
