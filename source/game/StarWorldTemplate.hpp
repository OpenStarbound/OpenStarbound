#pragma once

#include "StarOrderedMap.hpp"
#include "StarLruCache.hpp"
#include "StarWorldLayout.hpp"
#include "StarBiomePlacement.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarSkyParameters.hpp"
#include "StarAmbient.hpp"

namespace Star {

STAR_CLASS(WorldTemplate);

// Reference object that describes the generation of a single world, and all
// of the world metadata.  Meant to remain static (or relatively static)
// throughout the life of a World.
class WorldTemplate {
public:
  struct Dungeon {
    String dungeon;
    int baseHeight;
    int baseX;
    int xVariance;
    bool force;
    bool blendWithTerrain;
  };

  struct BlockInfo {
    BiomeIndex blockBiomeIndex = NullBiomeIndex;
    BiomeIndex environmentBiomeIndex = NullBiomeIndex;

    bool biomeTransition = false;

    bool terrain = false;
    bool foregroundCave = false;
    bool backgroundCave = false;

    MaterialId foreground = EmptyMaterialId;
    ModId foregroundMod = NoModId;

    MaterialId background = EmptyMaterialId;
    ModId backgroundMod = NoModId;

    LiquidId caveLiquid = EmptyLiquidId;
    float caveLiquidSeedDensity = 0.0f;

    LiquidId oceanLiquid = EmptyLiquidId;
    int oceanLiquidLevel = 0;

    bool encloseLiquids = false;
    bool fillMicrodungeons = false;

    Json toJson() const;
  };

  struct PotentialBiomeItems {
    // Potential items that would spawn at the given block assuming it is at
    // the surface.
    List<BiomeItemPlacement> surfaceBiomeItems;
    // ... Or on a cave surface.
    List<BiomeItemPlacement> caveSurfaceBiomeItems;
    // ... Or on a cave ceiling.
    List<BiomeItemPlacement> caveCeilingBiomeItems;
    // ... Or on a cave background wall.
    List<BiomeItemPlacement> caveBackgroundBiomeItems;
    // ... Or in the ocean
    List<BiomeItemPlacement> oceanItems;
  };

  // Creates a blank world with the given size
  WorldTemplate(Vec2U const& size);
  // Creates a world from the given visitable celestial object.
  WorldTemplate(CelestialCoordinate const& celestialCoordinate, CelestialDatabasePtr const& celestialDatabase);
  // Creates a world from a bare VisitableWorldParameters structure
  WorldTemplate(VisitableWorldParametersConstPtr const& worldParameters, SkyParameters const& skyParameters, uint64_t seed);
  // Load a world template from the given stored data.
  WorldTemplate(Json const& store);

  Json store() const;

  Maybe<CelestialParameters> const& celestialParameters() const;
  VisitableWorldParametersConstPtr worldParameters() const;
  SkyParameters skyParameters() const;
  WorldLayoutPtr worldLayout() const;

  void setWorldParameters(VisitableWorldParametersPtr newParameters);
  void setWorldLayout(WorldLayoutPtr newLayout);
  void setSkyParameters(SkyParameters newParameters);

  uint64_t worldSeed() const;
  String worldName() const;

  Vec2U size() const;

  // The average (ish) surface level for this world, off of which terrain
  // generators modify the surface height.
  float surfaceLevel() const;

  // The constant height at which everything below is considered "underground"
  float undergroundLevel() const;

  // returns true if the world is terrestrial and the specified position is within the
  // planet's surface layer
  bool inSurfaceLayer(Vec2I const& position) const;

  // If it is specified, searches the player start search region for an
  // acceptable player start area.  The block returned will be an empty block
  // above a terrain block, in a region of free space.  If no such block can be
  // found or the player start search region is not specified, returns nothing.
  Maybe<Vec2I> findSensiblePlayerStart() const;

  // Add either a solid region hint or a space region hint for the given
  // polygonal region.  Blending size and weighting is configured in the
  // WorldTemplate config file.
  void addCustomTerrainRegion(PolyF poly);
  void addCustomSpaceRegion(PolyF poly);
  void clearCustomTerrains();

  List<RectI> previewAddBiomeRegion(Vec2I const& position, int width);
  List<RectI> previewExpandBiomeRegion(Vec2I const& position, int newWidth);

  void addBiomeRegion(Vec2I const& position, String const& biomeName, String const& subBlockSelector, int width);
  void expandBiomeRegion(Vec2I const& position, int newWidth);

  List<Dungeon> dungeons() const;

  // Is this tile block naturally outside the terrain?
  bool isOutside(int x, int y) const;
  // Is this integral region of blocks outside the terrain?
  bool isOutside(RectI const& region) const;

  BlockInfo blockInfo(int x, int y) const;

  // partial blockinfo that doesn't use terrain selectors
  BlockInfo blockBiomeInfo(int x, int y) const;

  BiomeIndex blockBiomeIndex(int x, int y) const;
  BiomeIndex environmentBiomeIndex(int x, int y) const;
  BiomeConstPtr biome(BiomeIndex biomeIndex) const;

  BiomeConstPtr blockBiome(int x, int y) const;
  BiomeConstPtr environmentBiome(int x, int y) const;

  MaterialId biomeMaterial(BiomeIndex biomeIndex, int x, int y) const;

  // Returns the material and mod hue shift that should be applied to the given
  // material and mod for this biome.
  MaterialHue biomeMaterialHueShift(BiomeIndex biomeIndex, MaterialId material) const;
  MaterialHue biomeModHueShift(BiomeIndex biomeIndex, ModId mod) const;

  AmbientNoisesDescriptionPtr ambientNoises(int x, int y) const;
  AmbientNoisesDescriptionPtr musicTrack(int x, int y) const;

  StringList environmentStatusEffects(int x, int y) const;
  bool breathable(int x, int y) const;

  WeatherPool weathers() const;

  // Return potential items that would spawn at the given block.
	void addPotentialBiomeItems(int x, int y, PotentialBiomeItems& items, List<BiomeItemDistribution> const& distributions, BiomePlacementArea area, Maybe<BiomePlacementMode> mode = {}) const;
  PotentialBiomeItems potentialBiomeItemsAt(int x, int y) const;

  // Return only the potential items that can spawn at the given block.
	List<BiomeItemPlacement> validBiomeItems(int x, int y, PotentialBiomeItems potentialBiomeItems) const;

  float gravity() const;
  float threatLevel() const;

  // For consistently seeding object generation at this position
  uint64_t seedFor(int x, int y) const;

private:
  struct CustomTerrainRegion {
    PolyF region;
    RectF regionBounds;
    bool solid;
  };

  WorldTemplate();

  void determineWorldName();

  pair<float, float> customTerrainWeighting(int x, int y) const;

  // Calculates block info and adds to cache
  BlockInfo getBlockInfo(uint32_t x, uint32_t y) const;

  Json m_templateConfig;
  float m_customTerrainBlendSize;
  float m_customTerrainBlendWeight;

  Maybe<CelestialParameters> m_celestialParameters;
  VisitableWorldParametersConstPtr m_worldParameters;
  SkyParameters m_skyParameters;
  uint64_t m_seed;
  WorldGeometry m_geometry;
  WorldLayoutPtr m_layout;
  String m_worldName;

  List<CustomTerrainRegion> m_customTerrainRegions;

  mutable HashLruCache<Vector<uint32_t, 2>, BlockInfo> m_blockCache;
};

}
