#include "StarBiome.hpp"
#include "StarJsonExtra.hpp"
#include "StarParallax.hpp"
#include "StarAmbient.hpp"

namespace Star {

BiomePlaceables::BiomePlaceables() {
  grassMod = NoModId;
  grassModDensity = 0.0f;
  ceilingGrassMod = NoModId;
  ceilingGrassModDensity = 0.0f;
}

BiomePlaceables::BiomePlaceables(Json const& variant) {
  grassMod = variant.getInt("grassMod");
  grassModDensity = variant.getFloat("grassModDensity");
  ceilingGrassMod = variant.getInt("ceilingGrassMod");
  ceilingGrassModDensity = variant.getFloat("ceilingGrassModDensity");
  itemDistributions = variant.getArray("itemDistributions").transformed(construct<BiomeItemDistribution>());
}

Json BiomePlaceables::toJson() const {
  return JsonObject{
    {"grassMod", grassMod},
    {"grassModDensity", grassModDensity},
    {"ceilingGrassMod", ceilingGrassMod},
    {"ceilingGrassModDensity", ceilingGrassModDensity},
    {"itemDistributions", itemDistributions.transformed(mem_fn(&BiomeItemDistribution::toJson))}
  };
}

Maybe<TreeVariant> BiomePlaceables::firstTreeType() const {
  for (auto const& itemDistribution : itemDistributions) {
    for (auto const& biomeItem : itemDistribution.allItems()) {
      if (biomeItem.is<TreePair>())
        return biomeItem.get<TreePair>().first;
    }
  }
  return {};
}

Biome::Biome() {
  mainBlock = EmptyMaterialId;
  hueShift = 0.0f;
  materialHueShift = MaterialHue();
}

Biome::Biome(Json const& store) : Biome() {
  baseName = store.getString("baseName");
  description = store.getString("description");

  mainBlock = store.getUInt("mainBlock");
  subBlocks = store.getArray("subBlocks").transformed([](Json const& v) -> MaterialId { return v.toUInt(); });
  ores =
      store.getArray("ores").transformed([](Json const& v) { return pair<ModId, float>(v.getUInt(0), v.getFloat(1)); });
  hueShift = store.getFloat("hueShift");
  materialHueShift = store.getUInt("materialHueShift");

  surfacePlaceables = BiomePlaceables(store.get("surfacePlaceables"));
  undergroundPlaceables = BiomePlaceables(store.get("undergroundPlaceables"));

  if (auto config = store.opt("spawnProfile"))
    spawnProfile = SpawnProfile(*config);

  if (auto config = store.opt("parallax"))
    parallax = make_shared<Parallax>(*config);

  if (auto config = store.opt("ambientNoises"))
    ambientNoises = make_shared<AmbientNoisesDescription>(*config);
  if (auto config = store.opt("musicTrack"))
    musicTrack = make_shared<AmbientNoisesDescription>(*config);
}

Json Biome::toJson() const {
  return JsonObject{{"baseName", baseName},
      {"description", description},
      {"mainBlock", mainBlock},
      {"subBlocks", subBlocks.transformed(construct<Json>())},
      {"ores",
          ores.transformed([](pair<ModId, float> const& p) -> Json {
            return JsonArray{p.first, p.second};
          })},
      {"hueShift", hueShift},
      {"materialHueShift", materialHueShift},
      {"surfacePlaceables", surfacePlaceables.toJson()},
      {"undergroundPlaceables", undergroundPlaceables.toJson()},
      {"spawnProfile", spawnProfile.toJson()},
      {"parallax", parallax ? parallax->store() : Json()},
      {"ambientNoises", ambientNoises ? ambientNoises->toJson() : Json()},
      {"musicTrack", musicTrack ? musicTrack->toJson() : Json()}};
}

}
