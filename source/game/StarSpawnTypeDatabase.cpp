#include "StarSpawnTypeDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

SpawnParameters::SpawnParameters(Set<Area> areas, Region region, Time time) : areas(areas), region(region), time(time) {}

SpawnParameters::SpawnParameters(Json const& config) {
  if (config.isNull()) {
    areas = {Area::Surface, Area::Ceiling, Area::Air, Area::Liquid, Area::Solid};
    region = Region::All;
    time = Time::All;
    return;
  }

  if (auto areaName = config.optString("area")) {
    if (*areaName == "all")
      areas = {Area::Surface, Area::Ceiling, Area::Air, Area::Liquid, Area::Solid};
    else
      areas.add(SpawnParameters::AreaNames.getLeft(*areaName));
  } else if (auto areaNames = config.optArray("areas")) {
    for (auto areaName : *areaNames)
      areas.add(SpawnParameters::AreaNames.getLeft(areaName.toString()));
  }
  region = SpawnParameters::RegionNames.getLeft(config.getString("region"));
  time = SpawnParameters::TimeNames.getLeft(config.getString("time"));
}

bool SpawnParameters::compatible(SpawnParameters const& parameters) const {
  return areas.hasIntersection(parameters.areas)
      && (region == Region::All || parameters.region == Region::All || region == parameters.region)
      && (time == Time::All || parameters.time == Time::All || time == parameters.time);
}

EnumMap<SpawnParameters::Area> const SpawnParameters::AreaNames{
  {SpawnParameters::Area::Surface, "surface"},
  {SpawnParameters::Area::Ceiling, "ceiling"},
  {SpawnParameters::Area::Air, "air"},
  {SpawnParameters::Area::Liquid, "liquid"},
  {SpawnParameters::Area::Solid, "solid"}
};

EnumMap<SpawnParameters::Region> const SpawnParameters::RegionNames{
  {SpawnParameters::Region::All, "all"},
  {SpawnParameters::Region::Enclosed, "enclosed"},
  {SpawnParameters::Region::Exposed, "exposed"}
};

EnumMap<SpawnParameters::Time> const SpawnParameters::TimeNames{
  {SpawnParameters::Time::All, "all"},
  {SpawnParameters::Time::Day, "day"},
  {SpawnParameters::Time::Night, "night"}
};

SpawnType spawnTypeFromJson(Json const& config) {
  SpawnType spawnType;
  spawnType.typeName = config.getString("name");
  spawnType.dayLevelAdjustment = jsonToVec2F(config.get("dayLevelAdjustment", JsonArray{0, 0}));
  spawnType.nightLevelAdjustment = jsonToVec2F(config.get("nightLevelAdjustment", JsonArray{0, 0}));

  Json monsterType = config.get("monsterType");
  if (monsterType.type() == Json::Type::Array)
    spawnType.monsterType = jsonToWeightedPool<String>(monsterType);
  else
    spawnType.monsterType = monsterType.toString();

  spawnType.monsterParameters = config.get("monsterParameters", JsonObject{});
  spawnType.groupSize = jsonToVec2I(config.get("groupSize", JsonArray{1, 1}));
  spawnType.spawnChance = config.getFloat("spawnChance");
  spawnType.spawnParameters = SpawnParameters(config.get("spawnParameters", {}));
  spawnType.seedMix = config.getUInt("seedMix", 0);

  return spawnType;
}

SpawnProfile::SpawnProfile() {}

SpawnProfile::SpawnProfile(Json const& config) {
  spawnTypes = jsonToStringSet(config.get("spawnTypes", JsonArray()));
  monsterParameters = config.get("monsterParameters", {});
}

SpawnProfile::SpawnProfile(StringSet spawnTypes, Json monsterParameters)
  : spawnTypes(move(spawnTypes)), monsterParameters(move(monsterParameters)) {}

Json SpawnProfile::toJson() const {
  return JsonObject{
    {"spawnTypes", jsonFromStringSet(spawnTypes)},
    {"monsterParameters", monsterParameters}
  };
}

SpawnProfile constructSpawnProfile(Json const& config, uint64_t seed) {
  SpawnProfile spawnProfile;
  auto commonGroups = Root::singleton().assets()->json("/spawning.config:spawnGroups");
  for (auto group : config.get("groups", JsonArray()).iterateArray()) {
    auto poolNameOrConfig = group.get("pool");
    WeightedPool<String> typePool;
    if (poolNameOrConfig.canConvert(Json::Type::String))
      typePool = jsonToWeightedPool<String>(commonGroups.get(poolNameOrConfig.toString()));
    else
      typePool = jsonToWeightedPool<String>(poolNameOrConfig);

    spawnProfile.spawnTypes.addAll(typePool.selectUniques(group.getUInt("select"), ++seed));
  }

  spawnProfile.monsterParameters = config.getObject("monsterParameters", JsonObject());

  return spawnProfile;
}

SpawnTypeDatabase::SpawnTypeDatabase() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("spawntypes");
  assets->queueJsons(files);
  uint64_t seedMix = 0;
  for (auto const& file : files) {
    try {
      auto spawnTypes = assets->json(file);

      for (auto const& entry : spawnTypes.iterateArray()) {
        auto spawnType = spawnTypeFromJson(entry);

        if (m_spawnTypes.contains(spawnType.typeName))
          throw SpawnTypeDatabaseException::format("Duplicate spawnType named '{}' in config file '{}'", spawnType.typeName, file);

        if (!entry.contains("seedMix"))
          spawnType.seedMix = ++seedMix;

        m_spawnTypes[spawnType.typeName] = spawnType;
      }
    } catch (std::exception const& e) {
      throw SpawnTypeDatabaseException(strf("Error reading spawnType config file {}", file), e);
    }
  }
}

SpawnType SpawnTypeDatabase::spawnType(String const& typeName) const {
  if (auto spawnType = m_spawnTypes.maybe(typeName))
    return spawnType.take();
  throw SpawnTypeDatabaseException::format("No such spawnType '{}'", typeName);
}

}
