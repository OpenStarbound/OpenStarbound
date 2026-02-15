#include "StarWorldTemplate.hpp"
#include "StarJsonExtra.hpp"
#include "StarInterpolation.hpp"
#include "StarIterator.hpp"
#include "StarBiome.hpp"
#include "StarRoot.hpp"
#include "StarTerrainDatabase.hpp"
#include "StarLiquidTypes.hpp"
#include "StarAssets.hpp"
#include "StarLogging.hpp"
#include "StarDungeonGenerator.hpp"

namespace Star {

WorldTemplate::WorldTemplate(Vec2U const& size) : WorldTemplate() {
  m_geometry = size;
}

WorldTemplate::WorldTemplate(CelestialCoordinate const& celestialCoordinate, CelestialDatabasePtr const& celestialDatabase)
  : WorldTemplate() {
  auto celestialParameters = celestialDatabase->parameters(celestialCoordinate);
  if (!celestialParameters)
    throw StarException("Celestial parameters for constructing WorldTemplate not found!");

  m_celestialParameters = celestialParameters;
  m_worldParameters = m_celestialParameters->visitableParameters();
  if (!m_worldParameters)
    throw StarException("Cannot create WorldTemplate from non-visitable world");

  m_skyParameters = SkyParameters(celestialCoordinate, celestialDatabase);
  m_seed = m_celestialParameters->seed();
  m_geometry = WorldGeometry(m_worldParameters->worldSize);

  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters))
    m_layout = make_shared<WorldLayout>(WorldLayout::buildTerrestrialLayout(*terrestrialParameters, m_seed));
  else if (auto asteroidsParameters = as<AsteroidsWorldParameters>(m_worldParameters))
    m_layout = make_shared<WorldLayout>(WorldLayout::buildAsteroidsLayout(*asteroidsParameters, m_seed));
  else if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(m_worldParameters))
    m_layout = make_shared<WorldLayout>(WorldLayout::buildFloatingDungeonLayout(*floatingDungeonParameters, m_seed));

  determineWorldName();
}

WorldTemplate::WorldTemplate(VisitableWorldParametersConstPtr const& worldParameters, SkyParameters const& skyParameters, uint64_t seed)
  : WorldTemplate() {
  if (!worldParameters)
    throw StarException("Cannot create WorldTemplate from non-visitable world");

  m_worldParameters = worldParameters;
  m_skyParameters = skyParameters;
  m_seed = seed;
  m_geometry = WorldGeometry(m_worldParameters->worldSize);

  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters))
    m_layout = make_shared<WorldLayout>(WorldLayout::buildTerrestrialLayout(*terrestrialParameters, seed));
  else if (auto asteroidsParameters = as<AsteroidsWorldParameters>(m_worldParameters))
    m_layout = make_shared<WorldLayout>(WorldLayout::buildAsteroidsLayout(*asteroidsParameters, seed));
  else if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(m_worldParameters))
    m_layout = make_shared<WorldLayout>(WorldLayout::buildFloatingDungeonLayout(*floatingDungeonParameters, m_seed));

  determineWorldName();
}

WorldTemplate::WorldTemplate(Json const& store) : WorldTemplate() {
  m_celestialParameters = jsonToMaybe<CelestialParameters>(store.get("celestialParameters", {}));
  m_worldParameters = diskLoadVisitableWorldParameters(store.get("worldParameters", {}));
  m_skyParameters = SkyParameters(store.get("skyParameters"));

  m_seed = store.getUInt("seed");
  m_geometry = WorldGeometry(jsonToVec2U(store.get("size")));
  if (auto regionData = store.opt("regionData"))
    m_layout = make_shared<WorldLayout>(regionData.take());

  m_customTerrainRegions = store.getArray("customTerrainRegions", JsonArray()).transformed([](Json const& config) {
      CustomTerrainRegion ctr = {jsonToPolyF(config.get("region")), {}, config.getBool("solid")};
      ctr.regionBounds = ctr.region.boundBox();
      return ctr;
    });

  determineWorldName();
}

Json WorldTemplate::store() const {
  return JsonObject{
    {"celestialParameters", jsonFromMaybe(m_celestialParameters, mem_fn(&CelestialParameters::diskStore))},
    {"worldParameters", diskStoreVisitableWorldParameters(m_worldParameters)},
    {"skyParameters", m_skyParameters.toJson()},
    {"seed", m_seed},
    {"size", jsonFromVec2U(m_geometry.size())},
    {"regionData", m_layout ? m_layout->toJson() : Json()},
    {"customTerrainRegions", transform<JsonArray>(m_customTerrainRegions, [](CustomTerrainRegion const& region) {
        return JsonObject{{"region", jsonFromPolyF(region.region)}, {"solid", region.solid}};
      })}
  };
}

