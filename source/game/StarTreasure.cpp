#include "StarTreasure.hpp"
#include "StarObjectDatabase.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarAssets.hpp"
#include "StarItemBag.hpp"
#include "StarWorld.hpp"
#include "StarContainerObject.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

TreasureDatabase::TreasureDatabase() {
  auto assets = Root::singleton().assets();

  auto treasurePools = assets->scanExtension("treasurepools");
  auto treasureChests = assets->scanExtension("treasurechests");

  assets->queueJsons(treasurePools);
  assets->queueJsons(treasureChests);

  for (auto file : treasurePools) {
    for (auto const& pair : assets->json(file).iterateObject()) {
      if (m_treasurePools.contains(pair.first))
        throw TreasureException(strf("Duplicate TreasurePool config '{}' from file '{}'", pair.first, file));

      auto& treasurePool = m_treasurePools[pair.first];
      for (auto const& entry : pair.second.iterateArray()) {
        if (entry.size() != 2)
          throw TreasureException("Wrong size for TreasurePool entry, list must be 2");

        float startLevel = entry.getFloat(0);
        auto config = entry.get(1);

        ItemPool itemPool;

        for (auto const& entry : config.getArray("fill", {}))
          if (entry.contains("pool"))
            itemPool.fill.append(entry.getString("pool"));
          else if (entry.contains("item"))
            itemPool.fill.append(ItemDescriptor(entry.get("item")));
          else
            throw TreasureException(strf("TreasurePool entry '{}' did not specify a valid 'item' or 'pool'", entry));

        for (auto const& entry : config.getArray("pool", {})) {
          if (!entry.contains("weight"))
            throw TreasureException(strf("TreasurePool entry '{}' did not specify a weight", entry));

          if (entry.contains("pool"))
            itemPool.pool.add(entry.getFloat("weight"), entry.getString("pool"));
          else if (entry.contains("item"))
            itemPool.pool.add(entry.getFloat("weight"), ItemDescriptor(entry.get("item")));
          else
            throw TreasureException(strf("TreasurePool entry '{}' did not specify a valid 'item' or 'pool'", entry));
        }

        auto poolRounds = config.get("poolRounds", 1);
        if (poolRounds.canConvert(Json::Type::Float)) {
          itemPool.poolRounds = WeightedPool<int>(List<std::pair<double, int>>{{1.0, poolRounds.toFloat()}});
        } else {
          for (auto const& pair : poolRounds.iterateArray())
            itemPool.poolRounds.add(pair.getDouble(0), pair.getInt(1));
        }

        itemPool.levelVariance = jsonToVec2F(config.get("levelVariance", JsonArray{0, 0}));
        itemPool.allowDuplication = config.getBool("allowDuplication", true);

        treasurePool.addPoint(startLevel, std::move(itemPool));
      }
    }
  }

  for (auto file : treasureChests) {
    for (auto const& pair : assets->json(file).iterateObject()) {
      if (m_treasureChestSets.contains(pair.first))
        throw TreasureException(strf("Duplicate TreasureChestSet config '{}' from file '{}'", pair.first, file));

      auto& treasureChestSet = m_treasureChestSets[pair.first];
      for (auto const& entry : pair.second.iterateArray()) {
        TreasureChest treasureChest;

        treasureChest.containers = jsonToStringList(entry.get("containers"));
        treasureChest.treasurePool = entry.getString("treasurePool");
        treasureChest.minimumLevel = entry.getFloat("minimumLevel", 0);

        if (!m_treasurePools.contains(treasureChest.treasurePool))
          throw TreasureException(strf("No such TreasurePool '{}' for TreasureChestSet named '{}' in file '{}'", treasureChest.treasurePool, pair.first, file));

        treasureChestSet.append(treasureChest);
      }
    }
  }
}

StringList TreasureDatabase::treasurePools() const {
  return m_treasurePools.keys();
}

bool TreasureDatabase::isTreasurePool(String const& treasurePool) const {
  return m_treasurePools.contains(treasurePool);
}

StringList TreasureDatabase::treasureChestSets() const {
  return m_treasureChestSets.keys();
}

bool TreasureDatabase::isTreasureChestSet(String const& treasurePool) const {
  return m_treasureChestSets.contains(treasurePool);
}

TreasureDatabase::ItemPool::ItemPool() : allowDuplication() {}

TreasureDatabase::TreasureChest::TreasureChest() : minimumLevel() {}

List<ItemPtr> TreasureDatabase::createTreasure(String const& treasurePool, float level) const {
  return createTreasure(treasurePool, level, Random::randu64());
}

List<ItemPtr> TreasureDatabase::createTreasure(String const& treasurePool, float level, uint64_t seed) const {
  return createTreasure(treasurePool, level, seed, StringSet());
}

