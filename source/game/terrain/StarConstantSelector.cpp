#include "StarConstantSelector.hpp"

namespace Star {

char const* const ConstantSelector::Name = "constant";

ConstantSelector::ConstantSelector(Json const& config, TerrainSelectorParameters const& parameters)
  : TerrainSelector(Name, config, parameters) {
  m_value = config.getFloat("value");
}

float ConstantSelector::get(int, int) const {
  return m_value;
}

}
