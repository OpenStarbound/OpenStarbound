#include "StarIslandSurfaceSelector.hpp"
#include "StarGameTypes.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

char const* const IslandSurfaceSelector::Name = "islandSurface";

IslandSurfaceSelector::IslandSurfaceSelector(Json const& config, TerrainSelectorParameters const& parameters)
  : TerrainSelector(Name, config, parameters) {
  layerBaseHeight = parameters.baseHeight;
  worldWidth = parameters.worldWidth;

  islandElevation = config.getFloat("islandElevation");
  islandTaperPoint = config.getFloat("islandTaperPoint");

  islandHeight = PerlinF(config.getObject("islandHeight"), staticRandomU64(parameters.seed, parameters.baseHeight, "islandHeight"));
  islandDepth = PerlinF(config.getObject("islandDepth"), staticRandomU64(parameters.seed, parameters.baseHeight, "islandDepth"));
  islandDecision = PerlinF(config.getObject("islandDecision"), staticRandomU64(parameters.seed, parameters.baseHeight, "islandDecision"));
}

IslandColumn IslandSurfaceSelector::generateColumn(int x) const {
  float noiseAngle = 2 * Constants::pi * x / worldWidth;
  float noiseX = (std::cos(noiseAngle) * worldWidth) / (2 * Constants::pi);
  float noiseY = (std::sin(noiseAngle) * worldWidth) / (2 * Constants::pi);

  IslandColumn newCol;
  float thisIslandDecision = islandDecision.get(noiseX, noiseY);
  if (thisIslandDecision > 0) {
    float taperFactor = thisIslandDecision < islandTaperPoint ? std::sin((0.5 * Constants::pi * thisIslandDecision) / islandTaperPoint) : 1.0f;
    newCol.topLevel = islandElevation + layerBaseHeight + taperFactor * islandHeight.get(noiseX, noiseY);
    newCol.bottomLevel = islandElevation + layerBaseHeight - taperFactor * islandDepth.get(noiseX, noiseY);
  } else {
    newCol.topLevel = layerBaseHeight;
    newCol.bottomLevel = layerBaseHeight;
  }

  return newCol;
}

float IslandSurfaceSelector::get(int x, int y) const {
  auto col = columnCache.get(x, [=](int x) {
      return IslandSurfaceSelector::generateColumn(x);
    });

  return (col.topLevel - col.bottomLevel) / 2 - abs((col.topLevel + col.bottomLevel) / 2 - y);
}

}
