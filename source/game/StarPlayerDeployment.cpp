#include "StarPlayerDeployment.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarEntityRendering.hpp"

namespace Star {

PlayerDeployment::PlayerDeployment(Json const& config) : m_config(config) {
  m_deploying = false;
  m_deployed = false;
}

void PlayerDeployment::diskLoad(Json const& diskStore) {
  m_scriptComponent.setScriptStorage(diskStore.getObject("scriptStorage", JsonObject{}));
}

Json PlayerDeployment::diskStore() const {
  JsonObject result;
  result["scriptStorage"] = m_scriptComponent.getScriptStorage();
  return result;
}

void PlayerDeployment::init(Entity* player, World* world) {
  m_world = world;

  if (m_deploying) {
    m_deployed = true;
    m_deploying = false;
  } else {
    m_deployed = false;
  }

  m_scriptComponent.setScripts(jsonToStringList(m_config.getArray("scripts", JsonArray())));
  m_scriptComponent.setUpdateDelta(m_config.getInt("scriptDelta", 10));

  m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(player));
  m_scriptComponent.addCallbacks("player", LuaBindings::makePlayerCallbacks(as<Player>(player)));
  m_scriptComponent.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(as<Player>(player)->statusController()));
  m_scriptComponent.addCallbacks("config",
      LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def) { return m_config.query(name, def); }));

  m_scriptComponent.init(world);
}

bool PlayerDeployment::canDeploy() {
  Maybe<bool> res = m_scriptComponent.invoke<bool>("canDeploy");
  return res && *res;
}

void PlayerDeployment::setDeploying(bool deploying) {
  m_deploying = deploying;
}

bool PlayerDeployment::isDeploying() const {
  return m_deploying;
}

bool PlayerDeployment::isDeployed() const {
  return m_deployed;
}

void PlayerDeployment::uninit() {
  m_scriptComponent.uninit();
  m_scriptComponent.removeCallbacks("entity");
  m_scriptComponent.removeCallbacks("player");
  m_scriptComponent.removeCallbacks("status");
  m_scriptComponent.removeCallbacks("config");
  m_world = nullptr;
}

void PlayerDeployment::teleportOut() {
  m_scriptComponent.invoke("teleportOut");
}

Maybe<Json> PlayerDeployment::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  return m_scriptComponent.handleMessage(message, localMessage, args);
}

void PlayerDeployment::update() {
  m_scriptComponent.update(m_scriptComponent.updateDt());
}

void PlayerDeployment::render(RenderCallback* renderCallback, Vec2F const& position) {
  for (auto drawablePair : m_scriptComponent.drawables()) {
    drawablePair.first.translate(position);
    renderCallback->addDrawable(drawablePair.first, drawablePair.second.value(RenderLayerPlayer));
  }
  renderCallback->addParticles(m_scriptComponent.pullNewParticles());
  for (auto audio : m_scriptComponent.pullNewAudios()) {
    audio->setPosition(position);
    renderCallback->addAudio(audio);
  }
}

void PlayerDeployment::renderLightSources(RenderCallback* renderCallback) {
  renderCallback->addLightSources(m_scriptComponent.lightSources());
}

}
