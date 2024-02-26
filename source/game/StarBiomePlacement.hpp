#pragma once

#include "StarPerlin.hpp"
#include "StarWeightedPool.hpp"
#include "StarBiMap.hpp"
#include "StarPlant.hpp"
#include "StarTreasure.hpp"
#include "StarStrongTypedef.hpp"

namespace Star {

STAR_CLASS(BiomeItemDistribution);

STAR_EXCEPTION(BiomeException, StarException);

typedef pair<TreeVariant, TreeVariant> TreePair;

// Weighted pairs of object name / parameters.
typedef WeightedPool<pair<String, Json>> ObjectPool;

strong_typedef(String, TreasureBoxSet);
strong_typedef(StringSet, MicroDungeonNames);

typedef Variant<GrassVariant, BushVariant, TreePair, ObjectPool, TreasureBoxSet, MicroDungeonNames> BiomeItem;
BiomeItem variantToBiomeItem(Json const& store);
Json variantFromBiomeItem(BiomeItem const& biomeItem);

enum class BiomePlacementArea { Surface, Underground };
enum class BiomePlacementMode { Floor, Ceiling, Background, Ocean };
extern EnumMap<BiomePlacementMode> const BiomePlacementModeNames;

struct BiomeItemPlacement {
  BiomeItemPlacement(BiomeItem item, Vec2I position, float priority);

  // Orders by priority
  bool operator<(BiomeItemPlacement const& rhs) const;

  BiomeItem item;
  Vec2I position;
  float priority;
};

class BiomeItemDistribution {
public:
  struct PeriodicWeightedItem {
    BiomeItem item;
    PerlinF weight;
  };

  static Maybe<BiomeItem> createItem(Json const& itemSettings, RandomSource& rand, float biomeHueShift);

  BiomeItemDistribution();
  BiomeItemDistribution(Json const& config, uint64_t seed, float biomeHueShift = 0.0f);
  BiomeItemDistribution(Json const& store);

  Json toJson() const;

  BiomePlacementMode mode() const;
  List<BiomeItem> allItems() const;

  // Returns the best BiomeItem for this position out of the weighted item set,
  // if the density function specifies that an item should go in this position.
  Maybe<BiomeItemPlacement> itemToPlace(int x, int y) const;

private:
  enum class DistributionType {
    // Pure random distribution
    Random,
    // Uses perlin noise to morph a periodic function into a less predictable
    // periodic clumpy noise.
    Periodic
  };
  static EnumMap<DistributionType> const DistributionTypeNames;

  BiomePlacementMode m_mode;
  DistributionType m_distribution;
  float m_priority;

  // Used if the distribution type is Random

  float m_blockProbability;
  uint64_t m_blockSeed;
  List<BiomeItem> m_randomItems;

  // Used if the distribution type is Periodic

  PerlinF m_densityFunction;
  PerlinF m_modulusDistortion;
  int m_modulus;
  int m_modulusOffset;
  // Pairs items with a periodic weight.  Weight will vary over the space of
  // the distribution, If multiple items are present, this can be used to
  // select one of the items (with the highest weight) out of a list of items,
  // causing items to be grouped spatially in a way determined by the shape of
  // each weight function.
  List<pair<BiomeItem, PerlinF>> m_weightedItems;
};

}
