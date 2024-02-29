#include "StarWorldParameters.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarBiomeDatabase.hpp"
#include "StarLiquidsDatabase.hpp"

namespace Star {

EnumMap<WorldParametersType> const WorldParametersTypeNames{
    {WorldParametersType::TerrestrialWorldParameters, "TerrestrialWorldParameters"},
    {WorldParametersType::AsteroidsWorldParameters, "AsteroidsWorldParameters"},
    {WorldParametersType::FloatingDungeonWorldParameters, "FloatingDungeonWorldParameters"}};

EnumMap<BeamUpRule> const BeamUpRuleNames{
  {BeamUpRule::Nowhere, "Nowhere"},
  {BeamUpRule::Surface, "Surface"},
  {BeamUpRule::Anywhere, "Anywhere"},
  {BeamUpRule::AnywhereWithWarning, "AnywhereWithWarning"}};

EnumMap<WorldEdgeForceRegionType> const WorldEdgeForceRegionTypeNames{
  {WorldEdgeForceRegionType::None, "None"},
  {WorldEdgeForceRegionType::Top, "Top"},
  {WorldEdgeForceRegionType::Bottom, "Bottom"},
  {WorldEdgeForceRegionType::TopAndBottom, "TopAndBottom"}};

VisitableWorldParameters::VisitableWorldParameters(Json const& store) {
  typeName = store.getString("typeName", "");
  threatLevel = store.getFloat("threatLevel");
  worldSize = jsonToVec2U(store.get("worldSize"));
  gravity = store.getFloat("gravity", 1.0f);
  airless = store.getBool("airless", false);
  weatherPool = jsonToWeightedPool<String>(store.getArray("weatherPool", JsonArray()));
  environmentStatusEffects = store.opt("environmentStatusEffects").apply(jsonToStringList).value();
  overrideTech = store.opt("overrideTech").apply(jsonToStringList);
  globalDirectives = store.opt("globalDirectives").apply(jsonToDirectivesList);
  beamUpRule = BeamUpRuleNames.getLeft(store.getString("beamUpRule", "Surface"));
  disableDeathDrops = store.getBool("disableDeathDrops", false);
  terraformed = store.getBool("terraformed", false);
  worldEdgeForceRegions = WorldEdgeForceRegionTypeNames.getLeft(store.getString("worldEdgeForceRegions", "None"));
}

Json VisitableWorldParameters::store() const {
  return JsonObject{{"typeName", typeName},
      {"threatLevel", threatLevel},
      {"worldSize", jsonFromVec2U(worldSize)},
      {"gravity", gravity},
      {"airless", airless},
      {"weatherPool", jsonFromWeightedPool<String>(weatherPool)},
      {"environmentStatusEffects", jsonFromStringList(environmentStatusEffects)},
      {"overrideTech", jsonFromMaybe(overrideTech.apply(&jsonFromStringList))},
      {"globalDirectives", jsonFromMaybe(globalDirectives.apply(&jsonFromDirectivesList))},
      {"beamUpRule", BeamUpRuleNames.getRight(beamUpRule)},
      {"disableDeathDrops", disableDeathDrops},
      {"terraformed", terraformed},
      {"worldEdgeForceRegions", WorldEdgeForceRegionTypeNames.getRight(worldEdgeForceRegions)}};
}

void VisitableWorldParameters::read(DataStream& ds) {
  ds >> typeName;
  ds >> threatLevel;
  ds >> worldSize;
  ds >> gravity;
  ds >> airless;
  weatherPool = WeatherPool(ds.read<WeatherPool::ItemsList>());
  ds >> environmentStatusEffects;
  ds >> overrideTech;
  ds >> globalDirectives;
  ds >> beamUpRule;
  ds >> disableDeathDrops;
  ds >> terraformed;
  ds >> worldEdgeForceRegions;
}

void VisitableWorldParameters::write(DataStream& ds) const {
  ds << typeName;
  ds << threatLevel;
  ds << worldSize;
  ds << gravity;
  ds << airless;
  ds.writeContainer<WeatherPool::ItemsList>(weatherPool.items());
  ds << environmentStatusEffects;
  ds << overrideTech;
  ds << globalDirectives;
  ds << beamUpRule;
  ds << disableDeathDrops;
  ds << terraformed;
  ds << worldEdgeForceRegions;
}

TerrestrialWorldParameters::TerrestrialWorldParameters(Json const& store) : VisitableWorldParameters(store) {
  auto loadTerrestrialRegion = [](Json const& config) {
    return TerrestrialRegion{config.getString("biome"),
        config.getString("blockSelector"),
        config.getString("fgCaveSelector"),
        config.getString("bgCaveSelector"),
        config.getString("fgOreSelector"),
        config.getString("bgOreSelector"),
        config.getString("subBlockSelector"),
        static_cast<LiquidId>(config.getUInt("caveLiquid")),
        config.getFloat("caveLiquidSeedDensity"),
        static_cast<LiquidId>(config.getUInt("oceanLiquid")),
        static_cast<int>(config.getInt("oceanLiquidLevel")),
        config.getBool("encloseLiquids"),
        config.getBool("fillMicrodungeons")};
  };

  auto loadTerrestrialLayer = [loadTerrestrialRegion](Json const& config) {
    return TerrestrialLayer{static_cast<int>(config.getInt("layerMinHeight")),
        static_cast<int>(config.getInt("layerBaseHeight")),
        jsonToStringList(config.get("dungeons")),
        static_cast<int>(config.getInt("dungeonXVariance")),
        loadTerrestrialRegion(config.get("primaryRegion")),
        loadTerrestrialRegion(config.get("primarySubRegion")),
        config.getArray("secondaryRegions").transformed(loadTerrestrialRegion),
        config.getArray("secondarySubRegions").transformed(loadTerrestrialRegion),
        jsonToVec2F(config.get("secondaryRegionSizeRange")),
        jsonToVec2F(config.get("subRegionSizeRange"))};
  };

  primaryBiome = store.getString("primaryBiome");
  primarySurfaceLiquid = store.getUInt("surfaceLiquid");
  sizeName = store.getString("sizeName");
  hueShift = store.getFloat("hueShift");
  skyColoring = SkyColoring(store.get("skyColoring"));
  dayLength = store.getFloat("dayLength");
  blockNoiseConfig = store.get("blockNoise");
  blendNoiseConfig = store.get("blendNoise");
  blendSize = store.getFloat("blendSize");

  spaceLayer = loadTerrestrialLayer(store.get("spaceLayer"));
  atmosphereLayer = loadTerrestrialLayer(store.get("atmosphereLayer"));
  surfaceLayer = loadTerrestrialLayer(store.get("surfaceLayer"));
  subsurfaceLayer = loadTerrestrialLayer(store.get("subsurfaceLayer"));
  undergroundLayers = store.getArray("undergroundLayers").transformed(loadTerrestrialLayer);
  coreLayer = loadTerrestrialLayer(store.get("coreLayer"));
}

TerrestrialWorldParameters &TerrestrialWorldParameters::operator=(TerrestrialWorldParameters const& terrestrialWorldParameters) {
  this->primaryBiome = terrestrialWorldParameters.primaryBiome;
  this->primarySurfaceLiquid = terrestrialWorldParameters.primarySurfaceLiquid;
  this->sizeName = terrestrialWorldParameters.sizeName;
  this->hueShift = terrestrialWorldParameters.hueShift;

  this->skyColoring = terrestrialWorldParameters.skyColoring;
  this->dayLength = terrestrialWorldParameters.dayLength;

  this->blockNoiseConfig = terrestrialWorldParameters.blockNoiseConfig;
  this->blendNoiseConfig = terrestrialWorldParameters.blendNoiseConfig;
  this->blendSize = terrestrialWorldParameters.blendSize;

  this->spaceLayer = terrestrialWorldParameters.spaceLayer;
  this->atmosphereLayer = terrestrialWorldParameters.atmosphereLayer;
  this->surfaceLayer = terrestrialWorldParameters.surfaceLayer;
  this->subsurfaceLayer = terrestrialWorldParameters.subsurfaceLayer;
  this->undergroundLayers = terrestrialWorldParameters.undergroundLayers;
  this->coreLayer = terrestrialWorldParameters.coreLayer;
  return *this;
}

WorldParametersType TerrestrialWorldParameters::type() const {
  return WorldParametersType::TerrestrialWorldParameters;
}

Json TerrestrialWorldParameters::store() const {
  auto storeTerrestrialRegion = [](TerrestrialRegion const& region) -> Json {
    return JsonObject{{"biome", region.biome},
        {"blockSelector", region.blockSelector},
        {"fgCaveSelector", region.fgCaveSelector},
        {"bgCaveSelector", region.bgCaveSelector},
        {"fgOreSelector", region.fgOreSelector},
        {"bgOreSelector", region.bgOreSelector},
        {"subBlockSelector", region.subBlockSelector},
        {"caveLiquid", region.caveLiquid},
        {"caveLiquidSeedDensity", region.caveLiquidSeedDensity},
        {"oceanLiquid", region.oceanLiquid},
        {"oceanLiquidLevel", region.oceanLiquidLevel},
        {"encloseLiquids", region.encloseLiquids},
        {"fillMicrodungeons", region.fillMicrodungeons}};
  };
  auto storeTerrestrialLayer = [storeTerrestrialRegion](TerrestrialLayer const& layer) -> Json {
    return JsonObject{{"layerMinHeight", layer.layerMinHeight},
        {"layerBaseHeight", layer.layerBaseHeight},
        {"dungeons", jsonFromStringList(layer.dungeons)},
        {"dungeonXVariance", layer.dungeonXVariance},
        {"primaryRegion", storeTerrestrialRegion(layer.primaryRegion)},
        {"primarySubRegion", storeTerrestrialRegion(layer.primarySubRegion)},
        {"secondaryRegions", layer.secondaryRegions.transformed(storeTerrestrialRegion)},
        {"secondarySubRegions", layer.secondarySubRegions.transformed(storeTerrestrialRegion)},
        {"secondaryRegionSizeRange", jsonFromVec2F(layer.secondaryRegionSizeRange)},
        {"subRegionSizeRange", jsonFromVec2F(layer.subRegionSizeRange)}};
  };

  return VisitableWorldParameters::store().setAll(JsonObject{{"primaryBiome", primaryBiome},
      {"sizeName", sizeName},
      {"hueShift", hueShift},
      {"surfaceLiquid", primarySurfaceLiquid},
      {"skyColoring", skyColoring.toJson()},
      {"dayLength", dayLength},
      {"blockNoise", blockNoiseConfig},
      {"blendNoise", blendNoiseConfig},
      {"blendSize", blendSize},
      {"spaceLayer", storeTerrestrialLayer(spaceLayer)},
      {"atmosphereLayer", storeTerrestrialLayer(atmosphereLayer)},
      {"surfaceLayer", storeTerrestrialLayer(surfaceLayer)},
      {"subsurfaceLayer", storeTerrestrialLayer(subsurfaceLayer)},
      {"undergroundLayers", undergroundLayers.transformed(storeTerrestrialLayer)},
      {"coreLayer", storeTerrestrialLayer(coreLayer)}});
}

DataStream& operator>>(DataStream& ds, TerrestrialWorldParameters::TerrestrialRegion& region) {
  ds >> region.biome;
  ds >> region.blockSelector;
  ds >> region.fgCaveSelector;
  ds >> region.bgCaveSelector;
  ds >> region.fgOreSelector;
  ds >> region.bgOreSelector;
  ds >> region.subBlockSelector;
  ds >> region.caveLiquid;
  ds >> region.caveLiquidSeedDensity;
  ds >> region.oceanLiquid;
  ds >> region.oceanLiquidLevel;
  ds >> region.encloseLiquids;
  ds >> region.fillMicrodungeons;
  return ds;
}

void TerrestrialWorldParameters::read(DataStream& ds) {
  auto readTerrestrialLayer = [](DataStream& ds, TerrestrialLayer& layer) {
    ds >> layer.layerMinHeight;
    ds >> layer.layerBaseHeight;
    ds >> layer.dungeons;
    ds >> layer.dungeonXVariance;
    ds >> layer.primaryRegion;
    ds >> layer.primarySubRegion;
    ds >> layer.secondaryRegions;
    ds >> layer.secondarySubRegions;
    ds >> layer.secondaryRegionSizeRange;
    ds >> layer.subRegionSizeRange;
  };

  VisitableWorldParameters::read(ds);
  ds >> primaryBiome;
  ds >> primarySurfaceLiquid;
  ds >> sizeName;
  ds >> hueShift;
  ds >> skyColoring;
  ds >> dayLength;
  ds >> blendSize;
  ds >> blockNoiseConfig;
  ds >> blendNoiseConfig;
  readTerrestrialLayer(ds, spaceLayer);
  readTerrestrialLayer(ds, atmosphereLayer);
  readTerrestrialLayer(ds, surfaceLayer);
  readTerrestrialLayer(ds, subsurfaceLayer);
  ds.readContainer(undergroundLayers, readTerrestrialLayer);
  readTerrestrialLayer(ds, coreLayer);
}

DataStream& operator<<(DataStream& ds, TerrestrialWorldParameters::TerrestrialRegion const& region) {
  ds << region.biome;
  ds << region.blockSelector;
  ds << region.fgCaveSelector;
  ds << region.bgCaveSelector;
  ds << region.fgOreSelector;
  ds << region.bgOreSelector;
  ds << region.subBlockSelector;
  ds << region.caveLiquid;
  ds << region.caveLiquidSeedDensity;
  ds << region.oceanLiquid;
  ds << region.oceanLiquidLevel;
  ds << region.encloseLiquids;
  ds << region.fillMicrodungeons;
  return ds;
}

void TerrestrialWorldParameters::write(DataStream& ds) const {
  auto writeTerrestrialLayer = [](DataStream& ds, TerrestrialLayer const& layer) {
    ds << layer.layerMinHeight;
    ds << layer.layerBaseHeight;
    ds << layer.dungeons;
    ds << layer.dungeonXVariance;
    ds << layer.primaryRegion;
    ds << layer.primarySubRegion;
    ds << layer.secondaryRegions;
    ds << layer.secondarySubRegions;
    ds << layer.secondaryRegionSizeRange;
    ds << layer.subRegionSizeRange;
  };

  VisitableWorldParameters::write(ds);
  ds << primaryBiome;
  ds << primarySurfaceLiquid;
  ds << sizeName;
  ds << hueShift;
  ds << skyColoring;
  ds << dayLength;
  ds << blendSize;
  ds << blockNoiseConfig;
  ds << blendNoiseConfig;
  writeTerrestrialLayer(ds, spaceLayer);
  writeTerrestrialLayer(ds, atmosphereLayer);
  writeTerrestrialLayer(ds, surfaceLayer);
  writeTerrestrialLayer(ds, subsurfaceLayer);
  ds.writeContainer(undergroundLayers, writeTerrestrialLayer);
  writeTerrestrialLayer(ds, coreLayer);
}

AsteroidsWorldParameters::AsteroidsWorldParameters() {
  airless = true;
}

AsteroidsWorldParameters::AsteroidsWorldParameters(Json const& store) : VisitableWorldParameters(store) {
  asteroidTopLevel = static_cast<int>(store.getInt("asteroidTopLevel"));
  asteroidBottomLevel = static_cast<int>(store.getInt("asteroidBottomLevel"));
  blendSize = store.getFloat("blendSize");
  asteroidBiome = store.getString("asteroidBiome");
  ambientLightLevel = jsonToColor(store.get("ambientLightLevel"));
}

WorldParametersType AsteroidsWorldParameters::type() const {
  return WorldParametersType::AsteroidsWorldParameters;
}

Json AsteroidsWorldParameters::store() const {
  return VisitableWorldParameters::store().setAll(JsonObject{{"asteroidTopLevel", asteroidTopLevel},
      {"asteroidBottomLevel", asteroidBottomLevel},
      {"blendSize", blendSize},
      {"asteroidBiome", asteroidBiome},
      {"ambientLightLevel", jsonFromColor(ambientLightLevel)}});
}

void AsteroidsWorldParameters::read(DataStream& ds) {
  VisitableWorldParameters::read(ds);
  ds >> asteroidTopLevel;
  ds >> asteroidBottomLevel;
  ds >> blendSize;
  ds >> asteroidBiome;
  ds >> ambientLightLevel;
}

void AsteroidsWorldParameters::write(DataStream& ds) const {
  VisitableWorldParameters::write(ds);
  ds << asteroidTopLevel;
  ds << asteroidBottomLevel;
  ds << blendSize;
  ds << asteroidBiome;
  ds << ambientLightLevel;
}

FloatingDungeonWorldParameters::FloatingDungeonWorldParameters(Json const& store) : VisitableWorldParameters(store) {
  dungeonBaseHeight = static_cast<int>(store.getInt("dungeonBaseHeight"));
  dungeonSurfaceHeight = static_cast<int>(store.getInt("dungeonSurfaceHeight"));
  dungeonUndergroundLevel = static_cast<int>(store.getInt("dungeonUndergroundLevel"));
  primaryDungeon = store.getString("primaryDungeon");
  biome = store.optString("biome");
  ambientLightLevel = jsonToColor(store.get("ambientLightLevel"));
  dayMusicTrack = store.optString("dayMusicTrack");
  nightMusicTrack = store.optString("nightMusicTrack");
  dayAmbientNoises = store.optString("dayAmbientNoises");
  nightAmbientNoises = store.optString("nightAmbientNoises");
}

WorldParametersType FloatingDungeonWorldParameters::type() const {
  return WorldParametersType::FloatingDungeonWorldParameters;
}

Json FloatingDungeonWorldParameters::store() const {
  return VisitableWorldParameters::store().setAll(JsonObject{{"dungeonBaseHeight", dungeonBaseHeight},
      {"dungeonSurfaceHeight", dungeonSurfaceHeight},
      {"dungeonUndergroundLevel", dungeonUndergroundLevel},
      {"primaryDungeon", primaryDungeon},
      {"biome", jsonFromMaybe(biome)},
      {"ambientLightLevel", jsonFromColor(ambientLightLevel)},
      {"dayMusicTrack", jsonFromMaybe(dayMusicTrack)},
      {"nightMusicTrack", jsonFromMaybe(nightMusicTrack)},
      {"dayAmbientNoises", jsonFromMaybe(dayAmbientNoises)},
      {"nightAmbientNoises", jsonFromMaybe(nightAmbientNoises)}});
}

void FloatingDungeonWorldParameters::read(DataStream& ds) {
  VisitableWorldParameters::read(ds);
  ds >> dungeonBaseHeight;
  ds >> dungeonSurfaceHeight;
  ds >> dungeonUndergroundLevel;
  ds >> primaryDungeon;
  ds >> biome;
  ds >> ambientLightLevel;
  ds >> dayMusicTrack;
  ds >> nightMusicTrack;
  ds >> dayAmbientNoises;
  ds >> nightAmbientNoises;
}

void FloatingDungeonWorldParameters::write(DataStream& ds) const {
  VisitableWorldParameters::write(ds);
  ds << dungeonBaseHeight;
  ds << dungeonSurfaceHeight;
  ds << dungeonUndergroundLevel;
  ds << primaryDungeon;
  ds << biome;
  ds << ambientLightLevel;
  ds << dayMusicTrack;
  ds << nightMusicTrack;
  ds << dayAmbientNoises;
  ds << nightAmbientNoises;
}

Json diskStoreVisitableWorldParameters(VisitableWorldParametersConstPtr const& parameters) {
  if (!parameters)
    return {};

  return parameters->store().setAll({{"type", WorldParametersTypeNames.getRight(parameters->type())}});
}

VisitableWorldParametersPtr diskLoadVisitableWorldParameters(Json const& store) {
  if (store.isNull())
    return {};

  auto type = WorldParametersTypeNames.getLeft(store.getString("type"));
  if (type == WorldParametersType::TerrestrialWorldParameters)
    return make_shared<TerrestrialWorldParameters>(store);
  else if (type == WorldParametersType::AsteroidsWorldParameters)
    return make_shared<AsteroidsWorldParameters>(store);
  else if (type == WorldParametersType::FloatingDungeonWorldParameters)
    return make_shared<FloatingDungeonWorldParameters>(store);
  throw StarException("No such WorldParametersType");
}

ByteArray netStoreVisitableWorldParameters(VisitableWorldParametersConstPtr const& parameters) {
  if (!parameters)
    return {};

  DataStreamBuffer ds;
  ds.write(parameters->type());
  parameters->write(ds);
  return ds.takeData();
}

VisitableWorldParametersPtr netLoadVisitableWorldParameters(ByteArray data) {
  if (data.empty())
    return {};

  DataStreamBuffer ds(std::move(data));
  auto type = ds.read<WorldParametersType>();

  VisitableWorldParametersPtr parameters;
  if (type == WorldParametersType::TerrestrialWorldParameters)
    parameters = make_shared<TerrestrialWorldParameters>();
  else if (type == WorldParametersType::AsteroidsWorldParameters)
    parameters = make_shared<AsteroidsWorldParameters>();
  else if (type == WorldParametersType::FloatingDungeonWorldParameters)
    parameters = make_shared<FloatingDungeonWorldParameters>();
  else
    throw StarException("No such WorldParametersType");

  parameters->read(ds);

  return parameters;
}

TerrestrialWorldParametersPtr generateTerrestrialWorldParameters(String const& typeName, String const& sizeName, uint64_t seed) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto liquidsDatabase = root.liquidsDatabase();
  auto biomeDatabase = root.biomeDatabase();

