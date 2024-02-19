#include "StarContainerObject.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarLexicalCast.hpp"
#include "StarTreasure.hpp"
#include "StarItemDatabase.hpp"
#include "StarItemDrop.hpp"
#include "StarLogging.hpp"
#include "StarWorld.hpp"
#include "StarEntityRendering.hpp"
#include "StarMixer.hpp"
#include "StarObjectDatabase.hpp"
#include "StarAugmentItem.hpp"

namespace Star {

ContainerObject::ContainerObject(ObjectConfigConstPtr config, Json const& parameters) : Object(config, parameters) {
  m_opened.set(0);
  m_count = 0;
  m_currentState = 0;
  m_animationFrameCooldown = 0;
  m_autoCloseCooldown = 0;

  m_crafting.set(false);
  m_craftingProgress.set(0);

  m_initialized = false;
  m_itemsUpdated = true;
  m_runUpdatedCallback = true;

  m_items = make_shared<ItemBag>(configValue("slotCount").toInt());

  m_netGroup.addNetElement(&m_opened);
  m_netGroup.addNetElement(&m_crafting);
  m_netGroup.addNetElement(&m_craftingProgress);
  m_netGroup.addNetElement(&m_itemsNetState);

  m_craftingProgress.setInterpolator(lerp<float, float>);
}

void ContainerObject::init(World* world, EntityId entityId, EntityMode mode) {
  if (mode == EntityMode::Master)
    m_interactive.set(true);

  Object::init(world, entityId, mode);
  if (mode == EntityMode::Master) {
    if (!m_initialized) {
      m_initialized = true;
      float level = world->threatLevel();
      uint64_t seed = configValue("treasureSeed", Random::randu64()).toUInt();
      level = configValue("level", level).toFloat();
      level += configValue("levelAdjustment", 0).toFloat();
      if (!configValue("initialItems").isNull()) {
        List<ItemDescriptor> items;
        for (auto const& spec : configValue("initialItems").iterateArray())
          m_items->addItems({Root::singleton().itemDatabase()->item(ItemDescriptor(spec), level, ++seed)});
      }
      if (!configValue("treasurePools").isNull()) {
        String treasurePool = Random::randValueFrom(configValue("treasurePools").toArray()).toString();
        Root::singleton().treasureDatabase()->fillWithTreasure(m_items, treasurePool, level, ++seed);
      }
      itemsUpdated();
    }
  }
}

void ContainerObject::update(float dt, uint64_t currentStep) {
  Object::update(dt, currentStep);

  if (isMaster()) {
    for (auto const& drop : take(m_lostItems))
      world()->addEntity(ItemDrop::createRandomizedDrop(drop, position()));

    if (m_crafting.get())
      tickCrafting(dt);

    if (m_autoCloseCooldown > 0) {
      m_autoCloseCooldown -= 1;
      if (m_autoCloseCooldown <= 0) {
        --m_count;
        if (m_count <= 0) {
          m_count = 0;
          m_opened.set(0);
        } else {
          m_autoCloseCooldown = configValue("autoCloseCooldown").toInt();
        }
      }
    }

    m_ageItemsTimer.update(world()->epochTime());
    if (m_ageItemsTimer.elapsedTime() > configValue("ageItemsEvery", 10).toDouble()) {
      double elapsedTime = m_ageItemsTimer.elapsedTime() * configValue("itemAgeMultiplier", 1.0f).toDouble();
      for (auto& item : m_items->items()) {
        if (Root::singleton().itemDatabase()->ageItem(item, elapsedTime))
          itemsUpdated();
      }
      m_ageItemsTimer.setElapsedTime(0.0);
    }

    if (take(m_runUpdatedCallback))
      m_scriptComponent.invoke("containerCallback");

  } else {
    setImageKey("key", toString(m_currentState));
    setImageKey("state", m_crafting.get() ? "crafting" : "idle");

    m_animationFrameCooldown -= 1;
  }
}

void ContainerObject::render(RenderCallback* renderCallback) {
  auto assets = Root::singleton().assets();

  if (m_animationFrameCooldown <= 0) {
    if (m_opened.get() != m_currentState) {
      if (m_currentState == 0) {
        // opening, or flipping to the other side
        if (!configValue("openSounds").isNull()) {
          auto audio = make_shared<AudioInstance>(*assets->audio(Random::randValueFrom(configValue("openSounds").toArray()).toString()));
          audio->setPosition(position());
          audio->setRangeMultiplier(config()->soundEffectRangeMultiplier);
          renderCallback->addAudio(std::move(audio));
        }
      }
      if (m_currentState == configValue("openFrameIndex", 2).toInt()) {
        // closing
        if (!configValue("closeSounds").isNull()) {
          auto audio = make_shared<AudioInstance>(*assets->audio(Random::randValueFrom(configValue("closeSounds").toArray()).toString()));
          audio->setPosition(position());
          audio->setRangeMultiplier(config()->soundEffectRangeMultiplier);
          renderCallback->addAudio(std::move(audio));
        }
      }
      if (m_opened.get() < m_currentState) {
        m_currentState -= 1;
      } else {
        m_currentState += 1;
      }
      m_animationFrameCooldown = configValue("frameCooldown").toInt();
    } else {
      m_animationFrameCooldown = 0;
    }
  }

  Object::render(renderCallback);
}

void ContainerObject::destroy(RenderCallback* renderCallback) {
  Object::destroy(renderCallback);
  if (isMaster()) {
    for (auto const& drop : m_items->items())
      world()->addEntity(ItemDrop::createRandomizedDrop(drop, position()));
  }
}

Maybe<Json> ContainerObject::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  auto itemDb = Root::singleton().itemDatabase();

