#include "StarMixSelector.hpp"
#include "StarInterpolation.hpp"

namespace Star {

char const* const MixSelector::Name = "mix";

MixSelector::MixSelector(Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database)
  : TerrainSelector(Name, config, parameters) {
  auto readSource = [&](Json const& sourceConfig) {
    String type = sourceConfig.getString("type");
    return database->createSelectorType(type, sourceConfig, parameters);
  };

  m_mixSource = readSource(config.get("mixSource"));
  m_aSource = readSource(config.get("aSource"));
  m_bSource = readSource(config.get("bSource"));
}

float MixSelector::get(int x, int y) const {
  auto f = clamp(m_mixSource->get(x, y), -1.0f, 1.0f);
  if (f == -1)
    return m_aSource->get(x, y);
  if (f == 1)
    return m_bSource->get(x, y);
  return lerp(f * 0.5f + 0.5f, m_aSource->get(x, y), m_bSource->get(x, y));
}

}