  auto terrestrialConfig = assets->json("/terrestrial_worlds.config");

  auto regionDefaults = terrestrialConfig.get("regionDefaults");
  auto regionTypes = terrestrialConfig.get("regionTypes");

  auto baseConfig = terrestrialConfig.get("planetDefaults");
  auto sizeConfig = terrestrialConfig.get("planetSizes").get(sizeName);
  auto typeConfig = terrestrialConfig.get("planetTypes").get(typeName);
  auto config = jsonMerge(baseConfig, sizeConfig, typeConfig);

  auto gravityRange = jsonToVec2F(config.get("gravityRange"));
  auto dayLengthRange = jsonToVec2F(config.get("dayLengthRange"));
  auto threatLevelRange = jsonToVec2F(config.query("threatRange"));

  auto threatLevel = static_cast<float>(staticRandomDouble(seed, "ThreatLevel") * (threatLevelRange[1] - threatLevelRange[0]) + threatLevelRange[0]);
  auto surfaceBiomeSeed = staticRandomU64(seed, "SurfaceBiomeSeed");

  auto readRegion = [liquidsDatabase, threatLevel, seed](Json const& regionConfig, String const& layerName, int layerBaseHeight) {
    TerrestrialWorldParameters::TerrestrialRegion region;
    auto biomeChoices = jsonToStringList(binnedChoiceFromJson(regionConfig.get("biome"), threatLevel));
    region.biome = staticRandomValueFrom(biomeChoices, seed, layerName.utf8Ptr());

    region.blockSelector = staticRandomFrom(regionConfig.getArray("blockSelector"), seed, "blockSelector", layerName.utf8Ptr()).toString();
    region.fgCaveSelector = staticRandomFrom(regionConfig.getArray("fgCaveSelector"), seed, "fgCaveSelector", layerName.utf8Ptr()).toString();
    region.bgCaveSelector = staticRandomFrom(regionConfig.getArray("bgCaveSelector"), seed, "bgCaveSelector", layerName.utf8Ptr()).toString();
    region.fgOreSelector = staticRandomFrom(regionConfig.getArray("fgOreSelector"), seed, "fgOreSelector", layerName.utf8Ptr()).toString();
    region.bgOreSelector = staticRandomFrom(regionConfig.getArray("bgOreSelector"), seed, "bgOreSelector", layerName.utf8Ptr()).toString();
    region.subBlockSelector = staticRandomFrom(regionConfig.getArray("subBlockSelector"), seed, "subBlockSelector", layerName.utf8Ptr()).toString();

    if (auto caveLiquid = staticRandomValueFrom(regionConfig.getArray("caveLiquid", {}), seed, "caveLiquid").optString()) {
      auto caveLiquidSeedDensityRange = jsonToVec2F(regionConfig.get("caveLiquidSeedDensityRange"));
      region.caveLiquid = liquidsDatabase->liquidId(*caveLiquid);
      region.caveLiquidSeedDensity = staticRandomFloatRange(caveLiquidSeedDensityRange[0],
          caveLiquidSeedDensityRange[1],
          seed,
          "caveLiquidSeedDensity",
          layerName.utf8Ptr());
    } else {
      region.caveLiquid = EmptyLiquidId;
      region.caveLiquidSeedDensity = 0.0f;
    }

    if (auto oceanLiquid = staticRandomValueFrom(regionConfig.getArray("oceanLiquid", {}), seed, "oceanLiquid", layerName.utf8Ptr()).optString()) {
      region.oceanLiquid = liquidsDatabase->liquidId(*oceanLiquid);
      region.oceanLiquidLevel = static_cast<int>(regionConfig.getInt("oceanLevelOffset", 0) + layerBaseHeight);
    } else {
      region.oceanLiquid = EmptyLiquidId;
      region.oceanLiquidLevel = 0;
    }
    region.encloseLiquids = regionConfig.getBool("encloseLiquids", false);
    region.fillMicrodungeons = regionConfig.getBool("fillMicrodungeons", false);

    return region;
  };

