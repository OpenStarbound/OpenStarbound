#include "StarTechController.hpp"
#include "StarStatusController.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarPlayer.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarRoot.hpp"
#include "StarScriptedEntity.hpp"
#include "StarLoungingEntities.hpp"
#include "StarWorld.hpp"
#include "StarLogging.hpp"

namespace Star {

EnumMap<TechController::ParentState> const TechController::ParentStateNames{
  {TechController::ParentState::Stand, "Stand"},
  {TechController::ParentState::Fly, "Fly"},
  {TechController::ParentState::Fall, "Fall"},
  {TechController::ParentState::Sit, "Sit"},
  {TechController::ParentState::Lay, "Lay"},
  {TechController::ParentState::Duck, "Duck"},
  {TechController::ParentState::Walk, "Walk"},
  {TechController::ParentState::Run, "Run"},
  {TechController::ParentState::Swim, "Swim"},
  {TechController::ParentState::SwimIdle, "SwimIdle"}
};

TechController::TechController() {
  m_parentEntity = nullptr;
  m_movementController = nullptr;
  m_statusController = nullptr;

  m_moveRun = false;
  m_movePrimaryFire = false;
  m_moveAltFire = false;
  m_moveUp = false;
  m_moveDown = false;
  m_moveLeft = false;
  m_moveRight = false;
  m_moveJump = false;
  m_moveSpecial1 = false;
  m_moveSpecial2 = false;
  m_moveSpecial3 = false;

  addNetElement(&m_techAnimators);
  addNetElement(&m_parentState);
  addNetElement(&m_parentDirectives);
  addNetElement(&m_xParentOffset);
  addNetElement(&m_yParentOffset);
  addNetElement(&m_parentHidden);
  addNetElement(&m_toolUsageSuppressed);

  m_xParentOffset.setFixedPointBase(0.003125);
  m_yParentOffset.setFixedPointBase(0.003125);
  m_xParentOffset.setInterpolator(lerp<float, float>);
  m_yParentOffset.setInterpolator(lerp<float, float>);
}

Json TechController::diskStore() {
  if (m_overriddenTech) {
    auto modules = JsonArray();
    for (String moduleName : *m_overriddenTech)
      modules.append(JsonObject{{"module", moduleName}, {"scriptData", JsonObject()}});
    return JsonObject{{"techModules", modules}};
  } else {
    return JsonObject{{"techModules", transform<JsonArray>(m_techModules, [](TechModule const& tm) {
        return JsonObject{
            {"module", tm.config.name},
            {"scriptData", tm.scriptComponent.getScriptStorage()}
          };
      })
    }};
  }
}

void TechController::diskLoad(Json const& store) {
  setupTechModules( store.getArray("techModules", {}).transformed([](Json const& v) {
      return make_tuple(v.getString("module"), v.getObject("scriptData", {}));
    }));
}

void TechController::init(Entity* parentEntity, ActorMovementController* movementController, StatusController* statusController) {
  m_parentEntity = parentEntity;
  m_movementController = movementController;
  m_statusController = statusController;

  m_moveRun = false;
  m_movePrimaryFire = false;
  m_moveAltFire = false;
  resetMoves();

  if (m_parentEntity->isMaster())
    initializeModules();
}

void TechController::uninit() {
  m_parentEntity = nullptr;
  m_movementController = nullptr;
  m_statusController = nullptr;

  for (auto& module : m_techModules)
    unloadModule(module);
}

void TechController::setLoadedTech(StringList const& techModules, bool forceLoad) {
  if (forceLoad || loadedTech() != techModules) {
    setupTechModules(techModules.transformed([](String const& module) { return make_tuple(module, JsonObject()); }));
    if (m_parentEntity)
      initializeModules();
  }
}

StringList TechController::loadedTech() const {
  return transform<StringList>(m_techModules, [](TechModule const& tm) { return tm.config.name; });
}

void TechController::reloadTech() {
  setLoadedTech(loadedTech(), true);
}

bool TechController::techOverridden() const {
  return (bool)m_overriddenTech;
}

void TechController::setOverrideTech(StringList const& techModules) {
  if (!m_overriddenTech)
    m_overriddenTech = loadedTech();

  setLoadedTech(techModules, true);
}

void TechController::clearOverrideTech() {
  if (m_overriddenTech) {
    setLoadedTech(*m_overriddenTech, true);
    m_overriddenTech = {};
  }
}

void TechController::setShouldRun(bool shouldRun) {
  m_moveRun = shouldRun;
}

void TechController::beginPrimaryFire() {
  m_movePrimaryFire = true;
}

void TechController::beginAltFire() {
  m_moveAltFire = true;
}

void TechController::endPrimaryFire() {
  m_movePrimaryFire = false;
}

void TechController::endAltFire() {
  m_moveAltFire = false;
}

void TechController::moveUp() {
  m_moveUp = true;
}

void TechController::moveDown() {
  m_moveDown = true;
}

void TechController::moveLeft() {
  m_moveLeft = true;
}

void TechController::moveRight() {
  m_moveRight = true;
}

void TechController::jump() {
  m_moveJump = true;
}

void TechController::special(int specialKey) {
  if (specialKey == 1)
    m_moveSpecial1 = true;
  if (specialKey == 2)
    m_moveSpecial2 = true;
  if (specialKey == 3)
    m_moveSpecial3 = true;
}

void TechController::setAimPosition(Vec2F const& aimPosition) {
  m_aimPosition = aimPosition;
}

void TechController::tickMaster(float dt) {
  // if there's no gravity, fly instead of walking
  if (m_movementController->zeroG()) {
    if (m_moveRight || m_moveLeft || m_moveUp || m_moveDown) {
      Vec2F flightControl = {0, 0};

      if (m_moveRight)
        flightControl[0]++;
      if (m_moveLeft)
        flightControl[0]--;
      if (m_moveUp)
        flightControl[1]++;
      if (m_moveDown)
        flightControl[1]--;

      m_movementController->controlFly(flightControl);
    }
  } else {
    if (m_moveLeft != m_moveRight)
      m_movementController->controlMove(m_moveLeft ? Direction::Left : Direction::Right, m_moveRun);

    if (m_moveJump && !m_moveDown)
      m_movementController->controlJump();

    if (m_movementController->onGround() && m_moveDown && !m_moveJump)
      m_movementController->controlCrouch();
    else if (m_moveDown)
      m_movementController->controlDown();
  }

  for (auto& module : m_techModules) {
    JsonObject moves = {
      {"run", m_moveRun},
      {"up", m_moveUp},
      {"down", m_moveDown},
      {"left", m_moveLeft},
      {"right", m_moveRight},
      {"jump", m_moveJump},
      {"primaryFire", m_movePrimaryFire},
      {"altFire", m_moveAltFire},
      {"special1", m_moveSpecial1},
      {"special2", m_moveSpecial2},
      {"special3", m_moveSpecial3}
    };

    module.scriptComponent.update(JsonObject{{"moves", moves}, {"dt", module.scriptComponent.updateDt(dt)}});
  }

  resetMoves();
  updateAnimators(dt);
}

void TechController::tickSlave(float dt) {
  resetMoves();
  updateAnimators(dt);
}

Maybe<TechController::ParentState> TechController::parentState() const {
  return m_parentState.get();
}

DirectivesGroup const& TechController::parentDirectives() const {
  return m_parentDirectives.get();
}

Vec2F TechController::parentOffset() const {
  return {m_xParentOffset.get(), m_yParentOffset.get()};
}

bool TechController::toolUsageSuppressed() const {
  return m_toolUsageSuppressed.get();
}

bool TechController::parentHidden() const {
  return m_parentHidden.get();
}

List<Drawable> TechController::backDrawables() {
  List<Drawable> drawables;

  for (auto const& animator : m_techAnimators.netElements()) {
    if (animator->isVisible()) {
      for (auto& piece : animator->animator.drawablesWithZLevel(m_movementController->position())) {
        if (piece.second < 0.0f)
          drawables.append(std::move(piece.first));
      }
    }
  }

  return drawables;
}

List<Drawable> TechController::frontDrawables() {
  List<Drawable> drawables;

  for (auto const& animator : m_techAnimators.netElements()) {
    if (animator->isVisible()) {
      for (auto& piece : animator->animator.drawablesWithZLevel(m_movementController->position())) {
        if (piece.second >= 0.0f)
          drawables.append(std::move(piece.first));
      }
    }
  }

  return drawables;
}

List<LightSource> TechController::lightSources() const {
  List<LightSource> lightSources;

  for (auto const& animator : m_techAnimators.netElements()) {
    if (animator->isVisible())
      lightSources.appendAll(animator->animator.lightSources(m_movementController->position()));
  }

  return lightSources;
}

List<AudioInstancePtr> TechController::pullNewAudios() {
  List<AudioInstancePtr> newAudios;

  for (auto const& animator : m_techAnimators.netElements()) {
    if (animator->isVisible())
      newAudios.appendAll(animator->dynamicTarget.pullNewAudios());
    else
      animator->dynamicTarget.pullNewAudios();
  }

  return newAudios;
}

List<Particle> TechController::pullNewParticles() {
  List<Particle> newParticles;

  for (auto const& animator : m_techAnimators.netElements()) {
    if (animator->isVisible())
      newParticles.appendAll(animator->dynamicTarget.pullNewParticles());
    else
      animator->dynamicTarget.pullNewParticles();
  }

  return newParticles;
}

Maybe<Json> TechController::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  for (auto& module : m_techModules) {
    if (auto res = module.scriptComponent.handleMessage(message, localMessage, args))
      return res;
  }
  return {};
}

