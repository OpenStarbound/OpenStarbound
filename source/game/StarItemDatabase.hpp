#ifndef STAR_ITEM_DATABASE_HPP
#define STAR_ITEM_DATABASE_HPP

#include "StarThread.hpp"
#include "StarItemRecipe.hpp"
#include "StarItem.hpp"
#include "StarCasting.hpp"
#include "StarLuaRoot.hpp"

namespace Star {

STAR_CLASS(RecipeDatabase);
STAR_CLASS(AugmentItem);

STAR_CLASS(ItemDatabase);

STAR_EXCEPTION(ItemDatabaseException, ItemException);

enum class ItemType {
  Generic,
  LiquidItem,
  MaterialItem,
  ObjectItem,
  CurrencyItem,
  MiningTool,
  Flashlight,
  WireTool,
  BeamMiningTool,
  HarvestingTool,
  TillingTool,
  PaintingBeamTool,
  HeadArmor,
  ChestArmor,
  LegsArmor,
  BackArmor,
  Consumable,
  Blueprint,
  Codex,
  InspectionTool,
  InstrumentItem,
  GrapplingHook,
  ThrownItem,
  UnlockItem,
  ActiveItem,
  AugmentItem
};
extern EnumMap<ItemType> ItemTypeNames;

class ItemDatabase {
public:
  // During item loading, the ItemDatabase takes the ItemDescriptor and
  // produces a set of things from it:
  struct ItemConfig {
    // The relative path in assets to the base config
    String directory;

    // A possibly modified / generated config from the base config that is
    // re-constructed each time an ItemDescriptor is loaded.  Become's the
    // Item's base config.
    Json config;

    // The parameters from the ItemDescriptor, also possibly modified during
    // loading.  Since this become's the Item's parameters, it will be
    // subsequently stored with the Item as the new ItemDescriptor.
    Json parameters;
  };

  static uint64_t getCountOfItem(List<ItemPtr> const& bag, ItemDescriptor const& item, bool exactMatch = false);
  static uint64_t getCountOfItem(HashMap<ItemDescriptor, uint64_t> const& bag, ItemDescriptor const& item, bool exactMatch = false);
  static HashMap<ItemDescriptor, uint64_t> normalizeBag(List<ItemPtr> const& bag);
  static bool canMakeRecipe(ItemRecipe const& recipe, HashMap<ItemDescriptor, uint64_t> const& availableIngredients, StringMap<uint64_t> const& availableCurrencies);
  static HashSet<ItemRecipe> recipesFromSubset(HashMap<ItemDescriptor, uint64_t> const& normalizedBag, StringMap<uint64_t> const& availableCurrencies, HashSet<ItemRecipe> const& subset);
  static HashSet<ItemRecipe> recipesFromSubset(HashMap<ItemDescriptor, uint64_t> const& normalizedBag, StringMap<uint64_t> const& availableCurrencies, HashSet<ItemRecipe> const& subset, StringSet const& allowedTypes);
  static String guiFilterString(ItemPtr const& item);

  ItemDatabase();

  // Load an item based on item descriptor.  If loadItem is called with a
  // live ptr, and the ptr matches the descriptor read, then no new item is
  // constructed.  If ItemT is some other type than Item, then loadItem will
  // clear the item if the new item is not castable to it.  Returns whether
  // itemPtr was changed.  No exception will be thrown if there is an error
  // spawning the new item, it will be logged and the itemPtr will be set to a
  // default item.
  template <typename ItemT>
  bool loadItem(ItemDescriptor const& descriptor, shared_ptr<ItemT>& itemPtr) const;

  // Protects against re-instantiating an item in the same was as loadItem
  template <typename ItemT>
  bool diskLoad(Json const& diskStore, shared_ptr<ItemT>& itemPtr) const;

  ItemPtr diskLoad(Json const& diskStore) const;
  ItemPtr fromJson(Json const& spec) const;

  Json diskStore(ItemConstPtr const& itemPtr) const;

  Json toJson(ItemConstPtr const& itemPtr) const;

  bool hasItem(String const& itemName) const;
  ItemType itemType(String const& itemName) const;
  // Friendly name here can be different than the final friendly name, as it
  // can be modified by custom config or builder scripts.
  String itemFriendlyName(String const& itemName) const;
  StringSet itemTags(String const& itemName) const;

