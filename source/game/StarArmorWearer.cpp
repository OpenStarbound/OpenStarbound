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

ArmorWearer::ArmorWearer() : m_lastNude(true), m_needsHumanoidSync(true) {
  addNetElement(&m_headItemDataNetState);
  addNetElement(&m_chestItemDataNetState);
  addNetElement(&m_legsItemDataNetState);
  addNetElement(&m_backItemDataNetState);
  addNetElement(&m_headCosmeticItemDataNetState);
  addNetElement(&m_chestCosmeticItemDataNetState);
  addNetElement(&m_legsCosmeticItemDataNetState);
  addNetElement(&m_backCosmeticItemDataNetState);
}

void ArmorWearer::setupHumanoidClothingDrawables(Humanoid& humanoid, bool forceNude) {
  if (m_lastNude != forceNude)
    m_lastNude = forceNude;
  else if (!m_needsHumanoidSync)
    return;

  m_needsHumanoidSync = false;

  bool bodyHidden = false;
  if (m_headCosmeticItem && !forceNude) {
    humanoid.setHeadArmorFrameset(m_headCosmeticItem->frameset(humanoid.identity().gender));
    humanoid.setHeadArmorDirectives(m_headCosmeticItem->directives());
    humanoid.setHelmetMaskDirectives(m_headCosmeticItem->maskDirectives());
    bodyHidden = bodyHidden || m_headCosmeticItem->hideBody();
  } else if (m_headItem && !forceNude) {
    humanoid.setHeadArmorFrameset(m_headItem->frameset(humanoid.identity().gender));
    humanoid.setHeadArmorDirectives(m_headItem->directives());
    humanoid.setHelmetMaskDirectives(m_headItem->maskDirectives());
    bodyHidden = bodyHidden || m_headItem->hideBody();
  } else {
    humanoid.setHeadArmorFrameset("");
    humanoid.setHelmetMaskDirectives("");
  }

  if (m_chestCosmeticItem && !forceNude) {
    humanoid.setBackSleeveFrameset(m_chestCosmeticItem->backSleeveFrameset(humanoid.identity().gender));
    humanoid.setFrontSleeveFrameset(m_chestCosmeticItem->frontSleeveFrameset(humanoid.identity().gender));
    humanoid.setChestArmorFrameset(m_chestCosmeticItem->bodyFrameset(humanoid.identity().gender));
    humanoid.setChestArmorDirectives(m_chestCosmeticItem->directives());
    bodyHidden = bodyHidden || m_chestCosmeticItem->hideBody();
  } else if (m_chestItem && !forceNude) {
    humanoid.setBackSleeveFrameset(m_chestItem->backSleeveFrameset(humanoid.identity().gender));
    humanoid.setFrontSleeveFrameset(m_chestItem->frontSleeveFrameset(humanoid.identity().gender));
    humanoid.setChestArmorFrameset(m_chestItem->bodyFrameset(humanoid.identity().gender));
    humanoid.setChestArmorDirectives(m_chestItem->directives());
    bodyHidden = bodyHidden || m_chestItem->hideBody();
  } else {
    humanoid.setBackSleeveFrameset("");
    humanoid.setFrontSleeveFrameset("");
    humanoid.setChestArmorFrameset("");
  }

  if (m_legsCosmeticItem && !forceNude) {
    humanoid.setLegsArmorFrameset(m_legsCosmeticItem->frameset(humanoid.identity().gender));
    humanoid.setLegsArmorDirectives(m_legsCosmeticItem->directives());
    bodyHidden = bodyHidden || m_legsCosmeticItem->hideBody();
  } else if (m_legsItem && !forceNude) {
    humanoid.setLegsArmorFrameset(m_legsItem->frameset(humanoid.identity().gender));
    humanoid.setLegsArmorDirectives(m_legsItem->directives());
    bodyHidden = bodyHidden || m_legsItem->hideBody();
  } else {
    humanoid.setLegsArmorFrameset("");
  }

  if (m_backCosmeticItem && !forceNude) {
    humanoid.setBackArmorFrameset(m_backCosmeticItem->frameset(humanoid.identity().gender));
    humanoid.setBackArmorDirectives(m_backCosmeticItem->directives());
    bodyHidden = bodyHidden || m_backCosmeticItem->hideBody();
  } else if (m_backItem && !forceNude) {
    humanoid.setBackArmorFrameset(m_backItem->frameset(humanoid.identity().gender));
    humanoid.setBackArmorDirectives(m_backItem->directives());
    bodyHidden = bodyHidden || m_backItem->hideBody();
  } else {
    humanoid.setBackArmorFrameset("");
  }

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
  if (!Item::itemsEqual(m_headItem, headItem))
    return;
  m_headItem = headItem;
  m_needsHumanoidSync = true;
}

void ArmorWearer::setHeadCosmeticItem(HeadArmorPtr headCosmeticItem) {
  if (!Item::itemsEqual(m_headCosmeticItem, headCosmeticItem))
    return;
  m_headCosmeticItem = headCosmeticItem;
  m_needsHumanoidSync = true;
}

void ArmorWearer::setChestCosmeticItem(ChestArmorPtr chestCosmeticItem) {
  if (!Item::itemsEqual(m_chestCosmeticItem, chestCosmeticItem))
    return;
  m_chestCosmeticItem = chestCosmeticItem;
  m_needsHumanoidSync = true;
}

void ArmorWearer::setChestItem(ChestArmorPtr chestItem) {
  if (!Item::itemsEqual(m_chestItem, chestItem))
    return;
  m_chestItem = chestItem;
  m_needsHumanoidSync = true;
}

void ArmorWearer::setLegsItem(LegsArmorPtr legsItem) {
  if (!Item::itemsEqual(m_legsItem, legsItem))
    return;
  m_legsItem = legsItem;
  m_needsHumanoidSync = true;
}

void ArmorWearer::setLegsCosmeticItem(LegsArmorPtr legsCosmeticItem) {
  if (!Item::itemsEqual(m_legsCosmeticItem, legsCosmeticItem))
    return;
  m_legsCosmeticItem = legsCosmeticItem;
  m_needsHumanoidSync = true;
}

void ArmorWearer::setBackItem(BackArmorPtr backItem) {
  if (!Item::itemsEqual(m_backItem, backItem))
    return;
  m_backItem = backItem;
  m_needsHumanoidSync = true;
}

void ArmorWearer::setBackCosmeticItem(BackArmorPtr backCosmeticItem) {
  if (!Item::itemsEqual(m_backCosmeticItem, backCosmeticItem))
    return;
  m_backCosmeticItem = backCosmeticItem;
  m_needsHumanoidSync = true;
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

  bool changed = false;
  if (m_headItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_headItemDataNetState.get(), m_headItem);
  if (m_chestItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_chestItemDataNetState.get(), m_chestItem);
  if (m_legsItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_legsItemDataNetState.get(), m_legsItem);
  if (m_backItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_backItemDataNetState.get(), m_backItem);

  if (m_headCosmeticItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_headCosmeticItemDataNetState.get(), m_headCosmeticItem);
  if (m_chestCosmeticItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_chestCosmeticItemDataNetState.get(), m_chestCosmeticItem);
  if (m_legsCosmeticItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_legsCosmeticItemDataNetState.get(), m_legsCosmeticItem);
  if (m_backCosmeticItemDataNetState.pullUpdated())
    changed |= itemDatabase->loadItem(m_backCosmeticItemDataNetState.get(), m_backCosmeticItem);

  m_needsHumanoidSync = changed;
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
