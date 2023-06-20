#include "StarFlatSurfaceSelector.hpp"

namespace Star {

char const* const FlatSurfaceSelector::Name = "flatSurface";

FlatSurfaceSelector::FlatSurfaceSelector(Json const& config, TerrainSelectorParameters const& parameters)
  : TerrainSelector(Name, config, parameters) {
  surfaceLevel = parameters.baseHeight;
  adjustment = config.getFloat("adjustment", 0);
  flip = config.getBool("flip", false) ? -1 : 1;
}

float FlatSurfaceSelector::get(int, int y) const {
  return flip * (surfaceLevel - (y - adjustment));
}

}