Maybe<CelestialParameters> const& WorldTemplate::celestialParameters() const {
  return m_celestialParameters;
}

VisitableWorldParametersConstPtr WorldTemplate::worldParameters() const {
  return m_worldParameters;
}

SkyParameters WorldTemplate::skyParameters() const {
  return m_skyParameters;
}

WorldLayoutPtr WorldTemplate::worldLayout() const {
  return m_layout;
}

void WorldTemplate::setCelestialParameters(CelestialParameters newParameters){
  m_celestialParameters = take(newParameters);
}

void WorldTemplate::setWorldParameters(VisitableWorldParametersPtr newParameters) {
  m_worldParameters = take(newParameters);
}

void WorldTemplate::setWorldLayout(WorldLayoutPtr newLayout) {
  m_layout = take(newLayout);
  m_blockCache.clear();
}

void WorldTemplate::setSkyParameters(SkyParameters newParameters) {
  m_skyParameters = take(newParameters);
}

uint64_t WorldTemplate::worldSeed() const {
  return m_seed;
}

String WorldTemplate::worldName() const {
  return m_worldName;
}

Vec2U WorldTemplate::size() const {
  return m_geometry.size();
}

float WorldTemplate::surfaceLevel() const {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters))
    return terrestrialParameters->surfaceLayer.layerBaseHeight;
  return m_geometry.size()[1] / 2.0f;
}

float WorldTemplate::undergroundLevel() const {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters))
    return terrestrialParameters->surfaceLayer.layerMinHeight;
  else if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(m_worldParameters))
    return floatingDungeonParameters->dungeonUndergroundLevel;
  return 0.0f;
}

bool WorldTemplate::inSurfaceLayer(Vec2I const& position) const {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters)) {
    auto posLayerAndCell = m_layout->findLayerAndCell(position[0], position[1]);
    auto surfaceLayerAndCell = m_layout->findLayerAndCell(position[0], terrestrialParameters->surfaceLayer.layerBaseHeight);
    return posLayerAndCell.first == surfaceLayerAndCell.first;
  }
  return false;
}

Maybe<Vec2I> WorldTemplate::findSensiblePlayerStart() const {
  if (!m_layout)
    return {};

  auto playerStartSearchRegions = m_layout->playerStartSearchRegions();
  if (playerStartSearchRegions.empty())
    return {};

  int playerStartSearchTries = m_templateConfig.getInt("playerStartSearchTries");
  int playerStartFreeBlocksRadius = m_templateConfig.getInt("playerStartFreeBlocksRadius");
  int playerStartFreeBlocksHeight = m_templateConfig.getInt("playerStartFreeBlocksHeight");

  for (int i = 0; i < playerStartSearchTries; ++i) {
    RectI searchRegion = Random::randFrom(playerStartSearchRegions);
    int x = Random::randInt(searchRegion.xMin(), searchRegion.xMax());

    for (int y = searchRegion.yMax() - 1; y >= searchRegion.yMin(); --y) {
      if (getBlockInfo(x, y).terrain && !getBlockInfo(x, y + 1).terrain) {
        if (isOutside(RectI(x - playerStartFreeBlocksRadius, y + 1, x + playerStartFreeBlocksRadius, y + playerStartFreeBlocksHeight)))
          return Vec2I(x, y + 1);
      }
    }
  }

  return {};
}

void WorldTemplate::addCustomTerrainRegion(PolyF poly) {
  m_customTerrainRegions.append({poly, poly.boundBox(), true});
  m_blockCache.clear();
}

void WorldTemplate::addCustomSpaceRegion(PolyF poly) {
  m_customTerrainRegions.append({poly, poly.boundBox(), false});
  m_blockCache.clear();
}

void WorldTemplate::clearCustomTerrains() {
  m_customTerrainRegions.clear();
  m_blockCache.clear();
}

List<RectI> WorldTemplate::previewAddBiomeRegion(Vec2I const& position, int width) {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters)) {
    auto regionRects = m_layout->previewAddBiomeRegion(position, width);
    regionRects.transform([terrestrialParameters](RectI const& rect) {
        return rect.padded(ceil(terrestrialParameters->blendSize));
      });
    return regionRects;
  } else {
    Logger::error("Cannot add biome region to non-terrestrial world!");
    // throw StarException("Cannot add biome region to non-terrestrial world!");
    return List<RectI>();
  }
}