  auto readLayer = [readRegion, regionDefaults, regionTypes, seed, config](String const& layerName) -> Maybe<TerrestrialWorldParameters::TerrestrialLayer> {
    if (!config.get("layers").contains(layerName))
      return {};

    auto layerConfig = jsonMerge(config.get("layerDefaults"), config.get("layers").get(layerName));

    if (!layerConfig || !layerConfig.getBool("enabled"))
      return {};

    TerrestrialWorldParameters::TerrestrialLayer layer;

    layer.layerMinHeight = static_cast<int>(layerConfig.getFloat("layerLevel"));
    layer.layerBaseHeight = static_cast<int>(layerConfig.getFloat("baseHeight"));

    auto primaryRegionList = layerConfig.getArray("primaryRegion");
    auto primaryRegionConfigName = staticRandomFrom(primaryRegionList, seed, layerName.utf8Ptr(), "PrimaryRegionSelection").toString();
    Json primaryRegionConfig = jsonMerge(regionDefaults, regionTypes.get(primaryRegionConfigName));
    layer.primaryRegion = readRegion(primaryRegionConfig, layerName, layer.layerBaseHeight);

    {
      auto subRegionList = primaryRegionConfig.getArray("subRegion");
      Json subRegionConfig;
      if (!subRegionList.empty()) {
        String subRegionName = staticRandomFrom(subRegionList, seed, layerName, primaryRegionConfigName).toString();
        subRegionConfig = jsonMerge(regionDefaults, regionTypes.get(subRegionName));
      } else {
        subRegionConfig = primaryRegionConfig;
      }
      layer.primarySubRegion = readRegion(subRegionConfig, layerName, layer.layerBaseHeight);
    }

    Vec2U secondaryRegionCountRange = jsonToVec2U(layerConfig.get("secondaryRegionCount"));
    int secondaryRegionCount = staticRandomI32Range(static_cast<int>(secondaryRegionCountRange[0]), static_cast<int>(secondaryRegionCountRange[1]), seed, layerName, "SecondaryRegionCount");
    auto secondaryRegionList = layerConfig.getArray("secondaryRegions");
    if (!secondaryRegionList.empty()) {
      staticRandomShuffle(secondaryRegionList, seed, layerName, "SecondaryRegionShuffle");
      for (const auto& regionName : secondaryRegionList) {
        if (secondaryRegionCount <= 0)
          break;
        Json secondaryRegionConfig = jsonMerge(regionDefaults, regionTypes.get(regionName.toString()));
        layer.secondaryRegions.append(readRegion(secondaryRegionConfig, layerName, layer.layerBaseHeight));

        auto subRegionList = secondaryRegionConfig.getArray("subRegion");
        Json subRegionConfig;
        if (!subRegionList.empty()) {
          String subRegionName = staticRandomFrom(subRegionList, seed, layerName, regionName.toString()).toString();
          subRegionConfig = jsonMerge(regionDefaults, regionTypes.get(subRegionName));
        } else {
          subRegionConfig = secondaryRegionConfig;
        }
        layer.secondarySubRegions.append(readRegion(subRegionConfig, layerName, layer.layerBaseHeight));

        --secondaryRegionCount;
      }
    }

    layer.secondaryRegionSizeRange = jsonToVec2F(layerConfig.get("secondaryRegionSize"));
    layer.subRegionSizeRange = jsonToVec2F(layerConfig.get("subRegionSize"));

    WeightedPool<String> dungeonPool = jsonToWeightedPool<String>(layerConfig.get("dungeons"));
    Vec2U dungeonCountRange = layerConfig.opt("dungeonCountRange").apply(jsonToVec2U).value();
    unsigned dungeonCount = staticRandomU32Range(dungeonCountRange[0], dungeonCountRange[1], seed, layerName, "DungeonCount");
    layer.dungeons.appendAll(dungeonPool.selectUniques(dungeonCount, staticRandomHash64(seed, layerName, "DungeonChoice")));
    layer.dungeonXVariance = static_cast<int>(layerConfig.getInt("dungeonXVariance", 0));

    return layer;
  };