TechController::TechAnimator::TechAnimator(Maybe<String> ac) {
  animationConfig = std::move(ac);
  animator = animationConfig ? NetworkedAnimator(*animationConfig) : NetworkedAnimator();
  netGroup.addNetElement(&animator);
  netGroup.addNetElement(&visible);
}

void TechController::TechAnimator::initNetVersion(NetElementVersion const* version) {
  netGroup.initNetVersion(version);
}

void TechController::TechAnimator::netStore(DataStream& ds, NetCompatibilityRules rules) const {
  if (!checkWithRules(rules)) return;
  ds << animationConfig;
  netGroup.netStore(ds, rules);
}

void TechController::TechAnimator::netLoad(DataStream& ds, NetCompatibilityRules rules) {
  if (!checkWithRules(rules)) return;
  ds >> animationConfig;
  animator = animationConfig ? NetworkedAnimator(*animationConfig) : NetworkedAnimator();
  netGroup.netLoad(ds, rules);
}

void TechController::TechAnimator::enableNetInterpolation(float extrapolationHint) {
  netGroup.enableNetInterpolation(extrapolationHint);
}

void TechController::TechAnimator::disableNetInterpolation() {
  netGroup.disableNetInterpolation();
}

void TechController::TechAnimator::tickNetInterpolation(float dt) {
  netGroup.tickNetInterpolation(dt);
}

