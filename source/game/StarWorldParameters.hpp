#ifndef STAR_WORLD_PARAMTERS_HPP
#define STAR_WORLD_PARAMTERS_HPP

#include "StarGameTypes.hpp"
#include "StarSkyTypes.hpp"
#include "StarWeatherTypes.hpp"
#include "StarForceRegions.hpp"

namespace Star {

enum class WorldParametersType : uint8_t {
  TerrestrialWorldParameters,
  AsteroidsWorldParameters,
  FloatingDungeonWorldParameters
};
extern EnumMap<WorldParametersType> const WorldParametersTypeNames;

enum class BeamUpRule : uint8_t {
  Nowhere,
  Surface,
  Anywhere,
  AnywhereWithWarning
};
extern EnumMap<BeamUpRule> const BeamUpRuleNames;

enum class WorldEdgeForceRegionType : uint8_t {
  None,
  Top,
  Bottom,
  TopAndBottom
};
extern EnumMap<WorldEdgeForceRegionType> const WorldEdgeForceRegionTypeNames;

STAR_STRUCT(VisitableWorldParameters);
STAR_STRUCT(TerrestrialWorldParameters);
STAR_STRUCT(AsteroidsWorldParameters);
STAR_STRUCT(FloatingDungeonWorldParameters);

struct VisitableWorldParameters {
  VisitableWorldParameters();
  VisitableWorldParameters(VisitableWorldParameters const& visitableWorldParameters);
  VisitableWorldParameters(Json const& store);

  virtual ~VisitableWorldParameters();

  virtual WorldParametersType type() const = 0;

  virtual Json store() const;

  virtual void read(DataStream& ds);
  virtual void write(DataStream& ds) const;

  String typeName;
  float threatLevel;
  Vec2U worldSize;
  float gravity;
  bool airless;
  WeatherPool weatherPool;
  StringList environmentStatusEffects;
  Maybe<StringList> overrideTech;
  Maybe<StringList> globalDirectives;
  BeamUpRule beamUpRule;
  bool disableDeathDrops;
  bool terraformed;
  WorldEdgeForceRegionType worldEdgeForceRegions;
};

struct TerrestrialWorldParameters : VisitableWorldParameters {
  struct TerrestrialRegion {
    String biome;

    String blockSelector;
    String fgCaveSelector;
    String bgCaveSelector;
    String fgOreSelector;
    String bgOreSelector;
    String subBlockSelector;

    LiquidId caveLiquid;
    float caveLiquidSeedDensity;

    LiquidId oceanLiquid;
    int oceanLiquidLevel;

    bool encloseLiquids;
    bool fillMicrodungeons;
  };

  struct TerrestrialLayer {
    int layerMinHeight;
    int layerBaseHeight;

    StringList dungeons;
    int dungeonXVariance;

    TerrestrialRegion primaryRegion;
    TerrestrialRegion primarySubRegion;

    List<TerrestrialRegion> secondaryRegions;
    List<TerrestrialRegion> secondarySubRegions;

    Vec2F secondaryRegionSizeRange;
    Vec2F subRegionSizeRange;
  };

  TerrestrialWorldParameters();
  TerrestrialWorldParameters(TerrestrialWorldParameters const& terrestrialWorldParameters);
  TerrestrialWorldParameters(Json const& store);

  virtual WorldParametersType type() const override;

  virtual Json store() const override;

  virtual void read(DataStream& ds) override;
  virtual void write(DataStream& ds) const override;

  String primaryBiome;
  LiquidId primarySurfaceLiquid;
  String sizeName;
  float hueShift;

  SkyColoring skyColoring;
  float dayLength;

  Json blockNoiseConfig;
  Json blendNoiseConfig;
  float blendSize;

  TerrestrialLayer spaceLayer;
  TerrestrialLayer atmosphereLayer;
  TerrestrialLayer surfaceLayer;
  TerrestrialLayer subsurfaceLayer;
  List<TerrestrialLayer> undergroundLayers;
  TerrestrialLayer coreLayer;
};

struct AsteroidsWorldParameters : VisitableWorldParameters {
  AsteroidsWorldParameters();
  AsteroidsWorldParameters(Json const& store);

  virtual WorldParametersType type() const override;

  virtual Json store() const override;

  virtual void read(DataStream& ds) override;
  virtual void write(DataStream& ds) const override;

  int asteroidTopLevel;
  int asteroidBottomLevel;
  float blendSize;
  String asteroidBiome;
  Color ambientLightLevel;
};

struct FloatingDungeonWorldParameters : VisitableWorldParameters {
  FloatingDungeonWorldParameters();
  FloatingDungeonWorldParameters(Json const& store);

  virtual WorldParametersType type() const override;

  virtual Json store() const override;

  virtual void read(DataStream& ds) override;
  virtual void write(DataStream& ds) const override;

  int dungeonBaseHeight;
  int dungeonSurfaceHeight;
  int dungeonUndergroundLevel;
  String primaryDungeon;
  Color ambientLightLevel;
  Maybe<String> biome;
  Maybe<String> dayMusicTrack;
  Maybe<String> nightMusicTrack;
  Maybe<String> dayAmbientNoises;
  Maybe<String> nightAmbientNoises;
};

Json diskStoreVisitableWorldParameters(VisitableWorldParametersConstPtr const& parameters);
VisitableWorldParametersPtr diskLoadVisitableWorldParameters(Json const& store);

ByteArray netStoreVisitableWorldParameters(VisitableWorldParametersConstPtr const& parameters);
VisitableWorldParametersPtr netLoadVisitableWorldParameters(ByteArray data);

TerrestrialWorldParametersPtr generateTerrestrialWorldParameters(String const& typeName, String const& sizeName, uint64_t seed);
AsteroidsWorldParametersPtr generateAsteroidsWorldParameters(uint64_t seed);
FloatingDungeonWorldParametersPtr generateFloatingDungeonWorldParameters(String const& dungeonWorldName);

}

#endif
