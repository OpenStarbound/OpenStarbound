#include "StarPlayerFactory.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"

namespace Star {

PlayerConfig::PlayerConfig(JsonObject const& cfg) {
  defaultIdentity = HumanoidIdentity(cfg.value("defaultHumanoidIdentity"));
  humanoidTiming = Humanoid::HumanoidTiming(cfg.value("humanoidTiming"));

  for (Json v : cfg.value("defaultItems", JsonArray()).toArray())
    defaultItems.append(ItemDescriptor(v));

  for (Json v : cfg.value("defaultBlueprints", JsonObject()).getArray("tier1", JsonArray()))
    defaultBlueprints.append(ItemDescriptor(v));

  metaBoundBox = jsonToRectF(cfg.get("metaBoundBox"));

  movementParameters = cfg.get("movementParameters");
  zeroGMovementParameters = cfg.get("zeroGMovementParameters");
  statusControllerSettings = cfg.get("statusControllerSettings");

  footstepTiming = cfg.get("footstepTiming").toFloat();
  footstepSensor = jsonToVec2F(cfg.get("footstepSensor"));

  underwaterSensor = jsonToVec2F(cfg.get("underwaterSensor"));
  underwaterMinWaterLevel = cfg.get("underwaterMinWaterLevel").toFloat();

  splashConfig = EntitySplashConfig(cfg.get("splashConfig"));

  companionsConfig = cfg.get("companionsConfig");

  deploymentConfig = cfg.get("deploymentConfig");

  effectsAnimator = cfg.get("effectsAnimator").toString();

  teleportInTime = cfg.get("teleportInTime").toFloat();
  teleportOutTime = cfg.get("teleportOutTime").toFloat();

  deployInTime = cfg.get("deployInTime").toFloat();
  deployOutTime = cfg.get("deployOutTime").toFloat();

  bodyMaterialKind = cfg.get("bodyMaterialKind").toString();

  for (auto& p : cfg.get("genericScriptContexts").optObject().value(JsonObject()))
    genericScriptContexts[p.first] = p.second.toString();
}

PlayerFactory::PlayerFactory() : m_luaRoot(make_shared<LuaRoot>()) {
  auto assets = Root::singleton().assets();
  m_config = make_shared<PlayerConfig>(assets->json("/player.config").toObject());

  for (auto& path : assets->assetSources()) {
    auto metadata = assets->assetSourceMetadata(path);
    if (auto scripts = metadata.maybe("errorHandlers"))
      if (auto rebuildScripts = scripts.value().optArray("player"))
        m_rebuildScripts.insertAllAt(0, jsonToStringList(rebuildScripts.value()));
  }

}

PlayerPtr PlayerFactory::create() const {
  return make_shared<Player>(m_config);
}

PlayerPtr PlayerFactory::diskLoadPlayer(Json const& diskStore) const {
  try {
    return make_shared<Player>(m_config, diskStore);
  } catch (std::exception const& e) {
    auto exception = std::current_exception();
    auto error = strf("{}", outputException(e, false));
    Json newDiskStore = diskStore;
    for (auto script : m_rebuildScripts) {
      RecursiveMutexLocker locker(m_luaMutex);
      auto context = m_luaRoot->createContext(script);
      context.setCallbacks("root", LuaBindings::makeRootCallbacks());
      context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
      Json returnedDiskStore = context.invokePath<Json>("error", newDiskStore, error);
      if (returnedDiskStore != newDiskStore) {
        newDiskStore = returnedDiskStore;
        try {
          return make_shared<Player>(m_config, newDiskStore);
        } catch (std::exception const& e) {
          exception = std::current_exception();
          error = strf("{}", outputException(e, false));
        }
      }
    }
    std::rethrow_exception(exception);
  }
}

PlayerPtr PlayerFactory::netLoadPlayer(ByteArray const& netStore, NetCompatibilityRules rules) const {
  return make_shared<Player>(m_config, netStore, rules);
}

}
