#ifndef STAR_ROTATE_SELECTOR_HPP
#define STAR_ROTATE_SELECTOR_HPP

#include "StarTerrainDatabase.hpp"
#include "StarVector.hpp"

namespace Star {

struct RotateSelector : TerrainSelector {
  static char const* const Name;

  RotateSelector(Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database);

  float get(int x, int y) const override;

  float rotation;
  Vec2F rotationCenter;

  TerrainSelectorConstPtr m_source;
};

}

#endif
