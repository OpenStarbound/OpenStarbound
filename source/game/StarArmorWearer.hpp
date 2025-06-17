#pragma once

#include "StarHumanoid.hpp"
#include "StarNetElementSystem.hpp"
#include "StarEffectEmitter.hpp"
#include "StarItemDescriptor.hpp"
#include "StarStatusTypes.hpp"
#include "StarLightSource.hpp"
#include "StarDamage.hpp"

namespace Star {

STAR_CLASS(ObjectItem);
STAR_CLASS(ArmorItem);
STAR_CLASS(HeadArmor);
STAR_CLASS(ChestArmor);
STAR_CLASS(LegsArmor);
STAR_CLASS(BackArmor);
STAR_CLASS(ToolUserEntity);
STAR_CLASS(Item);
STAR_CLASS(World);

STAR_CLASS(ArmorWearer);

class ArmorWearer : public NetElementSyncGroup {
public:
  ArmorWearer();

  void setupHumanoidClothingDrawables(Humanoid& humanoid, bool forceNude);
  void effects(EffectEmitter& effectEmitter);
  List<PersistentStatusEffect> statusEffects() const;

  void reset();

  Json diskStore() const;
  void diskLoad(Json const& diskStore);

  void setHeadItem(HeadArmorPtr headItem);
  void setHeadCosmeticItem(HeadArmorPtr headCosmeticItem);
  void setChestItem(ChestArmorPtr chestItem);
  void setChestCosmeticItem(ChestArmorPtr chestCosmeticItem);
  void setLegsItem(LegsArmorPtr legsItem);
  void setLegsCosmeticItem(LegsArmorPtr legsCosmeticItem);
  void setBackItem(BackArmorPtr backItem);
  void setBackCosmeticItem(BackArmorPtr backCosmeticItem);
  bool setCosmeticItem(uint8_t slot, ArmorItemPtr cosmeticItem);

  HeadArmorPtr headItem() const;
  HeadArmorPtr headCosmeticItem() const;
  ChestArmorPtr chestItem() const;
  ChestArmorPtr chestCosmeticItem() const;
  LegsArmorPtr legsItem() const;
  LegsArmorPtr legsCosmeticItem() const;
  BackArmorPtr backItem() const;
  BackArmorPtr backCosmeticItem() const;
  ArmorItemPtr cosmeticItem(uint8_t slot) const;

  ItemDescriptor headItemDescriptor() const;
  ItemDescriptor headCosmeticItemDescriptor() const;
  ItemDescriptor chestItemDescriptor() const;
  ItemDescriptor chestCosmeticItemDescriptor() const;
  ItemDescriptor legsItemDescriptor() const;
  ItemDescriptor legsCosmeticItemDescriptor() const;
  ItemDescriptor backItemDescriptor() const;
  ItemDescriptor backCosmeticItemDescriptor() const;
  ItemDescriptor cosmeticItemDescriptor(uint8_t slot) const;

private:
  void netElementsNeedLoad(bool full) override;
  void netElementsNeedStore() override;

  HeadArmorPtr m_headItem;
  ChestArmorPtr m_chestItem;
  LegsArmorPtr m_legsItem;
  BackArmorPtr m_backItem;

  HeadArmorPtr m_headCosmeticItem;
  ChestArmorPtr m_chestCosmeticItem;
  LegsArmorPtr m_legsCosmeticItem;
  BackArmorPtr m_backCosmeticItem;

  NetElementData<ItemDescriptor> m_headItemDataNetState;
  NetElementData<ItemDescriptor> m_chestItemDataNetState;
  NetElementData<ItemDescriptor> m_legsItemDataNetState;
  NetElementData<ItemDescriptor> m_backItemDataNetState;

  NetElementData<ItemDescriptor> m_headCosmeticItemDataNetState;
  NetElementData<ItemDescriptor> m_chestCosmeticItemDataNetState;
  NetElementData<ItemDescriptor> m_legsCosmeticItemDataNetState;
  NetElementData<ItemDescriptor> m_backCosmeticItemDataNetState;

  struct Cosmetic {
    ArmorItemPtr item;
    bool needsSync = true;
    bool needsStore = true;
    NetElementData<ItemDescriptor> netState;
  };
  List<Cosmetic> m_cosmeticItems;
  Array<uint8_t, 4> m_wornCosmeticTypes;
  // only works under the assumption that this ArmorWearer
  // will only ever touch one Humanoid (which is true!)
  Maybe<Gender> m_lastGender;
  Maybe<Direction> m_lastDirection;
  bool m_lastNude;
  bool m_headNeedsSync;
  bool m_chestNeedsSync;
  bool m_legsNeedsSync;
  bool m_backNeedsSync;
  Direction facingDirection;
};

}