bool TechController::TechAnimator::writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules) const {
  return netGroup.writeNetDelta(ds, fromVersion, rules);
}

void TechController::TechAnimator::readNetDelta(DataStream& ds, float interpolationTime, NetCompatibilityRules rules) {
  netGroup.readNetDelta(ds, interpolationTime, rules);
}

void TechController::TechAnimator::blankNetDelta(float interpolationTime) {
  netGroup.blankNetDelta(interpolationTime);
}

void TechController::TechAnimator::setVisible(bool visible) {
  this->visible.set(visible);
  if (!visible)
    dynamicTarget.stopAudio();
}

bool TechController::TechAnimator::isVisible() const {
  return visible.get();
}

void TechController::setupTechModules(List<tuple<String, JsonObject>> const& moduleInits) {
  for (auto& techModule : m_techModules)
    unloadModule(techModule);

  m_techModules.clear();
  m_techAnimators.clearNetElements();

  auto techDatabase = Root::singleton().techDatabase();

  for (auto const& moduleInit : moduleInits) {
    if (techDatabase->contains(get<0>(moduleInit))) {
      auto& module = m_techModules.emplaceAppend();

      module.config = techDatabase->tech(get<0>(moduleInit));

      module.scriptComponent.setScripts(module.config.scripts);
      module.scriptComponent.setScriptStorage(get<1>(moduleInit));

      module.visible = module.config.parameters.getBool("visible", true);

      module.toolUsageSuppressed = false;

      auto moduleAnimator = make_shared<TechAnimator>(module.config.animationConfig);
      for (auto const& pair : module.config.parameters.get("animationParts", JsonObject()).iterateObject())
        moduleAnimator->animator.setPartTag(pair.first, "partImage", pair.second.toString());
      module.animatorId = m_techAnimators.addNetElement(moduleAnimator);
    } else {
      Logger::warn("Tech module '{}' not found in tech database", get<0>(moduleInit));
    }
  }
}

