#pragma once

#include "StarTerrainDatabase.hpp"
#include "StarPerlin.hpp"
#include "StarVector.hpp"

namespace Star {

struct DisplacementSelector : TerrainSelector {
  static char const* const Name;

  DisplacementSelector(
      Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database);

  float get(int x, int y) const override;

  PerlinF xDisplacementFunction;
  PerlinF yDisplacementFunction;

  float xXInfluence;
  float xYInfluence;
  float yXInfluence;
  float yYInfluence;

  bool yClamp;
  Vec2F yClampRange;
  float yClampSmoothing;

  float clampY(float v) const;

  TerrainSelectorConstPtr m_source;
};

}
