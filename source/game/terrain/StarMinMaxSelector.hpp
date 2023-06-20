#ifndef STAR_MIN_MAX_SELECTOR_HPP
#define STAR_MIN_MAX_SELECTOR_HPP

#include "StarTerrainDatabase.hpp"

namespace Star {

struct MinMaxSelector : TerrainSelector {
  static char const* const Name;

  MinMaxSelector(Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database);

  float get(int x, int y) const override;

  List<TerrainSelectorConstPtr> m_sources;
};

}

#endif
