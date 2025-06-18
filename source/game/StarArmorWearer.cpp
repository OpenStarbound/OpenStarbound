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
  for (size_t i = 0; i != m_armors.size(); ++i) {
    auto& armor = m_armors[i];
    armor.isCosmetic = i >= 4;
    if (i >= 8)
      armor.netState.setCompatibilityVersion(9);
    addNetElement(&armor.netState);
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

  auto armorVisible = [&](ArmorItemPtr const& item, bool extraCosmetic) {
    return !forceNude && item && item->visible(extraCosmetic);
  };

  bool allNeedsSync = nudeChanged || genderChanged;
  bool anyNeedsSync = allNeedsSync;
  Array<uint8_t, 4> wornCosmeticTypes;
  for (size_t i = 0; i != m_armors.size(); ++i) {
    auto& armor = m_armors[i];
    if (armor.needsSync)
      anyNeedsSync = true;
    if (allNeedsSync || (dirChanged && armor.item && armor.item->flipping()))
      anyNeedsSync = armor.needsSync = true;

    if (armorVisible(armor.item, i >= 8) && armor.isCosmetic) {
      for (auto armorType : armor.item->armorTypesToHide())
        ++wornCosmeticTypes[(uint8_t)armorType];
    }
  }

  bool bodyHidden = false;
  Json humanoidConfig;
  auto addHumanoidConfig = [&](ArmorItem const& item) {
    bodyHidden |= item.hideBody();
    auto newConfig = item.instanceValue("humanoidConfig");
    if (newConfig.isType(Json::Type::Object)) {
      if (!humanoidConfig)
        humanoidConfig = JsonObject();
      humanoidConfig = jsonMerge(humanoidConfig, newConfig);
    }
  };

  if (anyNeedsSync) {
    for (size_t i = 0; i != m_armors.size(); ++i) {
      Armor& armor = m_armors[i];
      bool allowed = true;
      if (!armorVisible(armor.item, i >= 8)) {
        allowed = false;
      } else if (!armor.isCosmetic) {
        uint8_t typeIndex = (uint8_t)armor.item->armorType();
        uint8_t prevWorn = m_wornCosmeticTypes[typeIndex];
        uint8_t curWorn = wornCosmeticTypes[typeIndex];
        if (curWorn == 0) {
          if (prevWorn != 0)
            armor.needsSync = true;
        } else {
          allowed = false;
        }
      }
      if (allowed) {
        addHumanoidConfig(*armor.item);
        if (armor.needsSync) {
          if (auto head = as<HeadArmor>(armor.item))
            humanoid.setWearableFromHead(i, *head);
          else if (auto chest = as<ChestArmor>(armor.item))
            humanoid.setWearableFromChest(i, *chest);
          else if (auto legs = as<LegsArmor>(armor.item))
            humanoid.setWearableFromLegs(i, *legs);
          else if (auto back = as<BackArmor>(armor.item))
            humanoid.setWearableFromBack(i, *back);
          armor.needsSync = false;
        }
      } else
        humanoid.removeWearable(i);
    }
    humanoid.loadConfig(humanoidConfig);
  }
  m_wornCosmeticTypes = wornCosmeticTypes;

  humanoid.setBodyHidden(bodyHidden);
}

