#ifndef STAR_BIOME_DATABASE_HPP
#define STAR_BIOME_DATABASE_HPP

#include "StarBiome.hpp"
#include "StarWeatherTypes.hpp"
#include "StarSkyTypes.hpp"

namespace Star {

STAR_CLASS(BiomeDatabase);

class BiomeDatabase {
public:
  BiomeDatabase();

  StringList biomeNames() const;

  float biomeHueShift(String const& biomeName, uint64_t seed) const;
  WeatherPool biomeWeathers(String const& biomeName, uint64_t seed, float threatLevel) const;
  bool biomeIsAirless(String const& biomeName) const;
  SkyColoring biomeSkyColoring(String const& biomeName, uint64_t seed) const;
  String biomeFriendlyName(String const& biomeName) const;
  StringList biomeStatusEffects(String const& biomeName) const;
  StringList biomeOres(String const& biomeName, float threatLevel) const;

  StringList weatherNames() const;
  WeatherType weatherType(String const& weatherName) const;

  BiomePtr createBiome(String const& biomeName, uint64_t seed, float verticalMidPoint, float threatLevel) const;

private:
  struct Config {
    String path;
    String name;
    Json parameters;
  };
  typedef StringMap<Config> ConfigMap;

  static float pickHueShiftFromJson(Json source, uint64_t seed, String const& key);

  BiomePlaceables readBiomePlaceables(Json const& config, uint64_t seed, float biomeHueShift) const;
  List<pair<ModId, float>> readOres(Json const& oreDistribution, float threatLevel) const;

  ConfigMap m_biomes;
  ConfigMap m_weathers;
};

}

#endif
