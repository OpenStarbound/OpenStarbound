#include "StarItemDatabase.hpp"
#include "StarCodexDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarCasting.hpp"
#include "StarCurrency.hpp"
#include "StarConsumableItem.hpp"
#include "StarBlueprintItem.hpp"
#include "StarCodexItem.hpp"
#include "StarLiquidItem.hpp"
#include "StarMaterialItem.hpp"
#include "StarObjectItem.hpp"
#include "StarItemDrop.hpp"
#include "StarInspectionTool.hpp"
#include "StarInstrumentItem.hpp"
#include "StarThrownItem.hpp"
#include "StarUnlockItem.hpp"
#include "StarActiveItem.hpp"
#include "StarAugmentItem.hpp"
#include "StarTools.hpp"
#include "StarArmors.hpp"
#include "StarObjectDatabase.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarItemLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"

namespace Star {

EnumMap<ItemType> ItemTypeNames{
  {ItemType::Generic, "generic"},
  {ItemType::LiquidItem, "liquid"},
  {ItemType::MaterialItem, "material"},
  {ItemType::ObjectItem, "object"},
  {ItemType::CurrencyItem, "currency"},
  {ItemType::MiningTool, "miningtool"},
  {ItemType::Flashlight, "flashlight"},
  {ItemType::WireTool, "wiretool"},
  {ItemType::BeamMiningTool, "beamminingtool"},
  {ItemType::HarvestingTool, "harvestingtool"},
  {ItemType::TillingTool, "tillingtool"},
  {ItemType::PaintingBeamTool, "paintingbeamtool"},
  {ItemType::HeadArmor, "headarmor"},
  {ItemType::ChestArmor, "chestarmor"},
  {ItemType::LegsArmor, "legsarmor"},
  {ItemType::BackArmor, "backarmor"},
  {ItemType::Consumable, "consumable"},
  {ItemType::Blueprint, "blueprint"},
  {ItemType::Codex, "codex"},
  {ItemType::InspectionTool, "inspectiontool"},
  {ItemType::InstrumentItem, "instrument"},
  {ItemType::ThrownItem, "thrownitem"},
  {ItemType::UnlockItem, "unlockitem"},
  {ItemType::ActiveItem, "activeitem"},
  {ItemType::AugmentItem, "augmentitem"}
};

uint64_t ItemDatabase::getCountOfItem(List<ItemPtr> const& bag, ItemDescriptor const& item, bool exactMatch) {
  auto normalizedBag = normalizeBag(bag);
  return getCountOfItem(normalizedBag, item, exactMatch);
}

uint64_t ItemDatabase::getCountOfItem(HashMap<ItemDescriptor, uint64_t> const& bag, ItemDescriptor const& item, bool exactMatch) {
  ItemDescriptor matchItem = exactMatch ? item.singular() : ItemDescriptor(item.name(), 1);
  if (!bag.contains(matchItem)) {
    return 0;
  } else {
    return bag.get(matchItem);
  }
}

HashMap<ItemDescriptor, uint64_t> ItemDatabase::normalizeBag(List<ItemPtr> const& bag) {
  HashMap<ItemDescriptor, uint64_t> normalizedBag;
  for (auto const& item : bag) {
    if (!item)
      continue;

    normalizedBag[ItemDescriptor(item->name(), 1)] += item->count();

    if (!item->parameters().toObject().empty())
      normalizedBag[ItemDescriptor(item->name(), 1, item->parameters())] += item->count();
  }

  return normalizedBag;
}

HashSet<ItemRecipe> ItemDatabase::recipesFromSubset(HashMap<ItemDescriptor, uint64_t> const& normalizedBag, StringMap<uint64_t> const& availableCurrencies, HashSet<ItemRecipe> const& subset) {
  HashSet<ItemRecipe> res;
  for (auto const& recipe : subset) {
    // add this recipe if we can make it.
    if (canMakeRecipe(recipe, normalizedBag, availableCurrencies))
      res.add(recipe);
  }

  return res;
}

HashSet<ItemRecipe> ItemDatabase::recipesFromSubset(HashMap<ItemDescriptor, uint64_t> const& normalizedBag, StringMap<uint64_t> const& availableCurrencies,
    HashSet<ItemRecipe> const& subset, StringSet const& allowedTypes) {
  HashSet<ItemRecipe> res;
  for (auto const& recipe : subset) {
    // is it the right kind of recipe for this check ?
    if (recipe.groups.hasIntersection(allowedTypes) || allowedTypes.empty() || recipe.groups.empty()) {
      // do we have the ingredients to make it.
      if (canMakeRecipe(recipe, normalizedBag, availableCurrencies)) {
        res.add(recipe);
      }
    }
  }

  return res;
}

String ItemDatabase::guiFilterString(ItemPtr const& item) {
  return (item->name() + item->friendlyName() + item->description()).toLower().splitAny(" ,.?*\\+/|\t").join("");
}

bool ItemDatabase::canMakeRecipe(ItemRecipe const& recipe, HashMap<ItemDescriptor, uint64_t> const& availableIngredients, StringMap<uint64_t> const& availableCurrencies) {
  for (auto const& p : recipe.currencyInputs) {
    if (availableCurrencies.value(p.first, 0) < p.second)
      return false;
  }

  for (auto const& input : recipe.inputs) {
    ItemDescriptor matchInput = recipe.matchInputParameters ? input.singular() : ItemDescriptor(input.name(), 1);
    if (availableIngredients.value(matchInput) < input.count())
      return false;
  }

  return true;
}

ItemDatabase::ItemDatabase()
  : m_luaRoot(make_shared<LuaRoot>()) {
  scanItems();
  addObjectItems();
  addCodexes();
  scanRecipes();
  addBlueprints();

  auto assets = Root::singleton().assets();
  for (auto& path : assets->assetSources()) {
    auto metadata = assets->assetSourceMetadata(path);
    if (auto scripts = metadata.maybe("scripts"))
      if (auto rebuildScripts = scripts.value().optArray("itemError"))
        m_rebuildScripts.insertAllAt(0, jsonToStringList(rebuildScripts.value()));
  }
}

void ItemDatabase::cleanup() {
  {
    MutexLocker locker(m_cacheMutex);
    m_itemCache.cleanup([](ItemCacheEntry const&, ItemPtr const& item) {
      return !item.unique();
    });
  }
}

ItemPtr ItemDatabase::diskLoad(Json const& diskStore) const {
  if (diskStore) {
    return item(ItemDescriptor::loadStore(diskStore));
  } else {
    return {};
  }
}

ItemPtr ItemDatabase::fromJson(Json const& spec) const {
  return item(ItemDescriptor(spec));
}

Json ItemDatabase::diskStore(ItemConstPtr const& itemPtr) const {
  if (itemPtr)
    return itemPtr->descriptor().diskStore();
  else
    return Json();
}

Json ItemDatabase::toJson(ItemConstPtr const& itemPtr) const {
  if (itemPtr)
    return itemPtr->descriptor().toJson();
  else
    return Json();
}

bool ItemDatabase::hasItem(String const& itemName) const {
  return m_items.contains(itemName);
}

ItemType ItemDatabase::itemType(String const& itemName) const {
  return itemData(itemName).type;
}

String ItemDatabase::itemFriendlyName(String const& itemName) const {
  return itemData(itemName).friendlyName;
}

StringSet ItemDatabase::itemTags(String const& itemName) const {
  return itemData(itemName).itemTags;
}

ItemDatabase::ItemConfig ItemDatabase::itemConfig(String const& itemName, Json parameters, Maybe<float> level, Maybe<uint64_t> seed) const {
  auto const& data = itemData(itemName);

  ItemConfig itemConfig;
  if (data.assetsConfig)
    itemConfig.config = Root::singleton().assets()->json(*data.assetsConfig);
  itemConfig.directory = data.directory;
  itemConfig.config = jsonMerge(itemConfig.config, data.customConfig);
  itemConfig.parameters = parameters;

  if (auto builder = itemConfig.config.optString("builder")) {
    RecursiveMutexLocker locker(m_luaMutex);
    auto context = m_luaRoot->createContext(*builder);
    context.setCallbacks("root", LuaBindings::makeRootCallbacks());
    context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
    luaTie(itemConfig.config, itemConfig.parameters) = context.invokePath<LuaTupleReturn<Json, Json>>(
        "build", itemConfig.directory, itemConfig.config, itemConfig.parameters, level, seed);
  }

  return itemConfig;
}

Maybe<String> ItemDatabase::itemFile(String const& itemName) const {
  if (!hasItem(itemName)) {
    return {};
  }
  auto const& data = itemData(itemName);
  return data.directory + data.filename;
}

ItemPtr ItemDatabase::itemShared(ItemDescriptor descriptor, Maybe<float> level, Maybe<uint64_t> seed) const {
  if (!descriptor)
    return {};

  ItemCacheEntry entry{ descriptor, level, seed };
  MutexLocker locker(m_cacheMutex);
  if (ItemPtr* cached = m_itemCache.ptr(entry))
    return *cached;
  else {
    locker.unlock();

    ItemPtr item = tryCreateItem(descriptor, level, seed);
    get<2>(entry) = item->parameters().optUInt("seed"); // Seed could've been changed by the buildscript

    locker.lock();
    return m_itemCache.get(entry, [&](ItemCacheEntry const&) -> ItemPtr { return std::move(item); });
  }
}

ItemPtr ItemDatabase::item(ItemDescriptor descriptor, Maybe<float> level, Maybe<uint64_t> seed, bool ignoreInvalid) const {
  if (!descriptor)
    return {};
  else
    return tryCreateItem(descriptor, level, seed, ignoreInvalid);
}

bool ItemDatabase::hasRecipeToMake(ItemDescriptor const& item) const {
  auto si = item.singular();
  for (auto const& recipe : m_recipes)
    if (recipe.output.singular() == si)
      return true;
  return false;
}

bool ItemDatabase::hasRecipeToMake(ItemDescriptor const& item, StringSet const& allowedTypes) const {
  auto si = item.singular();
  for (auto const& recipe : m_recipes)
    if (recipe.output.singular() == si)
      for (auto allowedType : allowedTypes)
        if (recipe.groups.contains(allowedType))
          return true;
  return false;
}

HashSet<ItemRecipe> ItemDatabase::recipesForOutputItem(String itemName) const {
  HashSet<ItemRecipe> result;
  for (auto const& recipe : m_recipes)
    if (recipe.output.name() == itemName)
      result.add(recipe);
  return result;
}

HashSet<ItemRecipe> ItemDatabase::recipesFromBagContents(List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies) const {
  auto normalizedBag = normalizeBag(bag);
  return recipesFromBagContents(normalizedBag, availableCurrencies);
}

HashSet<ItemRecipe> ItemDatabase::recipesFromBagContents(HashMap<ItemDescriptor, uint64_t> const& bag, StringMap<uint64_t> const& availableCurrencies) const {
  return recipesFromSubset(bag, availableCurrencies, m_recipes);
}

HashSet<ItemRecipe> ItemDatabase::recipesFromBagContents(List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies, StringSet const& allowedTypes) const {
  auto normalizedBag = normalizeBag(bag);
  return recipesFromBagContents(normalizedBag, availableCurrencies, allowedTypes);
}

HashSet<ItemRecipe> ItemDatabase::recipesFromBagContents(HashMap<ItemDescriptor, uint64_t> const& bag, StringMap<uint64_t> const& availableCurrencies, StringSet const& allowedTypes) const {
  return recipesFromSubset(bag, availableCurrencies, m_recipes, allowedTypes);
}

uint64_t ItemDatabase::maxCraftableInBag(List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies, ItemRecipe const& recipe) const {
  auto normalizedBag = normalizeBag(bag);

  return maxCraftableInBag(normalizedBag, availableCurrencies, recipe);
}

uint64_t ItemDatabase::maxCraftableInBag(HashMap<ItemDescriptor, uint64_t> const& bag, StringMap<uint64_t> const& availableCurrencies, ItemRecipe const& recipe) const {
  uint64_t res = highest<uint64_t>();

  for (auto const& p : recipe.currencyInputs) {
    uint64_t available = availableCurrencies.value(p.first, 0);
    if (available == 0)
      return 0;
    else if (p.second > 0)
      res = min(available / p.second, res);
  }

  for (auto const& input : recipe.inputs) {
    if (!bag.contains(input.singular()))
      return 0;
    else if (input.count() > 0)
      res = min(bag.get(input.singular()) / input.count(), res);
  }

  return res;
}

ItemRecipe ItemDatabase::getPreciseRecipeForMaterials(String const& group, List<ItemPtr> const& bag, StringMap<uint64_t> const& availableCurrencies) const {
  // picks the recipe that:
  // * can be crafted (duh)
  // * uses all the input material types
  // * uses the most materials (if recipes exist with the same input materials)

  auto options = recipesFromBagContents(bag, availableCurrencies);
  ItemRecipe result;
  int ingredientsCount = 0;
  for (auto const& recipe : options) {
    if (!recipe.groups.contains(group))
      continue;
    bool usesAllItemTypes = true;
    for (auto const& item : bag) {
      bool match = false;
      for (auto const& input : recipe.inputs)
        if (item->matches(input, recipe.matchInputParameters))
          match = true;
      if (!match)
        usesAllItemTypes = false;
    }
    if (!usesAllItemTypes)
      continue;
    int count = 0;
    for (auto const& input : recipe.inputs)
      count += input.count();
    if (count > ingredientsCount)
      result = recipe;
  }
  return result;
}

ItemRecipe ItemDatabase::parseRecipe(Json const& config) const {
  ItemRecipe res;
  try {
    res.currencyInputs = jsonToMapV<StringMap<uint64_t>>(config.get("currencyInputs", JsonObject()), mem_fn(&Json::toUInt));

    // parse currency items into currency inputs
    for (auto input : config.getArray("input")) {
      auto id = ItemDescriptor(input);
      if (itemType(id.name()) == ItemType::CurrencyItem) {
        auto currencyItem = as<CurrencyItem>(itemShared(id));
        res.currencyInputs[currencyItem->currencyType()] += currencyItem->totalValue();
      } else {
        res.inputs.push_back(id);
      }
    }

    res.output = ItemDescriptor(config.get("output"));
    res.duration = config.getFloat("duration", Root::singleton().assets()->json("/items/defaultParameters.config:defaultCraftDuration").toFloat());
    res.groups = StringSet::from(jsonToStringList(config.get("groups", JsonArray())));
    if (auto item = ItemDatabase::itemShared(res.output)) {
      res.outputRarity = item->rarity();
      res.guiFilterString = guiFilterString(item);
    }
    res.collectables = jsonToMapV<StringMap<String>>(config.get("collectables", JsonObject()), mem_fn(&Json::toString));
    res.matchInputParameters = config.getBool("matchInputParameters", false);

  } catch (JsonException const& e) {
    throw RecipeException(strf("Recipe missing required ingredient: {}", outputException(e, false)));
  }

  return res;
}

HashSet<ItemRecipe> const& ItemDatabase::allRecipes() const {
  return m_recipes;
}

HashSet<ItemRecipe> ItemDatabase::allRecipes(StringSet const& types) const {
  HashSet<ItemRecipe> res;
  for (auto const& i : m_recipes) {
    if (i.groups.hasIntersection(types))
      res.add(i);
  }
  return res;
}

ItemPtr ItemDatabase::applyAugment(ItemPtr const item, AugmentItem* augment) const {
  if (item) {
    RecursiveMutexLocker locker(m_luaMutex);
    LuaBaseComponent script;
    script.setLuaRoot(m_luaRoot);
    script.setScripts(augment->augmentScripts());
    script.addCallbacks("item", LuaBindings::makeItemCallbacks(augment));
    script.addCallbacks("config", LuaBindings::makeConfigCallbacks(bind(&Item::instanceValue, augment, _1, _2)));
    script.init();
    auto luaResult = script.invoke<LuaTupleReturn<Json, Maybe<uint64_t>>>("apply", item->descriptor().toJson());
    script.uninit();
    locker.unlock();

    if (luaResult) {
      if (!get<0>(*luaResult).isNull()) {
        augment->take(get<1>(*luaResult).value(1));
        return ItemDatabase::item(ItemDescriptor(get<0>(*luaResult)));
      }
    }
  }

  return item;
}

bool ItemDatabase::ageItem(ItemPtr& item, double aging) const {
  if (!item)
    return false;

  auto const& itemData = ItemDatabase::itemData(item->name());
  if (itemData.agingScripts.empty())
    return false;

  ItemDescriptor original = item->descriptor();

  RecursiveMutexLocker locker(m_luaMutex);
  LuaBaseComponent script;
  script.setLuaRoot(m_luaRoot);
  script.setScripts(itemData.agingScripts);
  script.init();
  auto aged = script.invoke<Json>("ageItem", original.toJson(), aging).apply(construct<ItemDescriptor>());
  script.uninit();
  locker.unlock();

  if (aged && *aged != original) {
    item = ItemDatabase::item(*aged);
    return true;
  }

  return false;
}

List<String> ItemDatabase::allItems() const {
  return m_items.keys();
}

ItemPtr ItemDatabase::createItem(ItemType type, ItemConfig const& config) {
  if (type == ItemType::Generic) {
    return make_shared<GenericItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::LiquidItem) {
    return make_shared<LiquidItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::MaterialItem) {
    return make_shared<MaterialItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::ObjectItem) {
    return make_shared<ObjectItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::CurrencyItem) {
    return make_shared<CurrencyItem>(config.config, config.directory);
  } else if (type == ItemType::MiningTool) {
    return make_shared<MiningTool>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::Flashlight) {
    return make_shared<Flashlight>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::WireTool) {
    return make_shared<WireTool>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::BeamMiningTool) {
    return make_shared<BeamMiningTool>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::PaintingBeamTool) {
    return make_shared<PaintingBeamTool>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::TillingTool) {
    return make_shared<TillingTool>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::HarvestingTool) {
    return make_shared<HarvestingTool>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::HeadArmor) {
    return make_shared<HeadArmor>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::ChestArmor) {
    return make_shared<ChestArmor>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::LegsArmor) {
    return make_shared<LegsArmor>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::BackArmor) {
    return make_shared<BackArmor>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::Consumable) {
    return make_shared<ConsumableItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::Blueprint) {
    return make_shared<BlueprintItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::Codex) {
    return make_shared<CodexItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::InspectionTool) {
    return make_shared<InspectionTool>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::InstrumentItem) {
    return make_shared<InstrumentItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::ThrownItem) {
    return make_shared<ThrownItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::UnlockItem) {
    return make_shared<UnlockItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::ActiveItem) {
    return make_shared<ActiveItem>(config.config, config.directory, config.parameters);
  } else if (type == ItemType::AugmentItem) {
    return make_shared<AugmentItem>(config.config, config.directory, config.parameters);
  } else {
    throw ItemException(strf("Unknown item type {}", (int)type));
  }
}

ItemPtr ItemDatabase::tryCreateItem(ItemDescriptor const& descriptor, Maybe<float> level, Maybe<uint64_t> seed, bool ignoreInvalid) const {
  ItemPtr result;
  String name = descriptor.name();
  Json parameters = descriptor.parameters();

  try {
    if ((name == "perfectlygenericitem") && parameters.contains("genericItemStorage")) {
      Json storage = parameters.get("genericItemStorage");
      name = storage.getString("name");
      parameters = storage.get("parameters");
    }
	  result = createItem(m_items.get(name).type, itemConfig(name, parameters, level, seed));
    result->setCount(descriptor.count());
    return result;
  }
  catch (std::exception const& e) {
    if (!ignoreInvalid) {
      auto lastException = e;
      Json newDiskStore = descriptor.toJson();
      for (auto script : m_rebuildScripts) {
        RecursiveMutexLocker locker(m_luaMutex);
        auto context = m_luaRoot->createContext(script);
        context.setCallbacks("root", LuaBindings::makeRootCallbacks());
        context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
        Json returnedDiskStore = context.invokePath<Json>("error", newDiskStore, strf("{}", outputException(lastException, false)));
        if (returnedDiskStore != newDiskStore) {
          newDiskStore = returnedDiskStore;
          try {
            ItemDescriptor newDescriptor(newDiskStore);
            result = createItem(m_items.get(newDescriptor.name()).type, itemConfig(newDescriptor.name(), newDescriptor.parameters(), level, seed));
            result->setCount(descriptor.count());
            return result;
          } catch (std::exception const& e) {
            lastException = e;
          }
        }
      }
      throw lastException;
    } else {
      throw e;
    }
  }
}

ItemDatabase::ItemData const& ItemDatabase::itemData(String const& name) const {
  if (auto p = m_items.ptr(name))
    return *p;
  throw ItemException::format("No such item '{}'", name);
}

ItemRecipe ItemDatabase::makeRecipe(List<ItemDescriptor> inputs, ItemDescriptor output, float duration, StringSet groups) const {
  ItemRecipe res;
  res.inputs = std::move(inputs);
  res.output = std::move(output);
  res.duration = duration;
  res.groups = std::move(groups);
  if (auto item = ItemDatabase::itemShared(res.output)) {
    res.outputRarity = item->rarity();
    res.guiFilterString = guiFilterString(item);
  }
  return res;
}

void ItemDatabase::addItemSet(ItemType type, String const& extension) {
  auto assets = Root::singleton().assets();
  for (auto& file : assets->scanExtension(extension)) {
    ItemData data;
    try {
      auto config = assets->json(file);
      data.type = type;
      data.assetsConfig = file;
      data.name = config.get("itemName").toString();
      data.friendlyName = config.getString("shortdescription", {});
      data.itemTags = config.opt("itemTags").apply(jsonToStringSet).value();
      data.agingScripts = config.opt("itemAgingScripts").apply(jsonToStringList).value();
      data.directory = AssetPath::directory(file);
      data.filename = AssetPath::filename(file);

      data.agingScripts = data.agingScripts.transformed(bind(&AssetPath::relativeTo, data.directory, _1));
    } catch (std::exception const& e) {
      throw ItemException(strf("Could not load item asset {}", file), e);
    }

    if (m_items.contains(data.name))
      throw ItemException(strf("Duplicate item name '{}' found", data.name));

    m_items[data.name] = data;
  }
}

void ItemDatabase::addObjectDropItem(String const& objectPath, Json const& objectConfig) {
  auto assets = Root::singleton().assets();

  ItemData data;
  data.type = ItemType::ObjectItem;
  data.name = objectConfig.get("objectName").toString();
  data.friendlyName = objectConfig.getString("shortdescription", {});
  data.itemTags = objectConfig.opt("itemTags").apply(jsonToStringSet).value();
  data.agingScripts = objectConfig.opt("itemAgingScripts").apply(jsonToStringList).value();
  data.directory = AssetPath::directory(objectPath);
  data.filename = AssetPath::filename(objectPath);
  JsonObject customConfig = objectConfig.toObject();
  if (!customConfig.contains("inventoryIcon")) {
    customConfig["inventoryIcon"] = assets->json("/objects/defaultParameters.config:missingIcon");
    Logger::warn(strf("Missing inventoryIcon for {}, using default", data.name).c_str());
  }
  customConfig["itemName"] = data.name;
  if (!customConfig.contains("tooltipKind"))
    customConfig["tooltipKind"] = "object";

  if (!customConfig.contains("printable"))
    customConfig["printable"] = customConfig.contains("price");

  // Don't inherit object scripts. this is kind of a crappy solution to prevent
  // ObjectItems (which are firable and therefore scripted) from trying to
  // execute scripts intended for objects
  customConfig.remove("scripts");

  data.customConfig = std::move(customConfig);

  if (m_items.contains(data.name))
    throw ItemException(strf("Object drop '{}' shares name with existing item", data.name));

  m_items[data.name] = std::move(data);
}

void ItemDatabase::scanItems() {
  auto assets = Root::singleton().assets();

  List<std::pair<ItemType, String>> itemSets;
  auto scanItemType = [&itemSets, assets](ItemType type, String const& extension) {
    itemSets.append(make_pair(type, extension));
    assets->queueJsons(assets->scanExtension(extension));
  };

  scanItemType(ItemType::Generic, "item");
  scanItemType(ItemType::LiquidItem, "liqitem");
  scanItemType(ItemType::MaterialItem, "matitem");
  scanItemType(ItemType::MiningTool, "miningtool");
  scanItemType(ItemType::Flashlight, "flashlight");
  scanItemType(ItemType::WireTool, "wiretool");
  scanItemType(ItemType::BeamMiningTool, "beamaxe");
  scanItemType(ItemType::TillingTool, "tillingtool");
  scanItemType(ItemType::PaintingBeamTool, "painttool");
  scanItemType(ItemType::HarvestingTool, "harvestingtool");
  scanItemType(ItemType::HeadArmor, "head");
  scanItemType(ItemType::ChestArmor, "chest");
  scanItemType(ItemType::LegsArmor, "legs");
  scanItemType(ItemType::BackArmor, "back");
  scanItemType(ItemType::CurrencyItem, "currency");
  scanItemType(ItemType::Consumable, "consumable");
  scanItemType(ItemType::Blueprint, "blueprint");
  scanItemType(ItemType::InspectionTool, "inspectiontool");
  scanItemType(ItemType::InstrumentItem, "instrument");
  scanItemType(ItemType::ThrownItem, "thrownitem");
  scanItemType(ItemType::UnlockItem, "unlock");
  scanItemType(ItemType::ActiveItem, "activeitem");
  scanItemType(ItemType::AugmentItem, "augment");

  for (auto const& itemset : itemSets)
    addItemSet(itemset.first, itemset.second);
}

void ItemDatabase::addObjectItems() {
  auto objectDatabase = Root::singleton().objectDatabase();

  for (auto const& objectName : objectDatabase->allObjects()) {
    auto objectConfig = objectDatabase->getConfig(objectName);

    if (objectConfig->hasObjectItem)
      addObjectDropItem(objectConfig->path, objectConfig->config);
  }
}

void ItemDatabase::scanRecipes() {
  auto assets = Root::singleton().assets();

  auto& files = assets->scanExtension("recipe");
  assets->queueJsons(files);
  for (auto& file : files) {
    try {
      m_recipes.add(parseRecipe(assets->json(file)));
    } catch (std::exception const& e) {
      Logger::error("Could not load recipe {}: {}", file, outputException(e, false));
    }
  }
}

void ItemDatabase::addBlueprints() {
  auto assets = Root::singleton().assets();

  for (auto const& recipe : m_recipes) {
    auto baseDesc = recipe.output;
    auto baseItem = itemShared(baseDesc);

    String blueprintName = strf("{}-recipe", baseItem->name());
    if (m_items.contains(blueprintName))
      continue;

    try {
      ItemData blueprintData;

      blueprintData.type = ItemType::Blueprint;
      JsonObject configInfo;
      configInfo["recipe"] = baseDesc.singular().toJson();

      String description = assets->json("/blueprint.config:description").toString();
      description = description.replace("<item>", baseItem->friendlyName());
      configInfo["description"] = Json(description);

      String shortDesc = assets->json("/blueprint.config:shortdescription").toString();
      shortDesc = shortDesc.replace("<item>", baseItem->friendlyName());
      configInfo["shortdescription"] = Json(shortDesc);

      configInfo["category"] = assets->json("/blueprint.config:category").toString();

      blueprintData.name = blueprintName;
      blueprintData.friendlyName = shortDesc;
      configInfo["itemName"] = blueprintData.name;

      if (baseItem->instanceValue("inventoryIcon", false))
        configInfo["inventoryIcon"] = baseItem->instanceValue("inventoryIcon");

      configInfo["rarity"] = RarityNames.getRight(baseItem->rarity());

      configInfo["price"] = baseItem->price();

      blueprintData.customConfig = std::move(configInfo);
      blueprintData.directory = itemData(baseDesc.name()).directory;

      m_items[blueprintData.name] = blueprintData;
    } catch (std::exception const& e) {
      Logger::error("Could not create blueprint item from recipe: {}", outputException(e, false));
    }
  }
}

void ItemDatabase::addCodexes() {
  auto assets = Root::singleton().assets();
  auto codexConfig = assets->json("/codex.config");

  auto codexDatabase = Root::singleton().codexDatabase();
  for (auto const& codexPair : codexDatabase->codexes()) {
    String codexItemName = strf("{}-codex", codexPair.second->id());
    if (m_items.contains(codexItemName)) {
      Logger::warn("Couldn't create codex item {} because an item with that name is already defined", codexItemName);
      continue;
    }

    try {
      ItemData codexItemData;

      codexItemData.type = ItemType::Codex;
      codexItemData.name = codexItemName;
      codexItemData.friendlyName = codexPair.second->title();
      codexItemData.directory = codexPair.second->directory();
      codexItemData.filename = codexPair.second->filename();
      auto customConfig = jsonMerge(codexConfig.get("defaultItemConfig"), codexPair.second->itemConfig()).toObject();
      customConfig["itemName"] = codexItemName;
      customConfig["codexId"] = codexPair.second->id();
      customConfig["shortdescription"] = codexPair.second->title();
      customConfig["description"] = codexPair.second->description();
      customConfig["codexIcon"] = codexPair.second->icon();
      codexItemData.customConfig = customConfig;

      m_items[codexItemName] = codexItemData;
    } catch (std::exception const& e) {
      Logger::error("Could not create item for codex {}: {}", codexPair.second->id(), outputException(e, false));
    }
  }
}

}