void ArmorWearer::effects(EffectEmitter& effectEmitter) {
  StringSet headEffects, chestEffects, legsEffects, backEffects;

  for (uint8_t i = 0; i != m_armors.size(); ++i) {
    auto& armor = m_armors[i];
    if (auto item = as<EffectSourceItem>(armor.item)) {
      auto armorType = armor.item->armorType();
      if (!armor.isCosmetic && m_wornCosmeticTypes[(uint8_t)armorType] > 0)
        continue;
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
  for (auto& armor : m_armors) {
    armor.item.reset();
    armor.needsStore = armor.needsSync = true;
  }
}

Json ArmorWearer::diskStore() const {
  JsonObject res;
  auto save = [&](uint8_t slot, String const& id) {
    auto& armor = m_armors[slot];
    if (armor.item)
      res[id] = armor.item->descriptor().diskStore();
  };

  save(0, "headItem");
  save(1, "chestItem");
  save(2, "legsItem");
  save(3, "backItem");
  save(4, "headCosmeticItem");
  save(5, "chestCosmeticItem");
  save(6, "legsCosmeticItem");
  save(7, "backCosmeticItem");

  for (size_t i = 8; i != m_armors.size(); ++i)
    save(i, strf("cosmetic{}Item", i - 7));

  return res;
}

void ArmorWearer::diskLoad(Json const& diskStore) {
  auto itemDb = Root::singleton().itemDatabase();
  auto load = [&](uint8_t slot, String const& id) {
    auto& armor = m_armors[slot];
    if (auto item = as<ArmorItem>(itemDb->diskLoad(diskStore.get(id, {})))) {
      setItem(slot, item);
    }
  };

  load(0, "headItem");
  load(1, "chestItem");
  load(2, "legsItem");
  load(3, "backItem");
  load(4, "headCosmeticItem");
  load(5, "chestCosmeticItem");
  load(6, "legsCosmeticItem");
  load(7, "backCosmeticItem");

  for (size_t i = 8; i != m_armors.size(); ++i)
    load(i, strf("cosmetic{}Item", i - 7));
}

List<PersistentStatusEffect> ArmorWearer::statusEffects() const {
  List<PersistentStatusEffect> statusEffects;
  auto addStatusFromItem = [&](ItemPtr const& item) {
    if (auto effectItem = as<StatusEffectItem>(item))
      statusEffects.appendAll(effectItem->statusEffects());
  };

  for (size_t i = 0; i != 4; ++i)
    addStatusFromItem(m_armors[i].item);

  return statusEffects;
}

bool ArmorWearer::setItem(uint8_t slot, ArmorItemPtr item) {
  if (slot >= m_armors.size())
    return false;
  auto& armor = m_armors[slot];
  if (Item::itemsEqual(armor.item, item))
    return false;
  armor.item = item;
  armor.needsStore = armor.needsSync = true;
  return true;
}

void ArmorWearer::setHeadItem(HeadArmorPtr headItem) { setItem(0, headItem); }
void ArmorWearer::setChestItem(ChestArmorPtr chestItem) { setItem(1, chestItem); }
void ArmorWearer::setLegsItem(LegsArmorPtr legsItem) { setItem(2, legsItem); }
void ArmorWearer::setBackItem(BackArmorPtr backItem) { setItem(3, backItem); }
void ArmorWearer::setHeadCosmeticItem(HeadArmorPtr headCosmeticItem) { setItem(4, headCosmeticItem); }
void ArmorWearer::setChestCosmeticItem(ChestArmorPtr chestCosmeticItem) { setItem(5, chestCosmeticItem); }
void ArmorWearer::setLegsCosmeticItem(LegsArmorPtr legsCosmeticItem) { setItem(6, legsCosmeticItem); }
void ArmorWearer::setBackCosmeticItem(BackArmorPtr backCosmeticItem) { setItem(7, backCosmeticItem); }

ArmorItemPtr ArmorWearer::item(uint8_t slot) const {
  if (slot < m_armors.size())
    return m_armors[slot].item;
  return {};
}

 HeadArmorPtr ArmorWearer:: headItem() const { return as< HeadArmor>(item(0)); }
ChestArmorPtr ArmorWearer::chestItem() const { return as<ChestArmor>(item(1)); }
 LegsArmorPtr ArmorWearer:: legsItem() const { return as< LegsArmor>(item(2)); }
 BackArmorPtr ArmorWearer:: backItem() const { return as< BackArmor>(item(3)); }
 HeadArmorPtr ArmorWearer:: headCosmeticItem() const { return as< HeadArmor>(item(4)); }
ChestArmorPtr ArmorWearer::chestCosmeticItem() const { return as<ChestArmor>(item(5)); }
 LegsArmorPtr ArmorWearer:: legsCosmeticItem() const { return as< LegsArmor>(item(6)); }
 BackArmorPtr ArmorWearer:: backCosmeticItem() const { return as< BackArmor>(item(7)); }

ItemDescriptor ArmorWearer::headItemDescriptor() const {
  if (auto item = headItem())
    return item->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::chestItemDescriptor() const {
  if (auto item = chestItem())
    return item->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::legsItemDescriptor() const {
  if (auto item = legsItem())
    return item->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::backItemDescriptor() const {
  if (auto item = backItem())
    return item->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::headCosmeticItemDescriptor() const {
  if (auto item = headCosmeticItem())
    return item->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::chestCosmeticItemDescriptor() const {
  if (auto item = chestCosmeticItem())
    return item->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::legsCosmeticItemDescriptor() const {
  if (auto item = legsCosmeticItem())
    return item->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::backCosmeticItemDescriptor() const {
  if (auto item = backCosmeticItem())
    return item->descriptor();
  return {};
}

bool ArmorWearer::setCosmeticItem(uint8_t slot, ArmorItemPtr cosmeticItem) {
  return setItem(slot + 8, cosmeticItem);
}

ArmorItemPtr ArmorWearer::cosmeticItem(uint8_t slot) const {
  return item(slot + 8);
}

ItemDescriptor ArmorWearer::cosmeticItemDescriptor(uint8_t slot) const {
  if (auto item = cosmeticItem(slot + 8))
    return item->descriptor();
  return {};
}

void ArmorWearer::netElementsNeedLoad(bool) {
  auto itemDatabase = Root::singleton().itemDatabase();

  for (auto& armor : m_armors) {
    if (armor.netState.pullUpdated())
      armor.needsStore |= armor.needsSync |= itemDatabase->loadItem(armor.netState.get(), armor.item);
  }
}

void ArmorWearer::netElementsNeedStore() {
  auto itemDatabase = Root::singleton().itemDatabase();
  for (auto& armor : m_armors) {
    if (armor.needsStore) {
      armor.netState.set(itemSafeDescriptor(armor.item));
      armor.needsStore = false;
    }
  }
}

}
