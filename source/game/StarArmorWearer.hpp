#ifndef STAR_ARMOR_WEARER_HPP
#define STAR_ARMOR_WEARER_HPP

#include "StarHumanoid.hpp"
#include "StarNetElementSystem.hpp"
#include "StarEffectEmitter.hpp"
#include "StarItemDescriptor.hpp"
#include "StarStatusTypes.hpp"
#include "StarLightSource.hpp"
#include "StarDamage.hpp"

namespace Star {

STAR_CLASS(ObjectItem);
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

  void setupHumanoidClothingDrawables(Humanoid& humanoid, bool forceNude) const;
  void effects(EffectEmitter& effectEmitter);
  List<PersistentStatusEffect> statusEffects() const;

  Json diskStore() const;
  void diskLoad(Json const& diskStore);

  void setHeadItem(HeadArmorPtr headItem);
  void setHeadCosmeticItem(HeadArmorPtr headCosmeticItem);
  void setChestCosmeticItem(ChestArmorPtr chestCosmeticItem);
  void setChestItem(ChestArmorPtr chestItem);
  void setLegsItem(LegsArmorPtr legsItem);
  void setLegsCosmeticItem(LegsArmorPtr legsCosmeticItem);
  void setBackItem(BackArmorPtr backItem);
  void setBackCosmeticItem(BackArmorPtr backCosmeticItem);

  HeadArmorPtr headItem() const;
  HeadArmorPtr headCosmeticItem() const;
  ChestArmorPtr chestItem() const;
  ChestArmorPtr chestCosmeticItem() const;
  LegsArmorPtr legsItem() const;
  LegsArmorPtr legsCosmeticItem() const;
  BackArmorPtr backItem() const;
  BackArmorPtr backCosmeticItem() const;

  ItemDescriptor headItemDescriptor() const;
  ItemDescriptor headCosmeticItemDescriptor() const;
  ItemDescriptor chestItemDescriptor() const;
  ItemDescriptor chestCosmeticItemDescriptor() const;
  ItemDescriptor legsItemDescriptor() const;
  ItemDescriptor legsCosmeticItemDescriptor() const;
  ItemDescriptor backItemDescriptor() const;
  ItemDescriptor backCosmeticItemDescriptor() const;

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
};

}

#endif