  auto surfaceLayer = readLayer("surface").take();
  String primaryBiome = surfaceLayer.primaryRegion.biome;

  auto parameters = make_shared<TerrestrialWorldParameters>();

  parameters->threatLevel = threatLevel;
  parameters->typeName = typeName;
  parameters->worldSize = jsonToVec2U(config.get("size"));
  parameters->gravity = staticRandomFloatRange(gravityRange[0], gravityRange[1], seed, "WorldGravity");
  parameters->airless = biomeDatabase->biomeIsAirless(primaryBiome);
  parameters->environmentStatusEffects = biomeDatabase->biomeStatusEffects(primaryBiome);
  parameters->overrideTech = config.opt("overrideTech").apply(jsonToStringList);
  parameters->globalDirectives = config.opt("globalDirectives").apply(jsonToDirectivesList);
  parameters->beamUpRule = BeamUpRuleNames.getLeft(config.getString("beamUpRule", "Surface"));
  parameters->disableDeathDrops = config.getBool("disableDeathDrops", false);
  parameters->worldEdgeForceRegions = WorldEdgeForceRegionTypeNames.getLeft(config.getString("worldEdgeForceRegions", "Top"));

  parameters->weatherPool = biomeDatabase->biomeWeathers(primaryBiome, seed, threatLevel);

