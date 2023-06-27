#include "StarBiomeDatabase.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarStoredFunctions.hpp"
#include "StarParallax.hpp"
#include "StarAmbient.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarAssets.hpp"
#include "StarSpawnTypeDatabase.hpp"

namespace Star {

BiomeDatabase::BiomeDatabase() {
  auto assets = Root::singleton().assets();

  // 'type' here is the extension of the file, and determines the selector type
  auto scanFiles = [=](String const& type, ConfigMap& map) {
    auto files = assets->scanExtension(type);
    assets->queueJsons(files);
    for (auto path : files) {
      auto parameters = assets->json(path);
      if (parameters.isNull())
        continue;

      auto name = parameters.getString("name");
      if (map.contains(name))
        throw BiomeException(strf("Duplicate {} generator name '{}'", type, name));
      map[name] = {path, name, parameters};
    }
  };

  scanFiles("biome", m_biomes);
  scanFiles("weather", m_weathers);
}

StringList BiomeDatabase::biomeNames() const {
  return m_biomes.keys();
}

float BiomeDatabase::biomeHueShift(String const& biomeName, uint64_t seed) const {
  auto const& config = m_biomes.get(biomeName);
  return pickHueShiftFromJson(config.parameters.get("hueShiftOptions", {}), seed, "BiomeHueShift");
}

WeatherPool BiomeDatabase::biomeWeathers(String const& biomeName, uint64_t seed, float threatLevel) const {
  WeatherPool weatherPool;
  if (auto weatherList = binnedChoiceFromJson(m_biomes.get(biomeName).parameters.get("weather", JsonArray{}), threatLevel).optArray()) {
    auto weatherPoolPath = staticRandomFrom(*weatherList, seed, "WeatherPool");

    auto assets = Root::singleton().assets();
    auto weatherPoolConfig = assets->fetchJson(weatherPoolPath);
    weatherPool = jsonToWeightedPool<String>(weatherPoolConfig);
  }

  return weatherPool;
}

bool BiomeDatabase::biomeIsAirless(String const& biomeName) const {
  auto const& config = m_biomes.get(biomeName);
  return config.parameters.getBool("airless", false);
}

SkyColoring BiomeDatabase::biomeSkyColoring(String const& biomeName, uint64_t seed) const {
  SkyColoring skyColoring;

  auto const& config = m_biomes.get(biomeName);
  if (auto skyOptions = config.parameters.optArray("skyOptions")) {
    auto option = staticRandomFrom(*skyOptions, seed, "BiomeSkyOption");

    skyColoring.mainColor = jsonToColor(option.get("mainColor"));

    skyColoring.morningColors.first = jsonToColor(option.query("morningColors[0]"));
    skyColoring.morningColors.second = jsonToColor(option.query("morningColors[1]"));

    skyColoring.dayColors.first = jsonToColor(option.query("dayColors[0]"));
    skyColoring.dayColors.second = jsonToColor(option.query("dayColors[1]"));

    skyColoring.eveningColors.first = jsonToColor(option.query("eveningColors[0]"));
    skyColoring.eveningColors.second = jsonToColor(option.query("eveningColors[1]"));

    skyColoring.nightColors.first = jsonToColor(option.query("nightColors[0]"));
    skyColoring.nightColors.second = jsonToColor(option.query("nightColors[1]"));

    skyColoring.morningLightColor = jsonToColor(option.get("morningLightColor"));
    skyColoring.dayLightColor = jsonToColor(option.get("dayLightColor"));
    skyColoring.eveningLightColor = jsonToColor(option.get("eveningLightColor"));
    skyColoring.nightLightColor = jsonToColor(option.get("nightLightColor"));
  }

  return skyColoring;
}

String BiomeDatabase::biomeFriendlyName(String const& biomeName) const {
  auto const& config = m_biomes.get(biomeName);
  return config.parameters.getString("friendlyName");
}

StringList BiomeDatabase::biomeStatusEffects(String const& biomeName) const {
  auto const& config = m_biomes.get(biomeName);
  return config.parameters.opt("statusEffects").apply(jsonToStringList).value();
}

StringList BiomeDatabase::biomeOres(String const& biomeName, float threatLevel) const {
  StringList res;

  auto const& config = m_biomes.get(biomeName);
  auto oreDistribution = config.parameters.get("ores", {});
  if (!oreDistribution.isNull()) {
    auto& root = Root::singleton();
    auto functionDatabase = root.functionDatabase();

    auto oresList = functionDatabase->configFunction(oreDistribution)->get(threatLevel);
    for (Json v : oresList.iterateArray()) {
      if (v.getFloat(1) > 0)
        res.append(v.getString(0));
    }
  }

  return res;
}

StringList BiomeDatabase::weatherNames() const {
  return m_weathers.keys();
}

WeatherType BiomeDatabase::weatherType(String const& name) const {
  if (!m_weathers.contains(name))
    throw BiomeException(strf("No such weather type '{}'", name));

  auto config = m_weathers.get(name);

  try {
    return WeatherType(config.parameters, config.path);
  } catch (MapException const& e) {
    throw BiomeException(strf("Required key not found in weather config {}", config.path), e);
  }
}

BiomePtr BiomeDatabase::createBiome(String const& biomeName, uint64_t seed, float verticalMidPoint, float threatLevel) const {
  if (!m_biomes.contains(biomeName))
    throw BiomeException(strf("No such biome '{}'", biomeName));

  auto& root = Root::singleton();
  auto materialDatabase = root.materialDatabase();

  try {
    RandomSource random(seed);
    auto config = m_biomes.get(biomeName);

    auto biome = make_shared<Biome>();
    float mainHueShift = biomeHueShift(biomeName, seed);

    biome->baseName = biomeName;
    biome->description = config.parameters.getString("description", "");

    if (config.parameters.contains("mainBlock"))
      biome->mainBlock = materialDatabase->materialId(config.parameters.getString("mainBlock"));

    for (Json v : config.parameters.getArray("subBlocks", {}))
      biome->subBlocks.append(materialDatabase->materialId(v.toString()));

    biome->ores = readOres(config.parameters.get("ores", {}), threatLevel);

    biome->surfacePlaceables = readBiomePlaceables(config.parameters.getObject("surfacePlaceables", {}), random.randu64(), mainHueShift);
    biome->undergroundPlaceables = readBiomePlaceables(config.parameters.getObject("undergroundPlaceables", {}), random.randu64(), mainHueShift);

    biome->hueShift = mainHueShift;
    biome->materialHueShift = materialHueFromDegrees(biome->hueShift);

    if (config.parameters.contains("parallax")) {
      auto parallaxFile = AssetPath::relativeTo(config.path, config.parameters.getString("parallax"));
      biome->parallax = make_shared<Parallax>(parallaxFile, seed, verticalMidPoint, mainHueShift, biome->surfacePlaceables.firstTreeType());
    }

    if (config.parameters.contains("musicTrack"))
      biome->musicTrack = make_shared<AmbientNoisesDescription>(config.parameters.getObject("musicTrack"), config.path);

    if (config.parameters.contains("ambientNoises"))
      biome->ambientNoises = make_shared<AmbientNoisesDescription>(config.parameters.getObject("ambientNoises"), config.path);

    if (config.parameters.contains("spawnProfile"))
      biome->spawnProfile = constructSpawnProfile(config.parameters.getObject("spawnProfile"), seed);

    return biome;
  } catch (std::exception const& cause) {
    throw BiomeException(strf("Failed to parse biome: '{}'", biomeName), cause);
  }
}

float BiomeDatabase::pickHueShiftFromJson(Json source, uint64_t seed, String const& key) {
  if (source.isNull())
    return 0;
  auto options = jsonToFloatList(source);
  if (options.size() == 0)
    return 0;
  auto t = staticRandomU32(seed, key);
  return options.at(t % options.size());
}

BiomePlaceables BiomeDatabase::readBiomePlaceables(Json const& config, uint64_t seed, float biomeHueShift) const {
  auto& root = Root::singleton();
  RandomSource rand(seed);
  BiomePlaceables placeables;
  if (config.contains("grassMod") && !config.getArray("grassMod").empty())
    placeables.grassMod = root.materialDatabase()->modId(rand.randFrom(config.getArray("grassMod")).toString());
  placeables.grassModDensity = config.getFloat("grassModDensity", 0);
  if (config.contains("ceilingGrassMod") && !config.getArray("ceilingGrassMod").empty())
    placeables.ceilingGrassMod = root.materialDatabase()->modId(rand.randFrom(config.getArray("ceilingGrassMod")).toString());
  placeables.ceilingGrassModDensity = config.getFloat("ceilingGrassModDensity", 0);

  for (auto const& itemConfig : config.getArray("items", {}))
    placeables.itemDistributions.append(BiomeItemDistribution(itemConfig, rand.randu64(), biomeHueShift));

  return placeables;
}

List<pair<ModId, float>> BiomeDatabase::readOres(Json const& oreDistribution, float threatLevel) const {
  List<pair<ModId, float>> ores;
  if (!oreDistribution.isNull()) {
    auto& root = Root::singleton();
    auto functionDatabase = root.functionDatabase();
    auto materialDatabase = root.materialDatabase();

    auto oresList = functionDatabase->configFunction(oreDistribution)->get(threatLevel);
    for (Json v : oresList.iterateArray()) {
      if (v.getFloat(1) > 0)
        ores.append({materialDatabase->modId(v.getString(0)), v.getFloat(1)});
    }
  }
  return ores;
}

}