List<RectI> WorldTemplate::previewExpandBiomeRegion(Vec2I const& position, int newWidth) {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters)) {
    auto regionRects = m_layout->previewExpandBiomeRegion(position, newWidth);
    regionRects.transform([terrestrialParameters](RectI const& rect) {
        return rect.padded(ceil(terrestrialParameters->blendSize));
      });
    return regionRects;
  } else {
    Logger::error("Cannot expand biome region on non-terrestrial world!");
    // throw StarException("Cannot expand biome region on non-terrestrial world!");
    return List<RectI>();
  }
}

void WorldTemplate::addBiomeRegion(Vec2I const& position, String const& biomeName, String const& subBlockSelector, int width) {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters)) {
    m_layout->addBiomeRegion(*terrestrialParameters, m_seed, position, biomeName, subBlockSelector, width);
    m_blockCache.clear();
  } else {
    Logger::error("Cannot add biome region to non-terrestrial world!");
    // throw StarException("Cannot add biome region to non-terrestrial world!");
  }
}

void WorldTemplate::expandBiomeRegion(Vec2I const& position, int newWidth) {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters)) {
    m_layout->expandBiomeRegion(position, newWidth);
    m_blockCache.clear();
  } else {
    Logger::error("Cannot expand biome region on non-terrestrial world!");
    // throw StarException("Cannot expand biome region on non-terrestrial world!");
  }
}

List<WorldTemplate::Dungeon> WorldTemplate::dungeons() const {
  List<Dungeon> dungeonList;

  if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(m_worldParameters)) {
    dungeonList.append({floatingDungeonParameters->primaryDungeon, floatingDungeonParameters->dungeonBaseHeight, 0, 0, true, false});
  } else if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldParameters)) {
    auto addLayerDungeons = [this, &dungeonList](TerrestrialWorldParameters::TerrestrialLayer const& layer) {
      if (layer.dungeons.size() > 0) {
        int dungeonSpacing = floor(m_geometry.width() / layer.dungeons.size());
        uint32_t dungeonOffset = staticRandomU32Range(0, m_geometry.width(), m_seed, layer.layerBaseHeight);
        for (auto const& dp : enumerateIterator(layer.dungeons)) {
          dungeonList.append({dp.first, layer.layerBaseHeight, (int)dungeonOffset, layer.dungeonXVariance, false, true});
          dungeonOffset = (dungeonOffset + dungeonSpacing) % m_geometry.width();
        }
      }
    };

    addLayerDungeons(terrestrialParameters->spaceLayer);
    addLayerDungeons(terrestrialParameters->atmosphereLayer);
    addLayerDungeons(terrestrialParameters->surfaceLayer);
    addLayerDungeons(terrestrialParameters->subsurfaceLayer);
    for (auto const& ul : terrestrialParameters->undergroundLayers)
      addLayerDungeons(ul);
    addLayerDungeons(terrestrialParameters->coreLayer);
  }

  return dungeonList;
}

WorldTemplate::BlockInfo WorldTemplate::blockInfo(int x, int y) const {
  return getBlockInfo(m_geometry.xwrap(x), y);
}

