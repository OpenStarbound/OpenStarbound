#ifndef STAR_BIOME_HPP
#define STAR_BIOME_HPP

#include "StarBiomePlacement.hpp"
#include "StarSpawner.hpp"

namespace Star {

STAR_STRUCT(AmbientNoisesDescription);
STAR_CLASS(Parallax);
STAR_STRUCT(BiomePlaceables);
STAR_STRUCT(Biome);

struct BiomePlaceables {
  BiomePlaceables();
  explicit BiomePlaceables(Json const& json);

  Json toJson() const;

  // If any of the item distributions contain trees, this returns the first
  // tree type.
  Maybe<TreeVariant> firstTreeType() const;

  ModId grassMod;
  float grassModDensity;
  ModId ceilingGrassMod;
  float ceilingGrassModDensity;

  List<BiomeItemDistribution> itemDistributions;
};

struct Biome {
  Biome();
  explicit Biome(Json const& store);

  Json toJson() const;

  String baseName;
  String description;

  MaterialId mainBlock;
  List<MaterialId> subBlocks;
  // Pairs the ore type with the commonality multiplier.
  List<pair<ModId, float>> ores;

  float hueShift;
  MaterialHue materialHueShift;

  BiomePlaceables surfacePlaceables;
  BiomePlaceables undergroundPlaceables;

  SpawnProfile spawnProfile;

  ParallaxPtr parallax;

  AmbientNoisesDescriptionPtr ambientNoises;
  AmbientNoisesDescriptionPtr musicTrack;
};

}

#endif
