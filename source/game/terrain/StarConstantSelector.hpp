#ifndef STAR_SOLID_SELECTOR_HPP
#define STAR_SOLID_SELECTOR_HPP

#include "StarTerrainDatabase.hpp"

namespace Star {

struct ConstantSelector : TerrainSelector {
  static char const* const Name;

  ConstantSelector(Json const& config, TerrainSelectorParameters const& parameters);

  float get(int x, int y) const override;

  float m_value;
};

}

#endif