WorldTemplate::BlockInfo WorldTemplate::blockBiomeInfo(int x, int y) const {
  BlockInfo blockInfo;

  if (!m_layout)
    return blockInfo;

  // The environment biome is calculated with weighting based on the flat coordinates.
  List<WorldLayout::RegionWeighting> flatWeighting = m_layout->getWeighting(x, y);

  int blendNoiseOffset = 0;
  if (auto const& blendNoise = m_layout->blendNoise())
    blendNoiseOffset = (int)blendNoise->get(x, y);

  Vec2I blockPos;
  List<WorldLayout::RegionWeighting> blockWeighting;
  List<WorldLayout::RegionWeighting> transitionWeighting;
  if (auto const& blockNoise = m_layout->blockNoise()) {
    blockPos = blockNoise->apply(Vec2I(x, y), m_geometry.size());
    blockWeighting = m_layout->getWeighting(blockPos[0] + blendNoiseOffset, blockPos[1]);
    transitionWeighting = m_layout->getWeighting(blockPos[0], blockPos[1]);
  } else {
    blockPos = Vec2I(x, y);
    blockWeighting = flatWeighting;
    transitionWeighting = flatWeighting;
  }

  if (flatWeighting.empty() || blockWeighting.empty())
    return blockInfo;

  auto const& primaryFlatWeighting = flatWeighting.first();
  auto const& primaryBlockWeighting = blockWeighting.first();

  blockInfo.blockBiomeIndex = primaryBlockWeighting.region->blockBiomeIndex;
  blockInfo.environmentBiomeIndex = primaryFlatWeighting.region->environmentBiomeIndex;

  blockInfo.biomeTransition = transitionWeighting.first().weight < m_templateConfig.getFloat("biomeTransitionThreshold", 0);

  if (auto blockBiome = biome(blockInfo.blockBiomeIndex)) {
    if (!blockInfo.foregroundCave) {
      blockInfo.foreground = blockBiome->mainBlock;
      blockInfo.background = blockInfo.foreground;
    } else if (!blockInfo.backgroundCave) {
      blockInfo.background = blockBiome->mainBlock;
    }

    if (!primaryBlockWeighting.region->subBlockSelectorIndexes.empty()) {
      for (size_t i = 0; i < blockBiome->subBlocks.size(); ++i) {
        auto const& selector = m_layout->getTerrainSelector(primaryBlockWeighting.region->subBlockSelectorIndexes.at(i));
        if (selector->get(primaryBlockWeighting.xValue - blendNoiseOffset, blockPos[1]) > 0.0f) {
          if (!blockInfo.foregroundCave) {
            blockInfo.foreground = blockBiome->subBlocks.at(i);
            blockInfo.background = blockInfo.foreground;
          } else if (!blockInfo.backgroundCave) {
            blockInfo.background = blockBiome->subBlocks.at(i);
          }

          break;
        }
      }
    }
  }

  return blockInfo;
}

bool WorldTemplate::isOutside(int x, int y) const {
  return !getBlockInfo(m_geometry.xwrap(x), y).terrain;
}

bool WorldTemplate::isOutside(RectI const& region) const {
  for (int x = region.xMin(); x < region.xMax(); ++x) {
    for (int y = region.yMin(); y < region.yMax(); ++y) {
      if (getBlockInfo(m_geometry.xwrap(x), y).terrain)
        return false;
    }
  }
  return true;
}

BiomeIndex WorldTemplate::blockBiomeIndex(int x, int y) const {
  return getBlockInfo(m_geometry.xwrap(x), y).blockBiomeIndex;
}

BiomeIndex WorldTemplate::environmentBiomeIndex(int x, int y) const {
  return getBlockInfo(m_geometry.xwrap(x), y).environmentBiomeIndex;
}

BiomeConstPtr WorldTemplate::biome(BiomeIndex biomeIndex) const {
  if (!m_layout)
    return {};
  if (biomeIndex == NullBiomeIndex)
    return {};
  return m_layout->getBiome(biomeIndex);
}

BiomeConstPtr WorldTemplate::blockBiome(int x, int y) const {
  return biome(blockBiomeIndex(m_geometry.xwrap(x), y));
}

BiomeConstPtr WorldTemplate::environmentBiome(int x, int y) const {
  return biome(environmentBiomeIndex(m_geometry.xwrap(x), y));
}

MaterialHue WorldTemplate::biomeMaterialHueShift(BiomeIndex biomeIndex, MaterialId material) const {
  if (m_layout && biomeIndex != NullBiomeIndex) {
    auto const& biome = m_layout->getBiome(biomeIndex);
    if (material == biome->mainBlock)
      return biome->materialHueShift;
  }

  return MaterialHue();
}

MaterialHue WorldTemplate::biomeModHueShift(BiomeIndex biomeIndex, ModId mod) const {
  if (m_layout && biomeIndex != NullBiomeIndex) {
    auto const& biome = m_layout->getBiome(biomeIndex);
    if (mod == biome->surfacePlaceables.grassMod || mod == biome->surfacePlaceables.ceilingGrassMod
        || mod == biome->undergroundPlaceables.grassMod
        || mod == biome->undergroundPlaceables.ceilingGrassMod)
      return biome->materialHueShift;
  }
  return MaterialHue();
}

WeatherPool WorldTemplate::weathers() const {
  if (m_worldParameters)
    return m_worldParameters->weatherPool;
  return {};
}

