#include "StarBiomePlacement.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

BiomeItem variantToBiomeItem(Json const& store) {
  auto type = store.get(0);
  if (type == "grass") {
    return GrassVariant(store.get(1));
  } else if (type == "bush") {
    return BushVariant(store.get(1));
  } else if (type == "treePair") {
    return TreePair(TreeVariant(store.get(1).get(0)), TreeVariant(store.get(1).get(1)));
  } else if (type == "objectPool") {
    return ObjectPool(store.getArray(1).transformed([](Json const& pair) {
        return make_pair(pair.getFloat(0), make_pair(pair.get(1).getString(0), pair.get(1).get(1)));
      }));
  } else if (type == "treasureBoxSet") {
    return TreasureBoxSet(store.getString(1));
  } else if (type == "microDungeon") {
    return MicroDungeonNames(jsonToStringSet(store.get(1)));
  } else {
    throw BiomeException(strf("Unrecognized biome item type '{}'", type));
  }
}

Json variantFromBiomeItem(BiomeItem const& biomeItem) {
  if (auto grassVariant = biomeItem.ptr<GrassVariant>()) {
    return JsonArray{"grass", grassVariant->toJson()};
  } else if (auto bushVariant = biomeItem.ptr<BushVariant>()) {
    return JsonArray{"bush", bushVariant->toJson()};
  } else if (auto treePair = biomeItem.ptr<TreePair>()) {
    return JsonArray{"treePair", JsonArray{treePair->first.toJson(), treePair->second.toJson()}};
  } else if (auto objectPool = biomeItem.ptr<ObjectPool>()) {
    return JsonArray{"objectPool", transform<JsonArray>(objectPool->items(), [](pair<double, pair<String, Json>> const& p) {
        return JsonArray{p.first, JsonArray{p.second.first, p.second.second}};
      })};
  } else if (auto treasureBoxSet = biomeItem.ptr<TreasureBoxSet>()) {
    return JsonArray{"treasureBoxSet", String(*treasureBoxSet)};
  } else if (auto microDungeonNames = biomeItem.ptr<MicroDungeonNames>()) {
    return JsonArray{"microDungeon", jsonFromStringSet(*microDungeonNames)};
  } else {
    throw BiomeException(strf("Unrecognized biome item type"));
  }
}

EnumMap<BiomePlacementMode> const BiomePlacementModeNames{
  {BiomePlacementMode::Floor, "floor"},
  {BiomePlacementMode::Ceiling, "ceiling"},
  {BiomePlacementMode::Background, "background"},
  {BiomePlacementMode::Ocean, "ocean"}
};

BiomeItemPlacement::BiomeItemPlacement(BiomeItem item, Vec2I position, float priority)
  : item(move(item)), position(position), priority(priority) {}

bool BiomeItemPlacement::operator<(BiomeItemPlacement const& rhs) const {
  return priority < rhs.priority;
}

