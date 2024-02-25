#pragma once

#include "StarTerrainDatabase.hpp"
#include "StarLruCache.hpp"
#include "StarVector.hpp"

namespace Star {

struct CacheSelector : TerrainSelector {
  static char const* const Name;

  CacheSelector(Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database);

  float get(int x, int y) const override;

  TerrainSelectorConstPtr m_source;
  mutable HashLruCache<Vec2I, float> m_cache;
};

}