  // Generate an item config for the given itemName, parameters, level and seed.
  // Level and seed are used by generation in some item types, and may be stored as part
  // of the unique item data or may be ignored.
  ItemConfig itemConfig(String const& itemName, Json parameters, Maybe<float> level = {}, Maybe<uint64_t> seed = {}) const;

  // Generates the config for the given item descriptor and then loads the item
  // from the appropriate factory.  If there is a problem instantiating the
  // item, will return a default item instead.  If item is passed a null
  // ItemDescriptor, it will return a null pointer.
  ItemPtr item(ItemDescriptor descriptor, Maybe<float> level = {}, Maybe<uint64_t> seed = {}) const;

  bool hasRecipeToMake(ItemDescriptor const& item) const;
  bool hasRecipeToMake(ItemDescriptor const& item, StringSet const& allowedTypes) const;

  HashSet<ItemRecipe> recipesForOutputItem(String itemName) const;

  HashSet<ItemRecipe> recipesFromBagContents(List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies) const;
  HashSet<ItemRecipe> recipesFromBagContents(HashMap<ItemDescriptor, uint64_t> const& bag, StringMap<uint64_t> const& availableCurrencies) const;

  HashSet<ItemRecipe> recipesFromBagContents(List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies, StringSet const& allowedTypes) const;
  HashSet<ItemRecipe> recipesFromBagContents(HashMap<ItemDescriptor, uint64_t> const& bag, StringMap<uint64_t> const& availableCurrencies, StringSet const& allowedTypes) const;

  uint64_t maxCraftableInBag(List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies, ItemRecipe const& recipe) const;
  uint64_t maxCraftableInBag(HashMap<ItemDescriptor, uint64_t> const& bag, StringMap<uint64_t> const& availableCurrencies, ItemRecipe const& recipe) const;

  ItemRecipe getPreciseRecipeForMaterials(String const& group, List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies) const;

  ItemRecipe parseRecipe(Json const& config) const;

  HashSet<ItemRecipe> allRecipes() const;
  HashSet<ItemRecipe> allRecipes(StringSet const& types) const;

  ItemPtr applyAugment(ItemPtr const item, AugmentItem* augment) const;
  bool ageItem(ItemPtr& item, double aging) const;

  List<String> allItems() const;

private:
  struct ItemData {
    ItemType type;
    String name;
    String friendlyName;
    StringSet itemTags;
    StringList agingScripts;
    Maybe<String> assetsConfig;
    JsonObject customConfig;
    String directory;
  };

  static ItemPtr createItem(ItemType type, ItemConfig const& config);

  ItemData const& itemData(String const& name) const;
  ItemRecipe makeRecipe(List<ItemDescriptor> inputs, ItemDescriptor output, float duration, StringSet groups) const;

  void addItemSet(ItemType type, String const& extension);
  void addObjectDropItem(String const& objectPath, Json const& objectConfig);

  void scanItems();
  void addObjectItems();
  void scanRecipes();
  void addBlueprints();
  void addCodexes();

  StringMap<ItemData> m_items;
  HashSet<ItemRecipe> m_recipes;

  mutable RecursiveMutex m_luaMutex;
  LuaRootPtr m_luaRoot;
};

template <typename ItemT>
bool ItemDatabase::loadItem(ItemDescriptor const& descriptor, shared_ptr<ItemT>& itemPtr) const {
  if (descriptor.isNull()) {
    if (itemPtr) {
      itemPtr.reset();
      return true;
    }
  } else {
    if (!itemPtr || !itemPtr->matches(descriptor, true)) {
      itemPtr = as<ItemT>(item(descriptor));
      return true;
    } else if (itemPtr->count() != descriptor.count()) {
      itemPtr->setCount(descriptor.count());
      return true;
    }
  }
  return false;
}

template <typename ItemT>
bool ItemDatabase::diskLoad(Json const& diskStore, shared_ptr<ItemT>& itemPtr) const {
  try {
    return loadItem(ItemDescriptor::loadStore(diskStore), itemPtr);
  } catch (StarException const&) {
    return false;
  }
}
}

#endif
