#include "StarTerrainDatabase.hpp"
#include "StarRidgeBlocksSelector.hpp"
#include "StarKarstCave.hpp"
#include "StarWormCave.hpp"
#include "StarConstantSelector.hpp"
#include "StarMaxSelector.hpp"
#include "StarMinMaxSelector.hpp"
#include "StarFlatSurfaceSelector.hpp"
#include "StarDisplacementSelector.hpp"
#include "StarRotateSelector.hpp"
#include "StarMixSelector.hpp"
#include "StarPerlinSelector.hpp"
#include "StarCacheSelector.hpp"
#include "StarIslandSurfaceSelector.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

TerrainSelectorParameters::TerrainSelectorParameters() {
  seed = Random::randu64();
  worldWidth = 0;
  commonality = 1.0f;
}

TerrainSelectorParameters::TerrainSelectorParameters(Json const& v) {
  worldWidth = v.getUInt("worldWidth");
  baseHeight = v.getFloat("baseHeight");
  seed = v.getUInt("seed");
  commonality = v.getFloat("commonality");
}

Json TerrainSelectorParameters::toJson() const {
  return JsonObject{
      {"worldWidth", worldWidth}, {"baseHeight", baseHeight}, {"seed", seed}, {"commonality", commonality}};
}

TerrainSelectorParameters TerrainSelectorParameters::withSeed(uint64_t seed) const {
  TerrainSelectorParameters copy = *this;
  copy.seed = seed;
  return copy;
}

TerrainSelectorParameters TerrainSelectorParameters::withCommonality(float commonality) const {
  TerrainSelectorParameters copy = *this;
  copy.commonality = commonality;
  return copy;
}

TerrainSelector::TerrainSelector(String type, Json config, TerrainSelectorParameters parameters)
  : type(move(type)), config(move(config)), parameters(move(parameters)) {}

TerrainSelector::~TerrainSelector() {}

TerrainDatabase::TerrainDatabase() {
  auto assets = Root::singleton().assets();

  // 'type' here is the extension of the file, and determines the selector type
  auto scanFiles = [this, assets](String const& type) {
    auto files = assets->scanExtension(type);
    assets->queueJsons(files);
    for (auto path : files) {
      auto parameters = assets->json(path);
      auto name = parameters.getString("name");
      if (m_terrainSelectors.contains(name))
        throw TerrainException(strf("Duplicate terrain generator name '%s'", name));
      m_terrainSelectors[name] = {type, parameters};
    }
  };

  scanFiles(KarstCaveSelector::Name);
  scanFiles(WormCaveSelector::Name);
  scanFiles(RidgeBlocksSelector::Name);

  auto files = assets->scanExtension("terrain");
  assets->queueJsons(files);
  for (auto path : files) {
    auto parameters = assets->json(path);
    auto name = parameters.getString("name");
    auto type = parameters.getString("type");
    if (m_terrainSelectors.contains(name))
      throw TerrainException(strf("Duplicate composed terrain generator name '%s'", name));
    m_terrainSelectors[name] = {type, parameters};
  }
}

TerrainDatabase::Config TerrainDatabase::selectorConfig(String const& name) const {
  if (auto config = m_terrainSelectors.maybe(name))
    return config.take();
  else
    throw TerrainException(strf("No such terrain selector '%s'", name));
}

TerrainSelectorConstPtr TerrainDatabase::createNamedSelector(String const& name, TerrainSelectorParameters const& parameters) const {
  auto config = selectorConfig(name);

  return createSelectorType(config.type, config.parameters, parameters);
}

TerrainSelectorConstPtr TerrainDatabase::constantSelector(float value) {
  return createSelectorType(ConstantSelector::Name, JsonObject{{"value", value}}, TerrainSelectorParameters());
}

Json TerrainDatabase::storeSelector(TerrainSelectorConstPtr const& selector) const {
  if (!selector)
    return {};

  return JsonObject{
    {"type", selector->type},
    {"config", selector->config},
    {"parameters", selector->parameters.toJson()}
  };
}

TerrainSelectorConstPtr TerrainDatabase::loadSelector(Json const& store) const {
  if (store.isNull())
    return {};
  return createSelectorType(store.getString("type"), store.get("config"),
      TerrainSelectorParameters(store.get("parameters")));
}

TerrainSelectorConstPtr TerrainDatabase::createSelectorType(String const& type, Json const& config, TerrainSelectorParameters const& parameters) const {
  if (type == WormCaveSelector::Name)
    return make_shared<WormCaveSelector>(config, parameters);
  else if (type == KarstCaveSelector::Name)
    return make_shared<KarstCaveSelector>(config, parameters);
  else if (type == ConstantSelector::Name)
    return make_shared<ConstantSelector>(config, parameters);
  else if (type == MaxSelector::Name)
    return make_shared<MaxSelector>(config, parameters, this);
  else if (type == MinMaxSelector::Name)
    return make_shared<MinMaxSelector>(config, parameters, this);
  else if (type == IslandSurfaceSelector::Name)
    return make_shared<IslandSurfaceSelector>(config, parameters);
  else if (type == FlatSurfaceSelector::Name)
    return make_shared<FlatSurfaceSelector>(config, parameters);
  else if (type == DisplacementSelector::Name)
    return make_shared<DisplacementSelector>(config, parameters, this);
  else if (type == RotateSelector::Name)
    return make_shared<RotateSelector>(config, parameters, this);
  else if (type == MixSelector::Name)
    return make_shared<MixSelector>(config, parameters, this);
  else if (type == PerlinSelector::Name)
    return make_shared<PerlinSelector>(config, parameters);
  else if (type == RidgeBlocksSelector::Name)
    return make_shared<RidgeBlocksSelector>(config, parameters);
  else if (type == CacheSelector::Name)
    return make_shared<CacheSelector>(config, parameters, this);
  else
    throw TerrainException(strf("Unknown terrain selector type '%s'", type));
}

}