void TechController::unloadModule(TechModule& techModule) {
  techModule.scriptComponent.uninit();
  techModule.scriptComponent.removeCallbacks("tech");
  techModule.scriptComponent.removeCallbacks("config");
  techModule.scriptComponent.removeCallbacks("entity");
  techModule.scriptComponent.removeCallbacks("animator");
  techModule.scriptComponent.removeCallbacks("status");
  techModule.scriptComponent.removeCallbacks("player");
  techModule.scriptComponent.removeActorMovementCallbacks();
}

void TechController::initializeModules() {
  for (auto& module : m_techModules) {
    module.scriptComponent.addCallbacks("tech", makeTechCallbacks(module));
    module.scriptComponent.addCallbacks("config", LuaBindings::makeConfigCallbacks([&module](String const& name, Json const& def) {
        return module.config.parameters.query(name, def);
      }));
    module.scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(m_parentEntity));
    module.scriptComponent.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(&m_techAnimators.getNetElement(module.animatorId)->animator));
    module.scriptComponent.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_statusController));
    if (auto player = as<Player>(m_parentEntity))
      module.scriptComponent.addCallbacks("player", LuaBindings::makePlayerCallbacks(player));
    module.scriptComponent.addActorMovementCallbacks(m_movementController);

    module.scriptComponent.init(m_parentEntity->world());
  }
}

void TechController::resetMoves() {
  m_moveRun = false;
  m_moveUp = false;
  m_moveDown = false;
  m_moveLeft = false;
  m_moveRight = false;
  m_moveJump = false;
  m_moveSpecial1 = false;
  m_moveSpecial2 = false;
  m_moveSpecial3 = false;
}

void TechController::updateAnimators(float dt) {
  for (auto const& module : m_techModules)
    m_techAnimators.getNetElement(module.animatorId)->setVisible(module.visible);

  for (auto const& animator : m_techAnimators.netElements()) {
    if (m_parentEntity->world()->isServer() || !animator->isVisible()) {
      animator->animator.update(dt, nullptr);
    } else {
      animator->animator.update(dt, &animator->dynamicTarget);
      animator->dynamicTarget.updatePosition(m_movementController->position());
    }
  }
}

LuaCallbacks TechController::makeTechCallbacks(TechModule& techModule) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("aimPosition", [this]() {
      return m_aimPosition;
    });

  callbacks.registerCallback("setVisible", [&techModule](bool visible) {
      techModule.visible = visible;
    });

  callbacks.registerCallback("setParentState", [this](Maybe<String> const& state) {
      if (state)
        m_parentState.set(ParentStateNames.getLeft(*state));
      else
        m_parentState.set({});
    });

  callbacks.registerCallback("setParentDirectives", [this, &techModule](Maybe<String> const& directives) {
      techModule.parentDirectives = directives.value();

      DirectivesGroup newParentDirectives;
      for (auto& module : m_techModules)
        newParentDirectives.append(module.parentDirectives);
      m_parentDirectives.set(std::move(newParentDirectives));
    });

  callbacks.registerCallback("setParentHidden", [this](bool hidden) {
      m_parentHidden.set(hidden);
    });

  callbacks.registerCallback("setParentOffset", [this](Vec2F const& po) {
      m_xParentOffset.set(po[0]);
      m_yParentOffset.set(po[1]);
    });

  callbacks.registerCallback("parentLounging", [this]() {
      if (auto loungingEntity = as<LoungingEntity>(m_parentEntity))
        return loungingEntity->loungingIn().isValid();
      return false;
    });

  callbacks.registerCallback("setToolUsageSuppressed", [this, &techModule](bool suppressed) {
      if (techModule.toolUsageSuppressed == suppressed)
        return;
      techModule.toolUsageSuppressed = suppressed;
      bool anySuppressed = false;
      for (auto& module : m_techModules)
        anySuppressed = anySuppressed || module.toolUsageSuppressed;
      m_toolUsageSuppressed.set(anySuppressed);
    });

  return callbacks;
}

}
