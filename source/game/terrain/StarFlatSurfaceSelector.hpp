#ifndef STAR_FLAT_SURFACE_SELECTOR_HPP
#define STAR_FLAT_SURFACE_SELECTOR_HPP

#include "StarTerrainDatabase.hpp"

namespace Star {

struct FlatSurfaceSelector : TerrainSelector {
  static char const* const Name;

  FlatSurfaceSelector(Json const& config, TerrainSelectorParameters const& parameters);

  float get(int x, int y) const override;

  float surfaceLevel;
  float adjustment;
  float flip;
};

}

#endif
