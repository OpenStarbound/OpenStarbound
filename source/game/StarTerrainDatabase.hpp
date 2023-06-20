#ifndef STAR_TERRAIN_DATABASE_HPP
#define STAR_TERRAIN_DATABASE_HPP

#include "StarJson.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_STRUCT(TerrainSelector);
STAR_CLASS(TerrainDatabase);

STAR_EXCEPTION(TerrainException, StarException);

struct TerrainSelectorParameters {
  TerrainSelectorParameters();
  explicit TerrainSelectorParameters(Json const& v);

  Json toJson() const;

  TerrainSelectorParameters withSeed(uint64_t seed) const;
  TerrainSelectorParameters withCommonality(float commonality) const;

  unsigned worldWidth;
  float baseHeight;
  uint64_t seed;
  float commonality;
};

struct TerrainSelector {
  TerrainSelector(String type, Json config, TerrainSelectorParameters parameters);
  virtual ~TerrainSelector();

  // Returns a float signifying the "solid-ness" of a block, >= 0.0 should be
  // considered solid, < 0.0 should be considered open space.
  virtual float get(int x, int y) const = 0;

  String type;
  Json config;
  TerrainSelectorParameters parameters;
};

class TerrainDatabase {
public:
  struct Config {
    String type;
    Json parameters;
  };

  Config selectorConfig(String const& name) const;
  TerrainSelectorConstPtr createSelectorType(String const& type, Json const& config, TerrainSelectorParameters const& parameters) const;

  TerrainDatabase();

  TerrainSelectorConstPtr createNamedSelector(String const& name, TerrainSelectorParameters const& parameters) const;
  TerrainSelectorConstPtr constantSelector(float value);

  Json storeSelector(TerrainSelectorConstPtr const& selector) const;
  TerrainSelectorConstPtr loadSelector(Json const& store) const;

private:
  StringMap<Config> m_terrainSelectors;
};

}

#endif