  parameters->primaryBiome = primaryBiome;
  parameters->sizeName = sizeName;
  parameters->hueShift = biomeDatabase->biomeHueShift(parameters->primaryBiome, surfaceBiomeSeed);

  parameters->primarySurfaceLiquid = surfaceLayer.primaryRegion.oceanLiquid != EmptyLiquidId
      ? surfaceLayer.primaryRegion.oceanLiquid
      : surfaceLayer.primaryRegion.caveLiquid;

  parameters->skyColoring = biomeDatabase->biomeSkyColoring(parameters->primaryBiome, seed);
  parameters->dayLength = staticRandomFloatRange(dayLengthRange[0], dayLengthRange[1], seed, "DayLength");

  parameters->blockNoiseConfig = config.get("blockNoise");
  parameters->blendNoiseConfig = config.get("blendNoise");
  parameters->blendSize = config.getFloat("blendSize");

  parameters->spaceLayer = readLayer("space").take();
  parameters->atmosphereLayer = readLayer("atmosphere").take();
  parameters->surfaceLayer = surfaceLayer;
  parameters->subsurfaceLayer = readLayer("subsurface").take();

  while (auto undergroundLayer = readLayer(strf("underground{}", parameters->undergroundLayers.size() + 1)))
    parameters->undergroundLayers.append(undergroundLayer.take());

