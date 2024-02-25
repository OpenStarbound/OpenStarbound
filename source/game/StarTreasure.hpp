#pragma once

#include "StarThread.hpp"
#include "StarParametricFunction.hpp"
#include "StarWeightedPool.hpp"
#include "StarItemDescriptor.hpp"
#include "StarGameTypes.hpp"

namespace Star {

STAR_CLASS(World);
STAR_CLASS(Item);
STAR_CLASS(ItemBag);
STAR_CLASS(ContainerObject);
STAR_CLASS(TreasureDatabase);

STAR_EXCEPTION(TreasureException, StarException);

class TreasureDatabase {
public:
  TreasureDatabase();

  StringList treasurePools() const;
  bool isTreasurePool(String const& treasurePool) const;

  StringList treasureChestSets() const;
  bool isTreasureChestSet(String const& treasurePool) const;

  List<ItemPtr> createTreasure(String const& treasurePool, float level) const;
  List<ItemPtr> createTreasure(String const& treasurePool, float level, uint64_t seed) const;

  // Adds created treasure to the given ItemBags, does not clear the ItemBag
  // first.  Returns overflow items.
  List<ItemPtr> fillWithTreasure(ItemBagPtr const& itemBag, String const& treasurePool, float level) const;
  List<ItemPtr> fillWithTreasure(ItemBagPtr const& itemBag, String const& treasurePool, float level, uint64_t seed) const;

  // If the given container does not fit at this position, or if the treasure
  // box set does not have an entry with a minimum level less than the given
  // world threat level, this method will return null.
  ContainerObjectPtr createTreasureChest(World* world, String const& treasureChestSet, Vec2I const& position, Direction direction) const;
  ContainerObjectPtr createTreasureChest(World* world, String const& treasureChestSet, Vec2I const& position, Direction direction, uint64_t seed) const;

private:
  List<ItemPtr> createTreasure(String const& treasurePool, float level, uint64_t seed, StringSet visitedPools) const;

  // Specifies either an item descriptor or the name of a valid treasurepool to
  // be
  // used when an entry is selected in a "fill" or "pool" list
  typedef MVariant<String, ItemDescriptor> TreasureEntry;

  struct ItemPool {
    ItemPool();

    // If non-empty, the treasure set is pre-filled with this before selecting
    // from the pool.
    List<TreasureEntry> fill;

    // Weighted pool of items to select from.
    WeightedPool<TreasureEntry> pool;

    // Weighted pool for the number of pool rounds.
    WeightedPool<int> poolRounds;

    // Any item levels that are applied will have a random value
    // from this range added to their level.
    Vec2F levelVariance;

    // When generating more than one item, should we allow each cycle to
    // generate an item that is stackable with a previous item?  This is not to
    // say a stack could actually be formed in an ItemBag, simply that the
    // Item::stackableWith method returns true.
    // Note that this flag does not apply to child pools
    bool allowDuplication;
  };
  typedef ParametricTable<float, ItemPool> TreasurePool;

  struct TreasureChest {
    TreasureChest();

    StringList containers;
    String treasurePool;
    float minimumLevel;
  };
  typedef List<TreasureChest> TreasureChestSet;

  StringMap<TreasurePool> m_treasurePools;
  StringMap<TreasureChestSet> m_treasureChestSets;
};

}