  if (message.equalsIgnoreCase("startCrafting")) {
    startCrafting();
    return Json();

  } else if (message.equalsIgnoreCase("stopCrafting")) {
    stopCrafting();
    return Json();

  } else if (message.equalsIgnoreCase("burnContainerContents")) {
    burnContainerContents();
    return Json();

  } else if (message.equalsIgnoreCase("addItems")) {
    return itemSafeDescriptor(doAddItems(itemDb->fromJson(args.at(0)))).toJson();

  } else if (message.equalsIgnoreCase("putItems")) {
    return itemSafeDescriptor(doPutItems(args.at(0).toUInt(), itemDb->fromJson(args.at(1)))).toJson();

  } else if (message.equalsIgnoreCase("takeItems")) {
    return itemSafeDescriptor(doTakeItems(args.at(0).toUInt(), args.at(1).toUInt())).toJson();

  } else if (message.equalsIgnoreCase("swapItems")) {
    return itemSafeDescriptor(doSwapItems(args.at(0).toUInt(), itemDb->fromJson(args.at(1)), args.get(2).optBool().value(true))).toJson();

  } else if (message.equalsIgnoreCase("applyAugment")) {
    return itemSafeDescriptor(doApplyAugment(args.at(0).toUInt(), itemDb->fromJson(args.at(1)))).toJson();

  } else if (message.equalsIgnoreCase("consumeItems")) {
    return Json(doConsumeItems(ItemDescriptor(args.at(0))));

  } else if (message.equalsIgnoreCase("consumeItemsAt")) {
    return Json(doConsumeItems(args.at(0).toUInt(), args.at(1).toUInt()));

  } else if (message.equalsIgnoreCase("clearContainer")) {
    return Json(transform<JsonArray>(doClearContainer(), [](auto const& item) {
        return itemSafeDescriptor(item).toJson();
      }));

  } else {
    return Object::receiveMessage(sendingConnection, message, args);
  }
}

InteractAction ContainerObject::interact(InteractRequest const&) {
  return InteractAction(InteractActionType::OpenContainer, entityId(), Json());
}

Json ContainerObject::containerGuiConfig() const {
  return Root::singleton().assets()->json(configValue("uiConfig").toString().replace("<slots>", toString(m_items->size())));
}