Maybe<BiomeItem> BiomeItemDistribution::createItem(Json const& config, RandomSource& rand, float biomeHueShift) {
  auto& root = Root::singleton();

  auto type = config.getString("type");
  if (type.equalsIgnoreCase("grass")) {
    auto grassList = jsonToStringList(config.get("grasses"));
    return BiomeItem{root.plantDatabase()->buildGrassVariant(rand.randFrom(grassList), biomeHueShift)};

  } else if (type.equalsIgnoreCase("bush")) {
    auto bushList = config.getArray("bushes", {});
    auto bushSettings = rand.randValueFrom(bushList);

    auto bushName = bushSettings.getString("name");
    auto bushMod = rand.randValueFrom(root.plantDatabase()->bushMods(bushName));
    float bushBaseHueShift = rand.randf(-1.0f, 1.0f) * bushSettings.getFloat("baseHueShiftMax");
    float bushModHueShift = rand.randf(-1.0f, 1.0f) * bushSettings.getFloat("modHueShiftMax");

    return BiomeItem{root.plantDatabase()->buildBushVariant(bushName, bushBaseHueShift, bushMod, bushModHueShift)};

  } else if (type.equalsIgnoreCase("tree")) {
    auto stemList = jsonToStringList(config.get("treeStemList", JsonArray()));
    auto foliageList = jsonToStringList(config.get("treeFoliageList", JsonArray()));

    // Find matching pairs of stem / foliage (that have the same shape)
    List<pair<String, String>> matchingPairs;
    for (auto stem : stemList) {
      for (auto foliage : foliageList) {
        if (foliage.empty() || root.plantDatabase()->treeStemShape(stem) == root.plantDatabase()->treeFoliageShape(foliage))
          matchingPairs.append({stem, foliage});
      }
    }

    if (matchingPairs.empty() && !stemList.empty() && !foliageList.empty())
      Logger::warn("Specified stemList and foliageList, but no matching pairs found.");

    auto chosenPair = rand.randValueFrom(matchingPairs);
    float treeStemHueShift = rand.randf(-1.0f, 1.0f) * config.getFloat("treeStemHueShiftMax", 0);
    float treeFoliageHueShift = rand.randf(-1.0f, 1.0f) * config.getFloat("treeFoliageHueShiftMax", 0);
    float treeAltFoliageHueShift = rand.randf(-1.0f, 1.0f) * config.getFloat("treeFoliageHueShiftMax", 0);

    if (!chosenPair.first.empty()) {
      TreeVariant primaryTree;
      TreeVariant altTree;
      if (chosenPair.second.empty()) {
        // Foliage-less trees
        primaryTree = root.plantDatabase()->buildTreeVariant(chosenPair.first, treeStemHueShift);
        altTree = root.plantDatabase()->buildTreeVariant(chosenPair.first, treeStemHueShift);
      } else {
        primaryTree = root.plantDatabase()->buildTreeVariant(
            chosenPair.first, treeStemHueShift, chosenPair.second, treeFoliageHueShift);
        altTree = root.plantDatabase()->buildTreeVariant(
            chosenPair.first, treeStemHueShift, chosenPair.second, treeAltFoliageHueShift);
      }
      return BiomeItem{TreePair{primaryTree, altTree}};
    }

  } else if (type.equalsIgnoreCase("object")) {
    Json objectPoolConfig = rand.randValueFrom(config.getArray("objectSets"));
    ObjectPool objectPool;

    Json objectParameters = objectPoolConfig.get("parameters", JsonObject());
    for (auto const& pair : objectPoolConfig.getArray("pool")) {
      if (pair.size() != 2)
        throw BiomeException("Wrong size for objects weight / list pair in biome items");

      objectPool.add(pair.getFloat(0), {pair.getString(1), objectParameters});
    }

    return BiomeItem{objectPool};

  } else if (type.equalsIgnoreCase("treasureBox")) {
    return BiomeItem{TreasureBoxSet(rand.randValueFrom(config.getArray("treasureBoxSets")).toString())};

  } else if (type.equalsIgnoreCase("microdungeon")) {
    return BiomeItem{MicroDungeonNames(jsonToStringSet(config.get("microdungeons", JsonArray())))};

  } else {
    throw BiomeException(strf("No such item type '{}' in item distribution", type));
  }

  return {};
}

BiomeItemDistribution::BiomeItemDistribution() {
  m_mode = BiomePlacementMode::Floor;
  m_distribution = DistributionType::Random;
  m_modulus = 1;
  m_modulusOffset = 0;
  m_blockSeed = 0;
  m_blockProbability = 0.0f;
  m_priority = 0.0f;
}

BiomeItemDistribution::BiomeItemDistribution(Json const& config, uint64_t seed, float biomeHueShift) {
  RandomSource rand(seed);

  m_mode = BiomePlacementModeNames.getLeft(config.getString("mode", "floor"));

  m_priority = config.getFloat("priority", 0.0f);
  int variants = config.getInt("variants", 1);

  m_modulus = 1;
  m_modulusOffset = 0;
  m_blockSeed = 0;
  m_blockProbability = 0.0f;

  // If distribution settings are string type, it should point to another asset
  // variant.
  auto distributionSettings = config.get("distribution", JsonObject());
  if (distributionSettings.type() == Json::Type::String) {
    auto assets = Root::singleton().assets();
    distributionSettings = assets->json(distributionSettings.toString());
  }

  m_distribution = DistributionTypeNames.getLeft(distributionSettings.getString("type"));
  if (m_distribution == DistributionType::Random) {
    m_blockProbability = distributionSettings.getFloat("blockProbability");
    m_blockSeed = rand.randu64();
    for (int i = 0; i < variants; ++i) {
      if (auto item = createItem(config, rand, biomeHueShift))
        m_randomItems.append(item.take());
    }

  } else if (m_distribution == DistributionType::Periodic) {
    unsigned octaves = distributionSettings.getInt("octaves", 1);
    float alpha = distributionSettings.getFloat("alpha", 2.0);
    float beta = distributionSettings.getFloat("beta", 2.0);

    float modulusVariance = distributionSettings.getFloat("modulusVariance", 0.0);

    // If density period / offset are not set, just offset a lot to get an even
    // distribution with no free spaces.
    float densityPeriod = distributionSettings.getFloat("densityPeriod", 10);
    float densityOffset = distributionSettings.getFloat("densityOffset", 2.0);

    float typePeriod = distributionSettings.getFloat("typePeriod", 10);

    m_modulus = distributionSettings.getInt("modulus", 1);
    m_modulusOffset = rand.randInt(-m_modulus, m_modulus);
    m_densityFunction = PerlinF(octaves, 1.0f / densityPeriod, 1.0, densityOffset, alpha, beta, rand.randu64());
    m_modulusDistortion = PerlinF(octaves, 1.0f / m_modulus, modulusVariance, modulusVariance * 2, alpha, beta, rand.randu64());

    for (int i = 0; i < variants; ++i) {
      if (auto item = createItem(config, rand, biomeHueShift)) {
        PerlinF weight(octaves, 1.0f / typePeriod, 1.0, 0.0, alpha, beta, rand.randu64());
        m_weightedItems.append({item.take(), weight});
      }
    }
  }
}

