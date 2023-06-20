#ifndef STAR_KARST_CAVE_HPP
#define STAR_KARST_CAVE_HPP

#include "StarTerrainDatabase.hpp"
#include "StarLruCache.hpp"
#include "StarVector.hpp"
#include "StarPerlin.hpp"

namespace Star {

class KarstCaveSelector : public TerrainSelector {
public:
  static char const* const Name;

  KarstCaveSelector(Json const& config, TerrainSelectorParameters const& parameters);

  float get(int x, int y) const override;

private:
  struct LayerPerlins {
    PerlinF caveDecision;
    PerlinF layerHeightVariation;
    PerlinF caveHeightVariation;
    PerlinF caveFloorVariation;
  };

  struct Sector {
    Sector(KarstCaveSelector const* parent, Vec2I sector);

    float get(int x, int y);

    bool inside(int x, int y);
    void set(int x, int y, float value);

    KarstCaveSelector const* parent;
    Vec2I sector;
    List<float> values;

    float m_maxValue;
  };

  LayerPerlins const& layerPerlins(int y) const;

  int m_sectorSize;
  int m_layerResolution;
  float m_layerDensity;
  int m_bufferHeight;
  float m_caveTaperPoint;

  Json m_caveDecisionPerlinConfig;
  Json m_layerHeightVariationPerlinConfig;
  Json m_caveHeightVariationPerlinConfig;
  Json m_caveFloorVariationPerlinConfig;

  int m_worldWidth;
  uint64_t m_seed;

  mutable HashLruCache<int, LayerPerlins> m_layerPerlinsCache;
  mutable HashLruCache<Vec2I, Sector> m_sectorCache;
};

}

#endif