String ContainerObject::containerDescription() const {
  return Object::shortDescription();
}

String ContainerObject::containerSubTitle() const {
  Json categories = Root::singleton().assets()->json("/items/categories.config:labels");
  return categories.getString(Object::category(), Object::category());
}

ItemDescriptor ContainerObject::iconItem() const {
  if (configValue("hasWindowIcon", true).toBool())
    return ItemDescriptor(name(), 1);
  return {};
}

ItemBagConstPtr ContainerObject::itemBag() const {
  return m_items;
}

void ContainerObject::containerOpen() {
  m_opened.set(configValue("openFrameIndex", 2).toInt());
  m_count++;
  m_autoCloseCooldown = configValue("autoCloseCooldown").toInt();
}

void ContainerObject::containerClose() {
  --m_count;
  if (m_count <= 0) {
    m_count = 0;
    m_opened.set(0);
  }
}

RpcPromise<ItemPtr> ContainerObject::addItems(ItemPtr const& items) {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "addItems", {itemSafeDescriptor(items).toJson()}).wrap([](Json res) {
        return Root::singleton().itemDatabase()->item(ItemDescriptor(res));
      });
  } else {
    return RpcPromise<ItemPtr>::createFulfilled(doAddItems(items));
  }
}

RpcPromise<ItemPtr> ContainerObject::putItems(size_t pos, ItemPtr const& items) {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "putItems", {itemSafeDescriptor(items).toJson()}).wrap([](Json res) {
        return Root::singleton().itemDatabase()->item(ItemDescriptor(res));
      });
  } else {
    return RpcPromise<ItemPtr>::createFulfilled(doPutItems(pos, items));
  }
}

RpcPromise<ItemPtr> ContainerObject::takeItems(size_t slot, size_t count) {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "takeItems", {slot, count}).wrap([](Json res) {
        return Root::singleton().itemDatabase()->item(ItemDescriptor(res));
      });
  } else {
    return RpcPromise<ItemPtr>::createFulfilled(doTakeItems(slot, count));
  }
}

RpcPromise<ItemPtr> ContainerObject::swapItems(size_t slot, ItemPtr const& items, bool tryCombine) {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "swapItems", {slot, itemSafeDescriptor(items).toJson(), tryCombine}).wrap([](Json res) {
        return Root::singleton().itemDatabase()->item(ItemDescriptor(res));
      });
  } else {
    return RpcPromise<ItemPtr>::createFulfilled(doSwapItems(slot, items, tryCombine));
  }
}

RpcPromise<ItemPtr> ContainerObject::applyAugment(size_t slot, ItemPtr const& augment) {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "applyAugment", {slot, itemSafeDescriptor(augment).toJson()}).wrap([](Json res) {
        return Root::singleton().itemDatabase()->item(ItemDescriptor(res));
      });
  } else {
    return RpcPromise<ItemPtr>::createFulfilled(doApplyAugment(slot, augment));
  }
}

RpcPromise<bool> ContainerObject::consumeItems(ItemDescriptor const& descriptor) {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "consumeItems", {descriptor.toJson()}).wrap([](Json res) {
        return res.toBool();
      });
  } else {
    return RpcPromise<bool>::createFulfilled(doConsumeItems(descriptor));
  }
}

RpcPromise<bool> ContainerObject::consumeItems(size_t pos, size_t count) {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "consumeItemsAt", {pos, count}).wrap([](Json res) {
        return res.toBool();
      });
  } else {
    return RpcPromise<bool>::createFulfilled(doConsumeItems(pos, count));
  }
}

RpcPromise<List<ItemPtr>> ContainerObject::clearContainer() {
  if (isSlave()) {
    return world()->sendEntityMessage(entityId(), "clearContainer", {}).wrap([](Json res) {
        auto itemDb = Root::singleton().itemDatabase();
        return res.toArray().transformed([itemDb](Json const& item) {
            return itemDb->item(ItemDescriptor(item));
          });
      });
  } else {
    return RpcPromise<List<ItemPtr>>::createFulfilled(doClearContainer());
  }
}

