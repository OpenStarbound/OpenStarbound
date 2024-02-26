#pragma once

#include "StarJson.hpp"
#include "StarVector.hpp"
#include "StarBiMap.hpp"
#include "StarWeightedPool.hpp"

namespace Star {

STAR_EXCEPTION(SpawnTypeDatabaseException, StarException);

STAR_CLASS(SpawnTypeDatabase);

struct SpawnParameters {
  enum class Area : uint8_t {
    Surface,
    Ceiling,
    Air,
    Liquid,
    Solid
  };

  enum class Region : uint8_t {
    All,
    Enclosed,
    Exposed
  };

  enum class Time : uint8_t {
    All,
    Day,
    Night
  };

  static EnumMap<Area> const AreaNames;
  static EnumMap<Region> const RegionNames;
  static EnumMap<Time> const TimeNames;

  // Null config constructs SpawnParameters filled with All
  SpawnParameters(Json const& config = {});
  SpawnParameters(Set<Area> areas, Region region, Time time);

  bool compatible(SpawnParameters const& parameters) const;

  Set<Area> areas;
  Region region;
  Time time;
};

struct SpawnType {
  String typeName;

  Vec2F dayLevelAdjustment;
  Vec2F nightLevelAdjustment;

  Variant<String, WeightedPool<String>> monsterType;
  Json monsterParameters;

  Vec2I groupSize;
  float spawnChance;

  SpawnParameters spawnParameters;
  uint64_t seedMix;
};

SpawnType spawnTypeFromJson(Json const& config);

struct SpawnProfile {
  SpawnProfile();
  SpawnProfile(Json const& config);
  SpawnProfile(StringSet spawnTypes, Json monsterParameters);

  Json toJson() const;

  StringSet spawnTypes;
  Json monsterParameters;
};

SpawnProfile constructSpawnProfile(Json const& config, uint64_t seed);

class SpawnTypeDatabase {
public:
  SpawnTypeDatabase();

  SpawnType spawnType(String const& typeName) const;

private:
  StringMap<SpawnType> m_spawnTypes;
};

}