AmbientNoisesDescriptionPtr WorldTemplate::ambientNoises(int x, int y) const {
  if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(m_worldParameters)) {
    if (floatingDungeonParameters->dayAmbientNoises || floatingDungeonParameters->nightAmbientNoises) {
      auto dayTracks = floatingDungeonParameters->dayAmbientNoises
          ? AmbientTrackGroup(StringList{*floatingDungeonParameters->dayAmbientNoises})
          : AmbientTrackGroup();
      auto nightTracks = floatingDungeonParameters->nightAmbientNoises
          ? AmbientTrackGroup(StringList{*floatingDungeonParameters->nightAmbientNoises})
          : AmbientTrackGroup();
      return make_shared<AmbientNoisesDescription>(dayTracks, nightTracks);
    }
  }
  if (auto biome = environmentBiome(x, y))
    return biome->ambientNoises;
  return {};
}

AmbientNoisesDescriptionPtr WorldTemplate::musicTrack(int x, int y) const {
  if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(m_worldParameters)) {
    if (floatingDungeonParameters->dayMusicTrack || floatingDungeonParameters->nightMusicTrack) {
      auto dayTracks = floatingDungeonParameters->dayMusicTrack
          ? AmbientTrackGroup(StringList{*floatingDungeonParameters->dayMusicTrack})
          : AmbientTrackGroup();
      auto nightTracks = floatingDungeonParameters->nightMusicTrack
          ? AmbientTrackGroup(StringList{*floatingDungeonParameters->nightMusicTrack})
          : AmbientTrackGroup();
      return make_shared<AmbientNoisesDescription>(dayTracks, nightTracks);
    }
  }
  if (auto biome = environmentBiome(x, y))
    return biome->musicTrack;
  return {};
}

StringList WorldTemplate::environmentStatusEffects(int, int) const {
  if (m_worldParameters)
    return m_worldParameters->environmentStatusEffects;
  return {};
}

bool WorldTemplate::breathable(int, int) const {
  if (m_worldParameters)
    return !m_worldParameters->airless;
  return true;
}

void WorldTemplate::addPotentialBiomeItems(int x, int y, PotentialBiomeItems& items, List<BiomeItemDistribution> const& distributions, BiomePlacementArea area, Maybe<BiomePlacementMode> mode) const {
  for (auto const& itemDistribution : distributions) {
    auto placeMode = mode.value(itemDistribution.mode());

    if (area == BiomePlacementArea::Surface) {
      if (placeMode == itemDistribution.mode() && placeMode == BiomePlacementMode::Floor) {
        if (auto itemToPlace = itemDistribution.itemToPlace(x, y))
          items.surfaceBiomeItems.append(itemToPlace.take());
      }
      if (placeMode == itemDistribution.mode() && placeMode == BiomePlacementMode::Ocean) {
        if (auto itemToPlace = itemDistribution.itemToPlace(x, y))
          items.oceanItems.append(itemToPlace.take());
      }
    } else if (area == BiomePlacementArea::Underground) {
      if (placeMode == itemDistribution.mode() && placeMode == BiomePlacementMode::Floor) {
        if (auto itemToPlace = itemDistribution.itemToPlace(x, y))
          items.caveSurfaceBiomeItems.append(itemToPlace.take());
      }
      if (placeMode == itemDistribution.mode() && placeMode == BiomePlacementMode::Ceiling) {
        if (auto itemToPlace = itemDistribution.itemToPlace(x, y))
          items.caveCeilingBiomeItems.append(itemToPlace.take());
      }
      if (placeMode == itemDistribution.mode() && placeMode == BiomePlacementMode::Background) {
        if (auto itemToPlace = itemDistribution.itemToPlace(x, y))
          items.caveBackgroundBiomeItems.append(itemToPlace.take());
      }
    }
  }
}


