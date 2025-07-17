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
  return m_loungePositions.size();
}

EntityAnchorConstPtr LoungeableEntity::anchor(size_t anchorPositionIndex) const {
  return loungeAnchor(anchorPositionIndex);
}

LoungeAnchorConstPtr LoungeableEntity::loungeAnchor(size_t positionIndex) const {
  auto const& positionConfig = m_loungePositions.valueAt(positionIndex);
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
  return loungePosition;
}

void LoungeableEntity::loungeControl(size_t index, LoungeControl loungeControl) {
  auto& loungePosition = m_loungePositions.valueAt(index);
  if (isSlave())
    loungePosition.slaveNewControls.add(loungeControl);
  else
    loungePosition.masterControlState[loungeControl].masterHeld = true;
}

void LoungeableEntity::loungeAim(size_t index, Vec2F const& aimPosition) {
  auto& loungePosition = m_loungePositions.valueAt(index);
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

void LoungeableEntity::setupLoungePositions(function<Json(String const&,Json const&)> configValue){
  m_slaveControlTimeout = configValue("slaveControlTimeout", Json()).toFloat();
  m_receiveExtraControls = configValue("receiveExtraControls", false).toBool();
  m_slaveHeartbeatTimer = GameTimer(configValue("slaveControlHeartbeat", Json()).toFloat());

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
    loungePosition.suppressTools = pair.second.optBool("suppressTools");
  }
}

void LoungeableEntity::setupLoungeNetStates(NetElementTopGroup * netGroup, u_int8_t minimumVersion) {
  m_loungePositions.sortByKey();
  for (auto& p : m_loungePositions) {
    p.second.enabled.setCompatibilityVersion(minimumVersion);
    netGroup->addNetElement(&p.second.enabled);
    p.second.orientation.setCompatibilityVersion(minimumVersion);
    netGroup->addNetElement(&p.second.orientation);
    p.second.emote.setCompatibilityVersion(minimumVersion);
    netGroup->addNetElement(&p.second.emote);
    p.second.dance.setCompatibilityVersion(minimumVersion);
    netGroup->addNetElement(&p.second.dance);
    p.second.directives.setCompatibilityVersion(minimumVersion);
    netGroup->addNetElement(&p.second.directives);
    p.second.statusEffects.setCompatibilityVersion(minimumVersion);
    netGroup->addNetElement(&p.second.statusEffects);
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

  for (auto& loungePositionPair : m_loungePositions) {
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

  for (auto& p : m_loungePositions) {
    if (heartbeat) {
      JsonArray allControlsHeld;
      for (LoungeControl control : p.second.slaveNewControls) {
        if (control > LoungeControl::Special3 && !m_receiveExtraControls)
          continue;
        allControlsHeld.append(LoungeControlNames.getRight(control));
      }
      world()->sendEntityMessage(entityId(), "control_all", {*m_loungePositions.indexOf(p.first), std::move(allControlsHeld)});
    } else {
      for (auto control : p.second.slaveNewControls.difference(p.second.slaveOldControls)) {
        if (control > LoungeControl::Special3 && !m_receiveExtraControls)
          continue;
        world()->sendEntityMessage(entityId(), "control_on", {*m_loungePositions.indexOf(p.first), LoungeControlNames.getRight(control)});
      }
      for (auto control : p.second.slaveOldControls.difference(p.second.slaveNewControls)) {
        if (control > LoungeControl::Special3 && !m_receiveExtraControls)
          continue;
        world()->sendEntityMessage(entityId(), "control_off", {*m_loungePositions.indexOf(p.first), LoungeControlNames.getRight(control)});
      }
    }

    if (p.second.slaveOldAimPosition != p.second.slaveNewAimPosition)
      world()->sendEntityMessage(entityId(), "aim", {*m_loungePositions.indexOf(p.first), p.second.slaveNewAimPosition[0], p.second.slaveNewAimPosition[1]});

    p.second.slaveOldControls = take(p.second.slaveNewControls);
    p.second.slaveOldAimPosition = p.second.slaveNewAimPosition;
  }
}

Maybe<Json> LoungeableEntity::receiveLoungeMessage(ConnectionId connectionId, String const& message, JsonArray const& args){
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
  }
  return {};
}

Maybe<size_t> LoungeableEntity::loungeInteract(InteractRequest const& request){
  Maybe<size_t> index;
  for (size_t i = 0; i < m_loungePositions.size(); ++i) {
    if (!index) {
      index = i;
    } else {
      auto const& thisLounge = m_loungePositions.valueAt(i);
      if (!thisLounge.enabled.get())
        continue;

      Vec2F thisLoungePosition = *networkedAnimator()->partPoint(thisLounge.part, thisLounge.partAnchor) + position();

      auto const& selectedLounge = m_loungePositions.valueAt(*index);
      Vec2F selectedLoungePosition = *networkedAnimator()->partPoint(selectedLounge.part, selectedLounge.partAnchor) + position();

      if (vmagSquared(thisLoungePosition - request.interactPosition) < vmagSquared(selectedLoungePosition - request.interactPosition))
        index = i;
    }
  }
  return index;
}

LuaCallbacks LoungeableEntity::makeLoungeableCallbacks()
{
  LuaCallbacks callbacks;
  callbacks.registerCallback("controlHeld", [this](String const& loungeName, String const& controlName) {
      auto const& mc = m_loungePositions.get(loungeName).masterControlState[LoungeControlNames.getLeft(controlName)];
      return mc.masterHeld || !mc.slavesHeld.empty();
    });

  callbacks.registerCallback("shiftingHeld", [this](String const& loungeName) {
      auto const& mc = m_loungePositions.get(loungeName).masterControlState[LoungeControl::Walk];
      if (mc.masterHeld || !mc.slavesHeld.empty())
        return true;
      else {
        for (EntityId entity : entitiesLoungingIn(*m_loungePositions.indexOf(loungeName))) {
          if (auto player = world()->get<Player>(entity))
            return player->shifting();
        }
      }
      return false;
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
      m_loungePositions.get(name).emote.set(std::move(emote));
    });

  callbacks.registerCallback("setLoungeDance", [this](String const& name, Maybe<String> dance) {
      m_loungePositions.get(name).dance.set(std::move(dance));
    });

  callbacks.registerCallback("setLoungeDirectives", [this](String const& name, Maybe<String> directives) {
      m_loungePositions.get(name).directives.set(std::move(directives));
    });

  callbacks.registerCallback("setLoungeStatusEffects", [this](String const& name, JsonArray const& statusEffects) {
      m_loungePositions.get(name).statusEffects.set(statusEffects.transformed(jsonToPersistentStatusEffect));
    });

  return callbacks;
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

}
