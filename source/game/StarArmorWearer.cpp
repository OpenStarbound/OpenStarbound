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

  reset();
}

void ArmorWearer::setupHumanoidClothingDrawables(Humanoid& humanoid, bool forceNude) {
  bool nudeChanged = m_lastNude != forceNude;
  auto gender = humanoid.identity().gender;
  bool genderChanged = !m_lastGender || m_lastGender.value() != gender;
  m_lastNude = forceNude;
  m_lastGender = gender;

  bool allNeedsSync = nudeChanged || genderChanged;
  bool headNeedsSync = allNeedsSync  || m_headNeedsSync;
  bool chestNeedsSync = allNeedsSync || m_chestNeedsSync;
  bool legsNeedsSync = allNeedsSync  || m_legsNeedsSync;
  bool backNeedsSync = allNeedsSync  || m_backNeedsSync;

  bool bodyHidden = false;
  HeadArmorPtr const& headArmor = m_headCosmeticItem ? m_headCosmeticItem : m_headItem;
  if (headArmor && !forceNude) {
    if (headNeedsSync) {
      humanoid.setHeadArmorFrameset(headArmor->frameset(humanoid.identity().gender));
      humanoid.setHeadArmorDirectives(headArmor->directives());
      humanoid.setHelmetMaskDirectives(headArmor->maskDirectives());
    }
    bodyHidden = bodyHidden || headArmor->hideBody();
  } else {
    humanoid.setHeadArmorFrameset("");
    humanoid.setHelmetMaskDirectives("");
  }

  Json humanoidConfig;

  auto addHumanoidConfig = [&](Item const& item) {
    auto newConfig = item.instanceValue("humanoidConfig");
    if (newConfig.isType(Json::Type::Object)) {
      if (!humanoidConfig)
        humanoidConfig = JsonObject();
      humanoidConfig = jsonMerge(humanoidConfig, newConfig);
    }
  };

  ChestArmorPtr const& chestArmor = m_chestCosmeticItem ? m_chestCosmeticItem : m_chestItem;
  if (chestArmor && !forceNude) {
    if (chestNeedsSync) {
      humanoid.setBackSleeveFrameset(chestArmor->backSleeveFrameset(humanoid.identity().gender));
      humanoid.setFrontSleeveFrameset(chestArmor->frontSleeveFrameset(humanoid.identity().gender));
      humanoid.setChestArmorFrameset(chestArmor->bodyFrameset(humanoid.identity().gender));
      humanoid.setChestArmorDirectives(chestArmor->directives());
      addHumanoidConfig(*chestArmor);
    }
    bodyHidden = bodyHidden || chestArmor->hideBody();
  } else {
    humanoid.setBackSleeveFrameset("");
    humanoid.setFrontSleeveFrameset("");
    humanoid.setChestArmorFrameset("");
  }

  LegsArmorPtr const& legsArmor = m_legsCosmeticItem ? m_legsCosmeticItem : m_legsItem;
  if (legsArmor && !forceNude) {
    if (legsNeedsSync) {
      humanoid.setLegsArmorFrameset(legsArmor->frameset(humanoid.identity().gender));
      humanoid.setLegsArmorDirectives(legsArmor->directives());
      addHumanoidConfig(*legsArmor);
    }
    bodyHidden = bodyHidden || legsArmor->hideBody();
  } else {
    humanoid.setLegsArmorFrameset("");
  }

  BackArmorPtr const& backArmor = m_backCosmeticItem ? m_backCosmeticItem : m_backItem;
  if (backArmor && !forceNude) {
    if (backNeedsSync) {
      humanoid.setBackArmorFrameset(backArmor->frameset(humanoid.identity().gender));
      humanoid.setBackArmorDirectives(backArmor->directives());
    }
    bodyHidden = bodyHidden || backArmor->hideBody();
  } else {
    humanoid.setBackArmorFrameset("");
  }

  if (chestNeedsSync || legsNeedsSync) {
    humanoid.loadConfig(humanoidConfig);
  }

  m_headNeedsSync = m_chestNeedsSync = m_legsNeedsSync = m_backNeedsSync = false;

  humanoid.setBodyHidden(bodyHidden);
}

void ArmorWearer::effects(EffectEmitter& effectEmitter) {
  if (auto item = as<EffectSourceItem>(m_headCosmeticItem))
    effectEmitter.addEffectSources("headArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_headItem))
    effectEmitter.addEffectSources("headArmor", item->effectSources());

  if (auto item = as<EffectSourceItem>(m_chestCosmeticItem))
    effectEmitter.addEffectSources("chestArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_chestItem))
    effectEmitter.addEffectSources("chestArmor", item->effectSources());

  if (auto item = as<EffectSourceItem>(m_legsCosmeticItem))
    effectEmitter.addEffectSources("legsArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_legsItem))
    effectEmitter.addEffectSources("legsArmor", item->effectSources());

  if (auto item = as<EffectSourceItem>(m_backCosmeticItem))
    effectEmitter.addEffectSources("backArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_backItem))
    effectEmitter.addEffectSources("backArmor", item->effectSources());
}

void ArmorWearer::reset() {
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

void ArmorWearer::netElementsNeedLoad(bool) {
  auto itemDatabase = Root::singleton().itemDatabase();

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
}

}
