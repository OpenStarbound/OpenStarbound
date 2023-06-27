#include "StarStatusEffectDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

StatusEffectDatabase::StatusEffectDatabase() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("statuseffect");
  assets->queueJsons(files);
  for (auto file : files) {
    auto uniqueEffect = parseUniqueEffect(assets->json(file), file);

    if (m_uniqueEffects.contains(uniqueEffect.name))
      throw StatusEffectDatabaseException::format(
          "Duplicate stat effect named '{}', config file '{}'", uniqueEffect.name, file);
    m_uniqueEffects[uniqueEffect.name] = uniqueEffect;
  }
}

bool StatusEffectDatabase::isUniqueEffect(UniqueStatusEffect const& effect) const {
  return m_uniqueEffects.contains(effect);
}

UniqueStatusEffectConfig StatusEffectDatabase::uniqueEffectConfig(UniqueStatusEffect const& effect) const {
  if (auto uniqueEffect = m_uniqueEffects.maybe(effect))
    return uniqueEffect.take();
  throw StatusEffectDatabaseException::format("No such unique stat effect '{}'", effect);
}

UniqueStatusEffectConfig StatusEffectDatabase::parseUniqueEffect(Json const& config, String const& path) const {
  try {
    auto assets = Root::singleton().assets();

    UniqueStatusEffectConfig effect;
    effect.name = config.getString("name");
    effect.blockingStat = config.optString("blockingStat");
    effect.effectConfig = config.get("effectConfig", JsonObject());
    effect.defaultDuration = config.getFloat("defaultDuration", 0.0f);
    effect.scripts =
        jsonToStringList(config.get("scripts", JsonArray{})).transformed(bind(&AssetPath::relativeTo, path, _1));
    effect.scriptDelta = config.getUInt("scriptDelta", 1);
    effect.animationConfig = config.optString("animationConfig").apply(bind(&AssetPath::relativeTo, path, _1));
    effect.label = config.getString("label", "");
    effect.description = config.getString("description", "");
    effect.icon = config.optString("icon").apply(bind(&AssetPath::relativeTo, path, _1));
    return effect;
  } catch (std::exception const& e) {
    throw StatusEffectDatabaseException("Error reading StatusEffect config", e);
  }
}

}