  parameters->coreLayer = readLayer("core").take();

  return parameters;
}

AsteroidsWorldParametersPtr generateAsteroidsWorldParameters(uint64_t seed) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  auto parameters = make_shared<AsteroidsWorldParameters>();

  auto asteroidsConfig = assets->json("/asteroids_worlds.config");
  String biome = asteroidsConfig.getString("biome");
  auto gravityRange = jsonToVec2F(asteroidsConfig.get("gravityRange"));

  auto threatLevelRange = jsonToVec2F(asteroidsConfig.get("threatRange"));
  parameters->threatLevel = static_cast<float>(staticRandomDouble(seed, "ThreatLevel") * (threatLevelRange[1] - threatLevelRange[0]) + threatLevelRange[0]);
  parameters->typeName = "asteroids";
  parameters->worldSize = jsonToVec2U(asteroidsConfig.get("worldSize"));
  parameters->gravity = staticRandomFloatRange(gravityRange[0], gravityRange[1], seed, "WorldGravity");
  parameters->environmentStatusEffects = jsonToStringList(asteroidsConfig.getArray("environmentStatusEffects", JsonArray()));
  parameters->overrideTech = asteroidsConfig.opt("overrideTech").apply(jsonToStringList);
  parameters->globalDirectives = asteroidsConfig.opt("globalDirectives").apply(jsonToDirectivesList);
  parameters->beamUpRule = BeamUpRuleNames.getLeft(asteroidsConfig.getString("beamUpRule", "Surface"));
  parameters->disableDeathDrops = asteroidsConfig.getBool("disableDeathDrops", false);
  parameters->worldEdgeForceRegions = WorldEdgeForceRegionTypeNames.getLeft(asteroidsConfig.getString("worldEdgeForceRegions", "TopAndBottom"));

  parameters->asteroidTopLevel = static_cast<int>(asteroidsConfig.getInt("asteroidsTop"));
  parameters->asteroidBottomLevel = static_cast<int>(asteroidsConfig.getInt("asteroidsBottom"));
  parameters->blendSize = asteroidsConfig.getFloat("blendSize");
  parameters->asteroidBiome = biome;
  parameters->ambientLightLevel = jsonToColor(asteroidsConfig.get("ambientLightLevel"));

  return parameters;
}

