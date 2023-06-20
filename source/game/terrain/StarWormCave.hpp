#ifndef STAR_WORM_CAVE_HPP
#define STAR_WORM_CAVE_HPP

#include "StarTerrainDatabase.hpp"
#include "StarLruCache.hpp"
#include "StarVector.hpp"

namespace Star {

class WormCaveSector {
public:
  WormCaveSector(int sectorSize, Vec2I sector, Json const& config, size_t seed, float commonality);

  float get(int x, int y);

private:
  bool inside(int x, int y);
  void set(int x, int y, float value);

  int m_sectorSize;
  Vec2I m_sector;
  List<float> m_values;

  float m_maxValue;
};

class WormCaveSelector : public TerrainSelector {
public:
  static char const* const Name;

  WormCaveSelector(Json const& config, TerrainSelectorParameters const& parameters);

  float get(int x, int y) const override;

private:
  int m_sectorSize;
  mutable HashLruCache<Vec2I, WormCaveSector> m_cache;
};

}

#endif
