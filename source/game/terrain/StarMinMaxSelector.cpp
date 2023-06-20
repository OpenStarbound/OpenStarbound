#include "StarMinMaxSelector.hpp"
#include "StarMathCommon.hpp"

namespace Star {

char const* const MinMaxSelector::Name = "minmax";

MinMaxSelector::MinMaxSelector(
    Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database)
  : TerrainSelector(Name, config, parameters) {
  for (auto const& sourceConfig : config.getArray("sources")) {
    String sourceType = sourceConfig.getString("type");
    uint64_t seedBias = sourceConfig.getUInt("seedBias", 0);
    TerrainSelectorParameters sourceParameters = parameters;
    sourceParameters.seed += seedBias;
    m_sources.append(database->createSelectorType(sourceType, sourceConfig, sourceParameters));
  }
}

float MinMaxSelector::get(int x, int y) const {
  float value = 0.0f;
  for (auto const& source : m_sources) {
    float srcVal = source->get(x, y);
    if (value > 0 || srcVal > 0)
      value = max(value, srcVal);
    else
      value = min(value, srcVal);
  }
  return value;
}

}
