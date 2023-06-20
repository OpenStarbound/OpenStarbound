#ifndef STAR_PERLIN_SELECTOR_HPP
#define STAR_PERLIN_SELECTOR_HPP

#include "StarTerrainDatabase.hpp"
#include "StarPerlin.hpp"

namespace Star {

struct PerlinSelector : TerrainSelector {
  static char const* const Name;

  PerlinSelector(Json const& config, TerrainSelectorParameters const& parameters);

  float get(int x, int y) const override;

  PerlinF function;

  float xInfluence;
  float yInfluence;
};

}

#endif