bool ContainerObject::isCrafting() const {
  return m_crafting.get();
}

void ContainerObject::startCrafting() {
  if (isSlave()) {
    world()->sendEntityMessage(entityId(), "startCrafting");
  } else {
    if (m_crafting.get())
      return;
    auto inputItems = m_items->items();
    inputItems.removeLast();
    m_goalRecipe = recipeForMaterials(inputItems);
    m_crafting.set(true);
    itemsUpdated();
    // tickCrafting validates
  }
}

void ContainerObject::stopCrafting() {
  if (isSlave()) {
    world()->sendEntityMessage(entityId(), "stopCrafting");
  } else {
    if (!m_crafting.get())
      return;
    m_crafting.set(false);
    m_craftingProgress.set(0);
    m_goalRecipe = ItemRecipe();
  }
}

float ContainerObject::craftingProgress() const {
  if (!isCrafting())
    return 1;
  return clamp(m_craftingProgress.get(), 0.0f, 1.0f);
}

void ContainerObject::burnContainerContents() {
  if (isSlave()) {
    world()->sendEntityMessage(entityId(), "burnContainerContents");
  } else {
    stopCrafting();
    auto level = world()->getProperty("ship.fuel", 0).toUInt();
    auto maxLevel = world()->getProperty("ship.maxFuel", 0).toUInt();
    for (auto& item : m_items->items()) {
      if (level > maxLevel)
        level = maxLevel;
      if (maxLevel == level)
        break;

      auto leftToFill = maxLevel - level;

      if (item) {
        auto fuelSingle = item->instanceValue("fuelAmount", 0).toUInt();
        if (fuelSingle > 0) {
          auto itemsToConsume = min<uint64_t>((leftToFill + fuelSingle - 1) / fuelSingle, item->count());
          level = min(maxLevel, level + fuelSingle * itemsToConsume);
          auto consumed = item->consume(itemsToConsume);
          starAssert(consumed);
          _unused(consumed);
        }
      }
    }
    itemsUpdated();
    world()->setProperty("ship.fuel", level);
  }
}

void ContainerObject::getNetStates(bool initial) {
  Object::getNetStates(initial);
  if (m_itemsNetState.pullUpdated()) {
    DataStreamBuffer ds(m_itemsNetState.get());
    m_items->read(ds);
    itemsUpdated();
  }
}

void ContainerObject::setNetStates() {
  Object::setNetStates();
  if (take(m_itemsUpdated)) {
    DataStreamBuffer ds;
    m_items->write(ds);
    m_itemsNetState.set(ds.takeData());
  }
}

void ContainerObject::readStoredData(Json const& diskStore) {
  Object::readStoredData(diskStore);

  m_opened.set(diskStore.getInt("opened"));
  m_currentState = diskStore.getInt("currentState");
  m_crafting.set(diskStore.getBool("crafting"));
  m_craftingProgress.set(diskStore.getFloat("craftingProgress"));
  m_initialized = diskStore.getBool("initialized");
  m_items = make_shared<ItemBag>(ItemBag::loadStore(diskStore.get("items")));
  m_ageItemsTimer = EpochTimer(diskStore.get("ageItemsTimer"));

  m_lostItems.appendAll(m_items->resize(configValue("slotCount").toUInt()));
}

Json ContainerObject::writeStoredData() const {
  return Object::writeStoredData().setAll({
      {"opened", m_opened.get()},
      {"currentState", m_currentState},
      {"crafting", m_crafting.get()},
      {"craftingProgress", m_craftingProgress.get()},
      {"initialized", m_initialized},
      {"items", m_items->diskStore()},
      {"ageItemsTimer", m_ageItemsTimer.toJson()}
    });
}

