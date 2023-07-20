#include "StarPlayerCompanions.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"

namespace Star {

Companion::Companion(Json const& json) : m_json(json) {
  m_portrait = json.getArray("portrait", JsonArray{}).transformed(construct<Drawable>());
}

Json Companion::toJson() const {
  return m_json;
}

Uuid Companion::podUuid() const {
  return Uuid(m_json.getString("podUuid"));
}

Maybe<String> Companion::name() const {
  return m_json.optString("name");
}

Maybe<String> Companion::description() const {
  return m_json.optString("description");
}

List<Drawable> Companion::portrait() const {
  return m_portrait;
}

Maybe<float> Companion::resource(String const& resourceName) const {
  if (auto status = m_json.opt("status"))
    if (auto resources = status->opt("resources"))
      return resources->optFloat(resourceName);
  return {};
}

Maybe<float> Companion::resourceMax(String const& resourceName) const {
  if (auto status = m_json.opt("status"))
    if (auto resources = status->opt("resourceMax"))
      return resources->optFloat(resourceName);
  return {};
}

Maybe<float> Companion::stat(String const& statName) const {
  if (auto status = m_json.opt("status"))
    if (auto stats = status->opt("stats"))
      return stats->optFloat(statName);
  return {};
}

PlayerCompanions::PlayerCompanions(Json const& config) : m_config(config) {}

void PlayerCompanions::diskLoad(Json const& diskStore) {
  m_scriptComponent.setScriptStorage(diskStore.getObject("scriptStorage", JsonObject{}));
  m_companions = jsonToMapV<StringMap<List<CompanionPtr>>>(diskStore.getObject("companions", JsonObject{}),
      [](Json const& companions) {
        return companions.toArray().transformed([](Json const& json) { return make_shared<Companion>(json); });
      });
}

Json PlayerCompanions::diskStore() const {
  JsonObject result;
  result["scriptStorage"] = m_scriptComponent.getScriptStorage();
  result["companions"] = jsonFromMapV(m_companions,
      [](List<CompanionPtr> const& companions) { return companions.transformed(mem_fn(&Companion::toJson)); });
  return result;
}

List<CompanionPtr> PlayerCompanions::getCompanions(String const& category) const {
  if (m_companions.contains(category))
    return m_companions.get(category);
  return {};
}

void PlayerCompanions::init(Entity* player, World* world) {
  m_world = world;

  m_scriptComponent.setScripts(jsonToStringList(m_config.getArray("scripts", JsonArray())));
  m_scriptComponent.setUpdateDelta(m_config.getInt("scriptDelta", 10));

  m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(player));
  m_scriptComponent.addCallbacks("player", LuaBindings::makePlayerCallbacks(as<Player>(player)));
  m_scriptComponent.addCallbacks(
      "status", LuaBindings::makeStatusControllerCallbacks(as<Player>(player)->statusController()));
  m_scriptComponent.addCallbacks("playerCompanions", makeCompanionsCallbacks());

  m_scriptComponent.addCallbacks("config",
      LuaBindings::makeConfigCallbacks([this](
          String const& name, Json const& def) { return m_config.query(name, def); }));

  m_scriptComponent.init(world);
}

void PlayerCompanions::uninit() {
  m_scriptComponent.uninit();
  m_scriptComponent.removeCallbacks("entity");
  m_scriptComponent.removeCallbacks("player");
  m_scriptComponent.removeCallbacks("status");
  m_scriptComponent.removeCallbacks("playerCompanions");
  m_scriptComponent.removeCallbacks("config");
  m_world = nullptr;
}

void PlayerCompanions::dismissCompanion(String const& category, Uuid const& podUuid) {
  m_scriptComponent.invoke("dismissCompanion", category, podUuid.hex());
}

Maybe<Json> PlayerCompanions::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  return m_scriptComponent.handleMessage(message, localMessage, args);
}

void PlayerCompanions::update(float dt) {
  m_scriptComponent.update(m_scriptComponent.updateDt(dt));
}

LuaCallbacks PlayerCompanions::makeCompanionsCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("getCompanions",
      [this](String const& category) {
        return m_companions[category].transformed([](CompanionPtr const& companion) { return companion->toJson(); });
      });
  callbacks.registerCallback("setCompanions",
      [this](String const& category, JsonArray const& companions) {
        m_companions[category] = companions.transformed([](Json const& json) { return make_shared<Companion>(json); });
      });

  return callbacks;
}

}