List<ItemPtr> TreasureDatabase::createTreasure(String const& treasurePool, float level, uint64_t seed, StringSet visitedPools) const {
  if (!m_treasurePools.contains(treasurePool))
    throw TreasureException(strf("Unknown treasure pool '{}'", treasurePool));

  if (!visitedPools.add(treasurePool))
    throw TreasureException(strf("Loop detected in treasure pool generation - set '{}' already contains '{}'", visitedPools, treasurePool));

  auto itemDatabase = Root::singleton().itemDatabase();

  List<ItemPtr> treasureItems;
  HashSet<ItemDescriptor> previousDescriptors;
  auto itemPool = m_treasurePools.get(treasurePool).get(level);

  int mix = 0;
  for (auto const& fillEntry : itemPool.fill) {
    if (fillEntry.is<String>()) {
      auto poolContents = createTreasure(fillEntry.get<String>(), level, seed + ++mix, visitedPools);
      for (auto item : poolContents) {
        if (itemPool.allowDuplication || previousDescriptors.add(item->descriptor().singular()))
          treasureItems.append(item);
      }
    } else {
      float itemLevel = level + itemPool.levelVariance[0] + staticRandomFloat(seed, ++mix, "FillLevelVariance") * (itemPool.levelVariance[1] - itemPool.levelVariance[0]);
      auto fillItem = itemDatabase->item(fillEntry.get<ItemDescriptor>(), itemLevel, seed + ++mix);
      if (itemPool.allowDuplication || previousDescriptors.add(fillItem->descriptor().singular()))
        treasureItems.append(fillItem);
    }
  }

  if (!itemPool.pool.empty()) {
    int poolRounds = itemPool.poolRounds.select(staticRandomU64(seed, "TreasurePoolRounds"));

    for (int i = 0; i < poolRounds; ++i) {
      auto poolEntry = itemPool.pool.select(staticRandomU64(seed, i, "TreasureItem"));

      if (poolEntry.is<String>()) {
        auto poolContents = createTreasure(poolEntry.get<String>(), level, staticRandomU64(seed, i, "TreasureSeedRecursion"), visitedPools);
        for (auto item : poolContents) {
          if (itemPool.allowDuplication || previousDescriptors.add(item->descriptor().singular()))
            treasureItems.append(item);
        }
      } else {
        float itemLevel = level + itemPool.levelVariance[0] + staticRandomFloat(staticRandomU64(seed, i, "TreasureLevelSeedMixer"), "PoolLevelVariance") * (itemPool.levelVariance[1] - itemPool.levelVariance[0]);
        auto roundItem = poolEntry.get<ItemDescriptor>();
        if (itemPool.allowDuplication || previousDescriptors.add(roundItem.singular()))
          treasureItems.append(itemDatabase->item(roundItem, itemLevel, seed + ++mix));
      }
    }
  }

  return treasureItems;
}

List<ItemPtr> TreasureDatabase::fillWithTreasure(
    ItemBagPtr const& itemBag, String const& treasurePool, float level) const {
  return fillWithTreasure(itemBag, treasurePool, level, Random::randu64());
}

List<ItemPtr> TreasureDatabase::fillWithTreasure(
    ItemBagPtr const& itemBag, String const& treasurePool, float level, uint64_t seed) const {
  List<ItemPtr> overflowItems;
  for (auto const& treasureItem : createTreasure(treasurePool, level, seed)) {
    if (auto overflow = itemBag->addItems(treasureItem))
      overflowItems.append(overflow);
  }

  return overflowItems;
}

ContainerObjectPtr TreasureDatabase::createTreasureChest(World* world, String const& treasureChestSet, Vec2I const& position, Direction direction) const {
  return createTreasureChest(world, treasureChestSet, position, direction, Random::randu64());
}

ContainerObjectPtr TreasureDatabase::createTreasureChest(World* world, String const& treasureChestSet, Vec2I const& position, Direction direction, uint64_t seed) const {
  auto objectDatabase = Root::singleton().objectDatabase();

  if (!m_treasureChestSets.contains(treasureChestSet))
    throw StarException(strf("Unknown treasure chest set '{}'", treasureChestSet));

  auto level = world->threatLevel();
  auto boxSet = m_treasureChestSets.get(treasureChestSet);
  eraseWhere(boxSet, [level](TreasureChest const& treasureChest) { return level < treasureChest.minimumLevel; });

  if (boxSet.empty())
    return {};

  auto const& treasureChest = staticRandomFrom(boxSet, seed, "TreasureChest");
  auto const& containerName = staticRandomFrom(treasureChest.containers, seed, "ContainerName");
  ContainerObjectPtr containerObject;
  auto parameters = JsonObject{{"treasurePools", JsonArray{treasureChest.treasurePool}}, {"treasureSeed", seed}};
  if (auto object = objectDatabase->createForPlacement(world, containerName, position, direction, parameters))
    containerObject = convert<ContainerObject>(object);

  return containerObject;
}

}