WorldTemplate::PotentialBiomeItems WorldTemplate::potentialBiomeItemsAt(int x, int y) const {
  if (!m_layout || y <= 0 || y >= (int)m_geometry.height() - 1)
    return {};
  x = m_geometry.xwrap(x);

  auto blockBiome = [this](int x, int y) -> Biome const* {
    BiomeIndex index = blockBiomeIndex(m_geometry.xwrap(x), y);
    if (index == NullBiomeIndex)
      return nullptr;
    return m_layout->getBiome(index).get();
  };

  auto lowerBlockBiome = blockBiome(x, y - 1);
  auto upperBlockBiome = blockBiome(x, y + 1);
  auto thisBlockBiome = blockBiome(x, y);

  PotentialBiomeItems potentialBiomeItems;
  // surface floor, surface ocean
  if (lowerBlockBiome)
    addPotentialBiomeItems(x, y, potentialBiomeItems, lowerBlockBiome->surfacePlaceables.itemDistributions, BiomePlacementArea::Surface, BiomePlacementMode::Floor);
  if (thisBlockBiome)
    addPotentialBiomeItems(x, y, potentialBiomeItems, thisBlockBiome->surfacePlaceables.itemDistributions, BiomePlacementArea::Surface, BiomePlacementMode::Ocean);

  // underground floor, ceiling, background
  if (lowerBlockBiome)
    addPotentialBiomeItems(x, y, potentialBiomeItems, lowerBlockBiome->undergroundPlaceables.itemDistributions, BiomePlacementArea::Underground, BiomePlacementMode::Floor);
  if (upperBlockBiome)
    addPotentialBiomeItems(x, y, potentialBiomeItems, upperBlockBiome->undergroundPlaceables.itemDistributions, BiomePlacementArea::Underground, BiomePlacementMode::Ceiling);
  if (thisBlockBiome)
    addPotentialBiomeItems(x, y, potentialBiomeItems, thisBlockBiome->undergroundPlaceables.itemDistributions, BiomePlacementArea::Underground, BiomePlacementMode::Background);

  return potentialBiomeItems;
}

List<BiomeItemPlacement> WorldTemplate::validBiomeItems(int x, int y, PotentialBiomeItems potentialBiomeItems) const {
  if (y <= 0 || y >= (int)m_geometry.height() - 1)
    return {};

  x = m_geometry.xwrap(x);

  auto block = getBlockInfo(x, y);

  if (block.biomeTransition)
    return {};
  List<BiomeItemPlacement> biomeItems;

  auto blockAbove = getBlockInfo(x, y + 1);
  auto blockBelow = getBlockInfo(x, y - 1);

  if (!blockBelow.biomeTransition && blockBelow.terrain && !block.terrain && !blockBelow.foregroundCave)
    biomeItems.appendAll(std::move(potentialBiomeItems.surfaceBiomeItems));

  if (!blockBelow.biomeTransition && blockBelow.terrain && block.terrain && !blockBelow.foregroundCave && block.foregroundCave)
    biomeItems.appendAll(std::move(potentialBiomeItems.caveSurfaceBiomeItems));

  if (!blockAbove.biomeTransition && blockAbove.terrain && block.terrain && !blockAbove.foregroundCave && block.foregroundCave)
    biomeItems.appendAll(std::move(potentialBiomeItems.caveCeilingBiomeItems));

  if (block.terrain && block.foregroundCave && !block.backgroundCave)
    biomeItems.appendAll(std::move(potentialBiomeItems.caveBackgroundBiomeItems));

  if (block.oceanLiquid != EmptyLiquidId && y == block.oceanLiquidLevel)
    biomeItems.appendAll(std::move(potentialBiomeItems.oceanItems));

  return biomeItems;
}

float WorldTemplate::gravity() const {
  if (m_worldParameters)
    return m_worldParameters->gravity;
  return m_templateConfig.getFloat("defaultGravity");
}

float WorldTemplate::threatLevel() const {
  if (m_worldParameters)
    return m_worldParameters->threatLevel;
  return 0.0f;
}

uint64_t WorldTemplate::seedFor(int x, int y) const {
  return staticRandomU64(m_seed, m_geometry.xwrap(x), y, "Block");
}

WorldTemplate::WorldTemplate() {
  auto assets = Root::singleton().assets();
  m_templateConfig = Root::singleton().assets()->json("/world_template.config");
  m_customTerrainBlendSize = m_templateConfig.getFloat("customTerrainBlendSize");
  m_customTerrainBlendWeight = m_templateConfig.getFloat("customTerrainBlendWeight");

  m_blockCache.setMaxSize(m_templateConfig.getInt("blockCacheSize"));
  m_geometry = Vec2U(2048, 2048);
  m_seed = Random::randu64();
}

void WorldTemplate::determineWorldName() {
  if (m_celestialParameters)
    m_worldName = m_celestialParameters->name();
  else if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(m_worldParameters))
    m_worldName = Root::singleton().dungeonDefinitions()->get(floatingDungeonParameters->primaryDungeon)->displayName();
  else
    m_worldName = "";
}

