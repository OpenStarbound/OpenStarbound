#include "StarArmorWearer.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarArmors.hpp"
#include "StarCasting.hpp"
#include "StarImageProcessing.hpp"
#include "StarLiquidItem.hpp"
#include "StarMaterialItem.hpp"
#include "StarObject.hpp"
#include "StarTools.hpp"
#include "StarActivatableItem.hpp"
#include "StarObjectItem.hpp"
#include "StarAssets.hpp"
#include "StarObjectDatabase.hpp"
#include "StarWorld.hpp"

namespace Star {

ArmorWearer::ArmorWearer() : m_lastNude(true) {
  addNetElement(&m_headItemDataNetState);
  addNetElement(&m_chestItemDataNetState);
  addNetElement(&m_legsItemDataNetState);
  addNetElement(&m_backItemDataNetState);
  addNetElement(&m_headCosmeticItemDataNetState);
  addNetElement(&m_chestCosmeticItemDataNetState);
  addNetElement(&m_legsCosmeticItemDataNetState);
  addNetElement(&m_backCosmeticItemDataNetState);

  m_cosmeticItems.resize(12);
  for (auto& cosmetic : m_cosmeticItems) {
    cosmetic.netState.setCompatibilityVersion(9);
    addNetElement(&cosmetic.netState);
  }

  reset();
}

void ArmorWearer::setupHumanoidClothingDrawables(Humanoid& humanoid, bool forceNude) {
  bool nudeChanged = m_lastNude != forceNude;
  auto gender = humanoid.identity().gender;
  bool genderChanged = !m_lastGender || *m_lastGender != gender;
  Direction direction = humanoid.facingDirection();
  bool dirChanged = !m_lastDirection || *m_lastDirection != direction;
  m_lastNude = forceNude;
  m_lastGender = gender;
  m_lastDirection = direction;

  auto flipped = [=](ArmorItemPtr const& armor) {
    return dirChanged && armor && armor->flipping();
  };

  bool allNeedsSync = nudeChanged || genderChanged;
  bool anyNeedsSync = allNeedsSync;
  Array<uint8_t, 4> wornCosmeticTypes;
  for (auto& cosmetic : m_cosmeticItems) {
    if (cosmetic.needsSync)
      anyNeedsSync = true;
    if (cosmetic.item) {
      if (forceNude || !cosmetic.item->visible(true))
        continue;
      if (flipped(cosmetic.item))
        anyNeedsSync = true;
      auto armorType = cosmetic.item->armorType();
      ++wornCosmeticTypes[(uint8_t)armorType];
    }
  }

  auto determineArmor = [&](ArmorType armorType, ArmorItemPtr const& base, ArmorItemPtr const& cosmetic, bool& outSync) -> ArmorItemPtr {
    ArmorItemPtr const* armor = nullptr;
    if (cosmetic && cosmetic->visible())
      armor = &cosmetic;
    else if (base && base->visible())
      armor = &base;
    uint8_t typeIndex = (uint8_t)armorType;
    uint8_t& cosmeticsPrevWorn = m_wornCosmeticTypes[typeIndex];
    uint8_t cosmeticsWorn = wornCosmeticTypes[typeIndex];
    if (armor == &base) {
      bool wearingCosmetics = cosmeticsWorn > 0;
      if (wearingCosmetics)
        armor = nullptr;
      if (cosmeticsPrevWorn > 0 != wearingCosmetics)
        outSync = true;
    }
    cosmeticsPrevWorn = cosmeticsWorn;
    return armor ? *armor : ArmorItemPtr();
  };

  HeadArmorPtr headArmor = as<HeadArmor>(determineArmor(ArmorType::Head, m_headItem, m_headCosmeticItem, m_headNeedsSync));
  ChestArmorPtr chestArmor = as<ChestArmor>(determineArmor(ArmorType::Chest, m_chestItem, m_chestCosmeticItem, m_chestNeedsSync));
  LegsArmorPtr legsArmor = as<LegsArmor>(determineArmor(ArmorType::Legs, m_legsItem, m_legsCosmeticItem, m_legsNeedsSync));
  BackArmorPtr backArmor = as<BackArmor>(determineArmor(ArmorType::Back, m_backItem, m_backCosmeticItem, m_backNeedsSync));

  bool headNeedsSync  = allNeedsSync || flipped(headArmor) || m_headNeedsSync;
  bool chestNeedsSync = allNeedsSync || flipped(chestArmor) || m_chestNeedsSync;
  bool legsNeedsSync  = allNeedsSync || flipped(legsArmor) || m_legsNeedsSync;
  bool backNeedsSync  = allNeedsSync || flipped(backArmor) || m_backNeedsSync;
  anyNeedsSync |= headNeedsSync || chestNeedsSync || legsNeedsSync || backNeedsSync;

  bool bodyFlipped = direction != Direction::Right;
  bool bodyHidden = false;
  
  Json humanoidConfig;

  auto addHumanoidConfig = [&](Item const& item) {
    auto newConfig = item.instanceValue("humanoidConfig");
    if (newConfig.isType(Json::Type::Object)) {
      if (!humanoidConfig)
        humanoidConfig = JsonObject();
      humanoidConfig = jsonMerge(humanoidConfig, newConfig);
    }
  };

  if (headArmor && !forceNude) {
    if (anyNeedsSync) {
      addHumanoidConfig(*headArmor);
      if (headNeedsSync)
        humanoid.setWearableFromHead(3, *headArmor);
    }
    bodyHidden |= headArmor->hideBody();
  } else
    humanoid.removeWearable(3);

  if (chestArmor && !forceNude) {
    if (anyNeedsSync) {
      addHumanoidConfig(*chestArmor);
      if (chestNeedsSync)
        humanoid.setWearableFromChest(2, *chestArmor);
    }
    bodyHidden |= chestArmor->hideBody();
  } else
    humanoid.removeWearable(2);

  if (legsArmor && !forceNude) {
    if (anyNeedsSync) {
      addHumanoidConfig(*legsArmor);
      if (legsNeedsSync)
        humanoid.setWearableFromLegs(1, *legsArmor);
    }
    bodyHidden |= legsArmor->hideBody();
  } else
    humanoid.removeWearable(1);

  if (backArmor && !forceNude) {
    if (anyNeedsSync) {
      addHumanoidConfig(*backArmor);
      if (backNeedsSync)
        humanoid.setWearableFromBack(0, *backArmor);
    }
    bodyHidden |= backArmor->hideBody();
  } else
    humanoid.removeWearable(0);

  if (anyNeedsSync) {
    for (size_t i = 0; i != 12; ++i) {
      size_t wearableIndex = 4 + i;
      Cosmetic& cosmetic = m_cosmeticItems[i];
      if (cosmetic.item && !forceNude && cosmetic.item->visible(true)) {
        addHumanoidConfig(*cosmetic.item);
        bodyHidden |= cosmetic.item->hideBody();
        if (allNeedsSync || cosmetic.needsSync || flipped(cosmetic.item)) {
          if (auto head = as<HeadArmor>(cosmetic.item))
            humanoid.setWearableFromHead(wearableIndex, *head);
          else if (auto chest = as<ChestArmor>(cosmetic.item))
            humanoid.setWearableFromChest(wearableIndex, *chest);
          else if (auto legs = as<LegsArmor>(cosmetic.item))
            humanoid.setWearableFromLegs(wearableIndex, *legs);
          else if (auto back = as<BackArmor>(cosmetic.item))
            humanoid.setWearableFromBack(wearableIndex, *back);
          cosmetic.needsSync = false;
        }
      } else
        humanoid.removeWearable(wearableIndex);
    }
    humanoid.loadConfig(humanoidConfig);
  }
  m_headNeedsSync = m_chestNeedsSync = m_legsNeedsSync = m_backNeedsSync = false;

  humanoid.setBodyHidden(bodyHidden);
}

void ArmorWearer::effects(EffectEmitter& effectEmitter) {
  auto gatherEffectSources = [&](ArmorType armorType, ArmorItemPtr const& base, ArmorItemPtr const& cosmetic) -> StringSet {
    uint8_t typeIndex = (uint8_t)armorType;
    if (auto item = as<EffectSourceItem>(cosmetic)) {
      return item->effectSources();
    } else if (!cosmetic && m_wornCosmeticTypes[typeIndex] == 0) {
      if (auto item = as<EffectSourceItem>(base))
        return item->effectSources();
    }
    return StringSet();
  };
 
  auto headEffects = gatherEffectSources(ArmorType::Head, m_headItem, m_headCosmeticItem);
  auto chestEffects = gatherEffectSources(ArmorType::Chest, m_chestItem, m_chestCosmeticItem);
  auto legsEffects = gatherEffectSources(ArmorType::Legs, m_legsItem, m_legsCosmeticItem);
  auto backEffects = gatherEffectSources(ArmorType::Back, m_backItem, m_backCosmeticItem);

  for (uint8_t i = 0; i != m_cosmeticItems.size(); ++i) {
    auto& cosmetic = m_cosmeticItems[i];
    if (auto item = as<EffectSourceItem>(cosmetic.item)) {
      auto armorType = cosmetic.item->armorType();
      auto newEffects = item->effectSources();
      if (armorType == ArmorType::Head)
        headEffects.addAll(newEffects);
      else if (armorType == ArmorType::Chest)
        chestEffects.addAll(newEffects);
      else if (armorType == ArmorType::Legs)
        legsEffects.addAll(newEffects);
      else if (armorType == ArmorType::Back)
        backEffects.addAll(newEffects);
    }
  }

  effectEmitter.addEffectSources("headArmor", headEffects);
  effectEmitter.addEffectSources("chestArmor", chestEffects);
  effectEmitter.addEffectSources("legsArmor", legsEffects);
  effectEmitter.addEffectSources("backArmor", backEffects);
}

void ArmorWearer::reset() {
  m_lastGender.reset();
  m_lastDirection.reset();
  m_headNeedsSync = m_chestNeedsSync = m_legsNeedsSync = m_backNeedsSync = true;
  m_headItem .reset();
  m_chestItem.reset();
  m_legsItem .reset();
  m_backItem .reset();
  m_headCosmeticItem .reset();
  m_chestCosmeticItem.reset();
  m_legsCosmeticItem .reset();
  m_backCosmeticItem .reset();
}

Json ArmorWearer::diskStore() const {
  JsonObject res;
  if (m_headItem)
    res["headItem"] = m_headItem->descriptor().diskStore();
  if (m_chestItem)
    res["chestItem"] = m_chestItem->descriptor().diskStore();
  if (m_legsItem)
    res["legsItem"] = m_legsItem->descriptor().diskStore();
  if (m_backItem)
    res["backItem"] = m_backItem->descriptor().diskStore();
  if (m_headCosmeticItem)
    res["headCosmeticItem"] = m_headCosmeticItem->descriptor().diskStore();
  if (m_chestCosmeticItem)
    res["chestCosmeticItem"] = m_chestCosmeticItem->descriptor().diskStore();
  if (m_legsCosmeticItem)
    res["legsCosmeticItem"] = m_legsCosmeticItem->descriptor().diskStore();
  if (m_backCosmeticItem)
    res["backCosmeticItem"] = m_backCosmeticItem->descriptor().diskStore();

  return res;
}

void ArmorWearer::diskLoad(Json const& diskStore) {
  auto itemDb = Root::singleton().itemDatabase();
  m_headItem = as<HeadArmor>(itemDb->diskLoad(diskStore.get("headItem", {})));
  m_chestItem = as<ChestArmor>(itemDb->diskLoad(diskStore.get("chestItem", {})));
  m_legsItem = as<LegsArmor>(itemDb->diskLoad(diskStore.get("legsItem", {})));
  m_backItem = as<BackArmor>(itemDb->diskLoad(diskStore.get("backItem", {})));
  m_headCosmeticItem = as<HeadArmor>(itemDb->diskLoad(diskStore.get("headCosmeticItem", {})));
  m_chestCosmeticItem = as<ChestArmor>(itemDb->diskLoad(diskStore.get("chestCosmeticItem", {})));
  m_legsCosmeticItem = as<LegsArmor>(itemDb->diskLoad(diskStore.get("legsCosmeticItem", {})));
  m_backCosmeticItem = as<BackArmor>(itemDb->diskLoad(diskStore.get("backCosmeticItem", {})));
}

List<PersistentStatusEffect> ArmorWearer::statusEffects() const {
  List<PersistentStatusEffect> statusEffects;
  auto addStatusFromItem = [&](ItemPtr const& item) {
    if (auto effectItem = as<StatusEffectItem>(item))
      statusEffects.appendAll(effectItem->statusEffects());
  };

  addStatusFromItem(m_headItem);
  addStatusFromItem(m_chestItem);
  addStatusFromItem(m_legsItem);
  addStatusFromItem(m_backItem);

  return statusEffects;
}

void ArmorWearer::setHeadItem(HeadArmorPtr headItem) {
  if (Item::itemsEqual(m_headItem, headItem))
    return;
  m_headItem = headItem;
  m_headNeedsSync |= !m_headCosmeticItem;
}

void ArmorWearer::setHeadCosmeticItem(HeadArmorPtr headCosmeticItem) {
  if (Item::itemsEqual(m_headCosmeticItem, headCosmeticItem))
    return;
  m_headCosmeticItem = headCosmeticItem;
  m_headNeedsSync = true;
}

void ArmorWearer::setChestItem(ChestArmorPtr chestItem) {
  if (Item::itemsEqual(m_chestItem, chestItem))
    return;
  m_chestItem = chestItem;
  m_chestNeedsSync |= !m_chestCosmeticItem;
}

void ArmorWearer::setChestCosmeticItem(ChestArmorPtr chestCosmeticItem) {
  if (Item::itemsEqual(m_chestCosmeticItem, chestCosmeticItem))
    return;
  m_chestCosmeticItem = chestCosmeticItem;
  m_chestNeedsSync = true;
}

void ArmorWearer::setLegsItem(LegsArmorPtr legsItem) {
  if (Item::itemsEqual(m_legsItem, legsItem))
    return;
  m_legsItem = legsItem;
  m_legsNeedsSync |= !m_legsCosmeticItem;
}

void ArmorWearer::setLegsCosmeticItem(LegsArmorPtr legsCosmeticItem) {
  if (Item::itemsEqual(m_legsCosmeticItem, legsCosmeticItem))
    return;
  m_legsCosmeticItem = legsCosmeticItem;
  m_legsNeedsSync = true;
}

void ArmorWearer::setBackItem(BackArmorPtr backItem) {
  if (Item::itemsEqual(m_backItem, backItem))
    return;
  m_backItem = backItem;
  m_backNeedsSync |= !m_backCosmeticItem;
}

void ArmorWearer::setBackCosmeticItem(BackArmorPtr backCosmeticItem) {
  if (Item::itemsEqual(m_backCosmeticItem, backCosmeticItem))
    return;
  m_backCosmeticItem = backCosmeticItem;
  m_backNeedsSync = true;
}

bool ArmorWearer::setCosmeticItem(uint8_t slot, ArmorItemPtr cosmeticItem) {
  if (slot >= m_cosmeticItems.size())
    return false;
  Cosmetic& cosmetic = m_cosmeticItems.at(slot);
  if (Item::itemsEqual(cosmetic.item, cosmeticItem))
    return false;
  cosmetic.item = std::move(cosmeticItem);
  cosmetic.needsStore = cosmetic.needsSync = true;
  return true;
}

HeadArmorPtr ArmorWearer::headItem() const {
  return m_headItem;
}

HeadArmorPtr ArmorWearer::headCosmeticItem() const {
  return m_headCosmeticItem;
}

ChestArmorPtr ArmorWearer::chestItem() const {
  return m_chestItem;
}

ChestArmorPtr ArmorWearer::chestCosmeticItem() const {
  return m_chestCosmeticItem;
}

LegsArmorPtr ArmorWearer::legsItem() const {
  return m_legsItem;
}

LegsArmorPtr ArmorWearer::legsCosmeticItem() const {
  return m_legsCosmeticItem;
}

BackArmorPtr ArmorWearer::backItem() const {
  return m_backItem;
}

BackArmorPtr ArmorWearer::backCosmeticItem() const {
  return m_backCosmeticItem;
}

ArmorItemPtr ArmorWearer::cosmeticItem(uint8_t slot) const {
  if (slot < m_cosmeticItems.size())
    return m_cosmeticItems[slot].item;
  return {};
}

ItemDescriptor ArmorWearer::headItemDescriptor() const {
  if (m_headItem)
    return m_headItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::headCosmeticItemDescriptor() const {
  if (m_headCosmeticItem)
    return m_headCosmeticItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::chestItemDescriptor() const {
  if (m_chestItem)
    return m_chestItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::chestCosmeticItemDescriptor() const {
  if (m_chestCosmeticItem)
    return m_chestCosmeticItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::legsItemDescriptor() const {
  if (m_legsItem)
    return m_legsItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::legsCosmeticItemDescriptor() const {
  if (m_legsCosmeticItem)
    return m_legsCosmeticItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::backItemDescriptor() const {
  if (m_backItem)
    return m_backItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::backCosmeticItemDescriptor() const {
  if (m_backCosmeticItem)
    return m_backCosmeticItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::cosmeticItemDescriptor(uint8_t slot) const {
  if (slot < m_cosmeticItems.size()) {
    auto& item = m_cosmeticItems[slot].item;
    if (item)
      return item->descriptor();
  }
  return {};
}

void ArmorWearer::netElementsNeedLoad(bool) {
  auto itemDatabase = Root::singleton().itemDatabase();

  for (auto& cosmetic : m_cosmeticItems) {
    if (cosmetic.netState.pullUpdated())
      cosmetic.needsStore |= cosmetic.needsSync |= itemDatabase->loadItem(cosmetic.netState.get(), cosmetic.item);
  }

  if (m_headCosmeticItemDataNetState.pullUpdated())
    m_headNeedsSync |= itemDatabase->loadItem(m_headCosmeticItemDataNetState.get(), m_headCosmeticItem);
  if (m_chestCosmeticItemDataNetState.pullUpdated())
    m_chestNeedsSync |= itemDatabase->loadItem(m_chestCosmeticItemDataNetState.get(), m_chestCosmeticItem);
  if (m_legsCosmeticItemDataNetState.pullUpdated())
    m_legsNeedsSync |= itemDatabase->loadItem(m_legsCosmeticItemDataNetState.get(), m_legsCosmeticItem);
  if (m_backCosmeticItemDataNetState.pullUpdated())
    m_backNeedsSync |= itemDatabase->loadItem(m_backCosmeticItemDataNetState.get(), m_backCosmeticItem);

  if (m_headItemDataNetState.pullUpdated())
    m_headNeedsSync |= itemDatabase->loadItem(m_headItemDataNetState.get(), m_headItem);
  if (m_chestItemDataNetState.pullUpdated())
    m_chestNeedsSync |= itemDatabase->loadItem(m_chestItemDataNetState.get(), m_chestItem);
  if (m_legsItemDataNetState.pullUpdated())
    m_legsNeedsSync |= itemDatabase->loadItem(m_legsItemDataNetState.get(), m_legsItem);
  if (m_backItemDataNetState.pullUpdated())
    m_backNeedsSync |= itemDatabase->loadItem(m_backItemDataNetState.get(), m_backItem);
}

void ArmorWearer::netElementsNeedStore() {
  auto itemDatabase = Root::singleton().itemDatabase();
  m_headItemDataNetState.set(itemSafeDescriptor(m_headItem));
  m_chestItemDataNetState.set(itemSafeDescriptor(m_chestItem));
  m_legsItemDataNetState.set(itemSafeDescriptor(m_legsItem));
  m_backItemDataNetState.set(itemSafeDescriptor(m_backItem));

  m_headCosmeticItemDataNetState.set(itemSafeDescriptor(m_headCosmeticItem));
  m_chestCosmeticItemDataNetState.set(itemSafeDescriptor(m_chestCosmeticItem));
  m_legsCosmeticItemDataNetState.set(itemSafeDescriptor(m_legsCosmeticItem));
  m_backCosmeticItemDataNetState.set(itemSafeDescriptor(m_backCosmeticItem));

  for (auto& cosmetic : m_cosmeticItems) {
    if (cosmetic.needsStore) {
      cosmetic.netState.set(itemSafeDescriptor(cosmetic.item));
      cosmetic.needsStore = false;
    }
  }
}

}
