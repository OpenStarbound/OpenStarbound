#pragma once

#include "StarTerrainDatabase.hpp"

namespace Star {

struct MixSelector : TerrainSelector {
  static char const* const Name;

  MixSelector(Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database);

  float get(int x, int y) const override;

  TerrainSelectorConstPtr m_mixSource;
  TerrainSelectorConstPtr m_aSource;
  TerrainSelectorConstPtr m_bSource;
};

}