FloatingDungeonWorldParametersPtr generateFloatingDungeonWorldParameters(String const& dungeonWorldName) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  auto worldConfig = assets->json("/dungeon_worlds.config:" + dungeonWorldName);

  auto parameters = make_shared<FloatingDungeonWorldParameters>();

  parameters->threatLevel = worldConfig.getFloat("threatLevel", 0);
  parameters->typeName = dungeonWorldName;
  parameters->worldSize = jsonToVec2U(worldConfig.get("worldSize"));
  parameters->gravity = worldConfig.getFloat("gravity");
  parameters->airless = worldConfig.getBool("airless", false);
  parameters->environmentStatusEffects = jsonToStringList(worldConfig.getArray("environmentStatusEffects", JsonArray()));
  parameters->overrideTech = worldConfig.opt("overrideTech").apply(jsonToStringList);
  parameters->globalDirectives = worldConfig.opt("globalDirectives").apply(jsonToDirectivesList);
  if (auto weatherPoolConfig = worldConfig.optArray("weatherPool"))
    parameters->weatherPool = jsonToWeightedPool<String>(*weatherPoolConfig);
  parameters->beamUpRule = BeamUpRuleNames.getLeft(worldConfig.getString("beamUpRule", "Surface"));
  parameters->disableDeathDrops = worldConfig.getBool("disableDeathDrops", false);
  parameters->worldEdgeForceRegions = WorldEdgeForceRegionTypeNames.getLeft(worldConfig.getString("worldEdgeForceRegions", "Top"));

  parameters->dungeonBaseHeight = static_cast<int>(worldConfig.getInt("dungeonBaseHeight"));
  parameters->dungeonSurfaceHeight = static_cast<int>(worldConfig.getInt("dungeonSurfaceHeight", parameters->dungeonBaseHeight));
  parameters->dungeonUndergroundLevel = static_cast<int>(worldConfig.getInt("dungeonUndergroundLevel", 0));
  parameters->primaryDungeon = worldConfig.getString("primaryDungeon");
  parameters->biome = worldConfig.optString("biome");
  parameters->ambientLightLevel = jsonToColor(worldConfig.get("ambientLightLevel"));
  if (worldConfig.contains("musicTrack")) {
    parameters->dayMusicTrack = worldConfig.optString("musicTrack");
    parameters->nightMusicTrack = worldConfig.optString("musicTrack");
  } else {
    parameters->dayMusicTrack = worldConfig.optString("dayMusicTrack");
    parameters->nightMusicTrack = worldConfig.optString("nightMusicTrack");
  }
  if (worldConfig.contains("ambientNoises")) {
    parameters->dayAmbientNoises = worldConfig.optString("ambientNoises");
    parameters->nightAmbientNoises = worldConfig.optString("ambientNoises");
  } else {
    parameters->dayAmbientNoises = worldConfig.optString("dayAmbientNoises");
    parameters->nightAmbientNoises = worldConfig.optString("nightAmbientNoises");
  }

  return parameters;
}

}