ItemRecipe ContainerObject::recipeForMaterials(List<ItemPtr> const& inputItems) {
  auto& root = Root::singleton();
  auto itemDatabase = root.itemDatabase();

  Json recipeGroup = configValue("recipeGroup");
  if (!recipeGroup.isNull())
    return itemDatabase->getPreciseRecipeForMaterials(recipeGroup.toString(), inputItems, {});

  Maybe<Json> result = m_scriptComponent.invoke<Json>("craftingRecipe", inputItems.filtered([](ItemPtr const& item) {
      return (bool)item;
    }).transformed([](ItemPtr const& item) {
      return item->descriptor().toJson();
    }));
  if (!result || result->isNull())
    return ItemRecipe();
  return itemDatabase->parseRecipe(*result);
}

void ContainerObject::tickCrafting(float dt) {
  if (!m_crafting.get())
    return;

  auto inputItems = m_items->items();
  inputItems.removeLast();
  auto recipe = recipeForMaterials(inputItems);
  bool craftingFail = false;
  if (recipe.isNull() || m_goalRecipe != recipe)
    craftingFail = true;
  ItemPtr targetItem = m_items->at(m_items->size() - 1);
  if (targetItem) {
    if (!targetItem->matches(m_goalRecipe.output, true))
      craftingFail = true;
    else if (targetItem->count() + m_goalRecipe.output.count() > targetItem->maxStack())
      craftingFail = true;
  }
  if (craftingFail) {
    m_crafting.set(false);
    m_craftingProgress.set(0);
    m_goalRecipe = ItemRecipe();
    return;
  }
  if (m_goalRecipe.duration > 0)
    m_craftingProgress.set(m_craftingProgress.get() + dt / m_goalRecipe.duration);
  else
    m_craftingProgress.set(1.0f);
  if (m_craftingProgress.get() >= 1.0f) {
    m_craftingProgress.set(0);
    for (auto const& input : m_goalRecipe.inputs) {
      bool consumed = m_items->consumeItems(input);
      _unused(consumed);
      starAssert(consumed);
    }
    ItemPtr overflow =
        m_items->putItems(m_items->size() - 1, Root::singleton().itemDatabase()->item(m_goalRecipe.output));
    if (overflow)
      world()->addEntity(ItemDrop::createRandomizedDrop(overflow, position()));
    itemsUpdated();
  }
}

ItemPtr ContainerObject::doAddItems(ItemPtr const& items) {
  itemsUpdated();
  return m_items->addItems(items);
}

ItemPtr ContainerObject::doPutItems(size_t slot, ItemPtr const& items) {
  itemsUpdated();
  return m_items->putItems(slot, items);
}

ItemPtr ContainerObject::doTakeItems(size_t slot, size_t count) {
  itemsUpdated();
  return m_items->takeItems(slot, count);
}

ItemPtr ContainerObject::doSwapItems(size_t slot, ItemPtr const& items, bool tryCombine) {
  itemsUpdated();
  return m_items->swapItems(slot, items, tryCombine);
}

ItemPtr ContainerObject::doApplyAugment(size_t slot, ItemPtr const& item) {
  itemsUpdated();
  if (auto augment = as<AugmentItem>(item))
    if (auto slotItem = m_items->at(slot))
      m_items->setItem(slot, augment->applyTo(slotItem));
  return item;
}

bool ContainerObject::doConsumeItems(ItemDescriptor const& descriptor) {
  if (m_items->consumeItems(descriptor)) {
    itemsUpdated();
    return true;
  }

  return false;
}

bool ContainerObject::doConsumeItems(size_t slot, size_t count) {
  if (m_items->consumeItems(slot, count)) {
    itemsUpdated();
    return true;
  }

  return false;
}

List<ItemPtr> ContainerObject::doClearContainer() {
  stopCrafting();
  List<ItemPtr> result = m_items->takeAll();
  m_items->clearItems();
  itemsUpdated();
  return result;
}

void ContainerObject::itemsUpdated() {
  m_itemsUpdated = true;
  m_runUpdatedCallback = true;
}

}
