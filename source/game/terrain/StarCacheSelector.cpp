#include "StarCacheSelector.hpp"

namespace Star {

char const* const CacheSelector::Name = "cache";

CacheSelector::CacheSelector(
    Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database)
  : TerrainSelector(Name, config, parameters) {
  auto sourceConfig = config.get("source");
  String sourceType = sourceConfig.getString("type");
  uint64_t seedBias = sourceConfig.getUInt("seedBias", 0);
  TerrainSelectorParameters sourceParameters = parameters;
  sourceParameters.seed += seedBias;
  m_source = database->createSelectorType(type, sourceConfig, sourceParameters);

  m_cache.setMaxSize(config.getUInt("lruCacheSize", 20000));
}

float CacheSelector::get(int x, int y) const {
  return m_cache.get(Vec2I(x, y), [this](Vec2I const& key) {
      return m_source->get(key[0], key[1]);
    });
}

}