BiomeItemDistribution::BiomeItemDistribution(Json const& store) {
  m_mode = BiomePlacementModeNames.getLeft(store.getString("mode"));
  m_distribution = DistributionTypeNames.getLeft(store.getString("distribution"));
  m_priority = store.getFloat("priority");
  m_blockProbability = store.getFloat("blockProbability");
  m_blockSeed = store.getUInt("blockSeed");
  m_randomItems = store.getArray("randomItems").transformed(variantToBiomeItem);
  m_densityFunction = PerlinF(store.get("densityFunction"));
  m_modulusDistortion = PerlinF(store.get("modulusDistortion"));
  m_modulus = store.getInt("modulus");
  m_modulusOffset = store.getInt("modulusOffset");
  m_weightedItems = store.getArray("weightedItems") .transformed([](Json const& v) {
      return make_pair(variantToBiomeItem(v.get(0)), PerlinF(v.get(1)));
    });
}

Json BiomeItemDistribution::toJson() const {
  return JsonObject{
    {"mode", BiomePlacementModeNames.getRight(m_mode)},
    {"distribution", DistributionTypeNames.getRight(m_distribution)},
    {"priority", m_priority},
    {"blockProbability", m_blockProbability},
    {"blockSeed", m_blockSeed},
    {"randomItems", m_randomItems.transformed(variantFromBiomeItem)},
    {"densityFunction", m_densityFunction.toJson()},
    {"modulusDistortion", m_modulusDistortion.toJson()},
    {"modulus", m_modulus},
    {"modulusOffset", m_modulusOffset},
    {"weightedItems", m_weightedItems.transformed([](pair<BiomeItem, PerlinF> const& p) -> Json {
        return JsonArray{variantFromBiomeItem(p.first), p.second.toJson()};
      })},
  };
}

BiomePlacementMode BiomeItemDistribution::mode() const {
  return m_mode;
}

List<BiomeItem> BiomeItemDistribution::allItems() const {
  if (m_distribution == DistributionType::Random) {
    return m_randomItems;
  } else if (m_distribution == DistributionType::Periodic) {
    List<BiomeItem> items;
    for (auto const& pair : m_weightedItems)
      items.append(pair.first);
    return items;
  } else {
    return {};
  }
}

Maybe<BiomeItemPlacement> BiomeItemDistribution::itemToPlace(int x, int y) const {
  if (m_distribution == DistributionType::Random) {
    if (staticRandomFloat(x, y, m_blockSeed) <= m_blockProbability)
      return BiomeItemPlacement{staticRandomValueFrom(m_randomItems, x, y, m_blockSeed), Vec2I(x, y), m_priority};

  } else if (m_distribution == DistributionType::Periodic) {
    BiomeItem const* biomeItem = nullptr;
    if (m_densityFunction.get(x, y) > 0) {
      if ((int)(x + m_modulusOffset + m_modulusDistortion.get(x, y)) % m_modulus == 0) {
        float maxWeight = lowest<float>();
        for (auto const& weightedItem : m_weightedItems) {
          float weight = weightedItem.second.get(x, y);
          if (weight > maxWeight) {
            maxWeight = weight;
            biomeItem = &weightedItem.first;
          }
        }
      }
    }

    if (biomeItem)
      return BiomeItemPlacement{*biomeItem, Vec2I(x, y), m_priority};
  }

  return {};
}

EnumMap<BiomeItemDistribution::DistributionType> const BiomeItemDistribution::DistributionTypeNames{
  {DistributionType::Random, "random"},
  {DistributionType::Periodic, "periodic"}
};

}