pair<float, float> WorldTemplate::customTerrainWeighting(int x, int y) const {
  auto assets = Root::singleton().assets();

  float minimumDistance = highest<float>();
  float finalSolidWeight = 0.0f;
  float totalWeight = 0.0f;

  for (auto const& region : m_customTerrainRegions) {
    if (!m_geometry.rectContains(region.regionBounds.padded(m_customTerrainBlendSize), Vec2F(x, y)))
      continue;

    float distance = m_geometry.polyDistance(region.region, Vec2F(x, y));
    if (distance >= m_customTerrainBlendSize)
      continue;

    float weight = 1.0f - distance / m_customTerrainBlendSize;
    totalWeight += weight;
    if (!region.solid)
      weight *= -1.0f;

    finalSolidWeight += weight;
    minimumDistance = min(minimumDistance, distance);
  }

  if (minimumDistance > m_customTerrainBlendSize)
    return {0.0f, 0.0f};

  finalSolidWeight /= totalWeight;

  return {finalSolidWeight * m_customTerrainBlendWeight, 1.0f - minimumDistance / m_customTerrainBlendSize};
}

WorldTemplate::BlockInfo WorldTemplate::getBlockInfo(uint32_t x, uint32_t y) const {
  return m_blockCache.get(Vector<uint32_t, 2>(x, y), [this, x, y](Vector<uint32_t, 2>) {
      BlockInfo blockInfo;

      if (!m_layout)
        return blockInfo;

      // The environment biome is calculated with weighting based on the flat coordinates.
      List<WorldLayout::RegionWeighting> flatWeighting = m_layout->getWeighting(x, y);

      // The block biome is calculated optionally with higher frequency noise
      // added to prevent straight lines appearing on the boundaries of
      // regions.

      int blendNoiseOffset = 0;
      if (auto const& blendNoise = m_layout->blendNoise())
        blendNoiseOffset = (int)blendNoise->get(x, y);

      Vec2I blockPos;
      List<WorldLayout::RegionWeighting> blockWeighting;
      List<WorldLayout::RegionWeighting> transitionWeighting;
      if (auto const& blockNoise = m_layout->blockNoise()) {
        blockPos = blockNoise->apply(Vec2I(x, y), m_geometry.size());
        blockWeighting = m_layout->getWeighting(blockPos[0] + blendNoiseOffset, blockPos[1]);
        transitionWeighting = m_layout->getWeighting(blockPos[0], blockPos[1]);
      } else {
        blockPos = Vec2I(x, y);
        blockWeighting = flatWeighting;
        transitionWeighting = flatWeighting;
      }

      if (flatWeighting.empty() || blockWeighting.empty())
        return blockInfo;

      auto const& primaryFlatWeighting = flatWeighting.first();
      auto const& primaryBlockWeighting = blockWeighting.first();

      blockInfo.blockBiomeIndex = primaryBlockWeighting.region->blockBiomeIndex;
      blockInfo.environmentBiomeIndex = primaryFlatWeighting.region->environmentBiomeIndex;

      blockInfo.biomeTransition = transitionWeighting.first().weight < m_templateConfig.getFloat("biomeTransitionThreshold", 0);

      float terrainSelect = 0.0f;
      float foregroundCaveSelect = 0.0f;
      float backgroundCaveSelect = 0.0f;

      // Terrain weighting uses the flat weighting, and weights each selector
      // to blend among them.
      for (auto const& weighting : flatWeighting) {
        if (weighting.region->terrainSelectorIndex != NullTerrainSelectorIndex) {
          auto const& terrainSelector = m_layout->getTerrainSelector(weighting.region->terrainSelectorIndex);
          float select = terrainSelector->get(weighting.xValue, y) * weighting.weight;
          terrainSelect += select;
        }
      }

      // This is a bit of a cheat. Since customTerrainWeighting is always flat,
      // there are some odd effects that come from linearly interpolating from
      // the generally non-flat terrain sources to flat regions of space.  By
      // using an interpolator that has an exaggerated S curve between the
      // points, this hides some of these effects.
      auto ctweighting = customTerrainWeighting(x, y);
      terrainSelect = quintic2(ctweighting.second, terrainSelect, ctweighting.first);

      if (terrainSelect > 0.0f) {
        blockInfo.terrain = true;

        for (auto const& weighting : flatWeighting) {
          if (weighting.region->foregroundCaveSelectorIndex != NullTerrainSelectorIndex) {
            auto const& foregroundCaveSelector = m_layout->getTerrainSelector(weighting.region->foregroundCaveSelectorIndex);
            foregroundCaveSelect += foregroundCaveSelector->get(weighting.xValue, y) * weighting.weight;
          }

          if (weighting.region->backgroundCaveSelectorIndex != NullTerrainSelectorIndex) {
            auto const& backgroundCaveSelector = m_layout->getTerrainSelector(weighting.region->backgroundCaveSelectorIndex);
            backgroundCaveSelect += backgroundCaveSelector->get(weighting.xValue, y) * weighting.weight;
          }
        }

        auto surfaceCaveAttenuationDist = m_templateConfig.getFloat("surfaceCaveAttenuationDist", 0);
        if (terrainSelect < surfaceCaveAttenuationDist) {
          auto surfaceCaveAttenuationFactor = m_templateConfig.getFloat("surfaceCaveAttenuationFactor", 1);
          foregroundCaveSelect -= (surfaceCaveAttenuationDist - terrainSelect) * surfaceCaveAttenuationFactor;
          backgroundCaveSelect -= (surfaceCaveAttenuationDist - terrainSelect) * surfaceCaveAttenuationFactor;
        }
      }

      blockInfo.foregroundCave = foregroundCaveSelect > 0.0f;
      blockInfo.backgroundCave = backgroundCaveSelect > 0.0f;

      auto const& regionLiquids = primaryFlatWeighting.region->regionLiquids;
      blockInfo.caveLiquid = regionLiquids.caveLiquid;
      blockInfo.caveLiquidSeedDensity = regionLiquids.caveLiquidSeedDensity;
      blockInfo.oceanLiquid = regionLiquids.oceanLiquid;
      blockInfo.oceanLiquidLevel = regionLiquids.oceanLiquidLevel;
      blockInfo.encloseLiquids = regionLiquids.encloseLiquids;
      blockInfo.fillMicrodungeons = regionLiquids.fillMicrodungeons;

      if (!blockInfo.terrain && blockInfo.encloseLiquids && (int)y < blockInfo.oceanLiquidLevel) {
        blockInfo.terrain = true;
        blockInfo.foregroundCave = true;
      }

      if (blockInfo.terrain) {
        if (auto blockBiome = biome(blockInfo.blockBiomeIndex)) {
          if (!blockInfo.foregroundCave) {
            blockInfo.foreground = blockBiome->mainBlock;
            blockInfo.background = blockInfo.foreground;
          } else if (!blockInfo.backgroundCave) {
            blockInfo.background = blockBiome->mainBlock;
          }

          // subBlock, foregroundOre, and backgroundOre selectors can be empty
          // if they are not enabled, otherwise they will always have the
          // correct count

          if (!primaryBlockWeighting.region->subBlockSelectorIndexes.empty()) {
            for (size_t i = 0; i < blockBiome->subBlocks.size(); ++i) {
              auto const& selector = m_layout->getTerrainSelector(primaryBlockWeighting.region->subBlockSelectorIndexes.at(i));
              if (selector->get(primaryBlockWeighting.xValue - blendNoiseOffset, blockPos[1]) > 0.0f) {
                if (!blockInfo.foregroundCave) {
                  blockInfo.foreground = blockBiome->subBlocks.at(i);
                  blockInfo.background = blockInfo.foreground;
                } else if (!blockInfo.backgroundCave) {
                  blockInfo.background = blockBiome->subBlocks.at(i);
                }

                break;
              }
            }
          }

          if (!blockInfo.foregroundCave && !primaryBlockWeighting.region->foregroundOreSelectorIndexes.empty()) {
            for (size_t i = 0; i < blockBiome->ores.size(); ++i) {
              auto const& selector = m_layout->getTerrainSelector(primaryBlockWeighting.region->foregroundOreSelectorIndexes.at(i));
              if (selector->get(x, y) > 0.0f) {
                blockInfo.foregroundMod = blockBiome->ores.at(i).first;
                break;
              }
            }
          }

          if (!blockInfo.backgroundCave && !primaryBlockWeighting.region->backgroundOreSelectorIndexes.empty()) {
            for (size_t i = 0; i < blockBiome->ores.size(); ++i) {
              auto const& selector = m_layout->getTerrainSelector(primaryBlockWeighting.region->backgroundOreSelectorIndexes.at(i));
              if (selector->get(x, y) > 0.0f) {
                blockInfo.backgroundMod = blockBiome->ores.at(i).first;
                break;
              }
            }
          }
        }
      }

      return blockInfo;
    });
}

}
