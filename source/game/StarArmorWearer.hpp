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

  // returns true if movement parameters changed
  bool setupHumanoid(Humanoid& humanoid, bool forceNude);
  void effects(EffectEmitter& effectEmitter);
  List<PersistentStatusEffect> statusEffects() const;

  void reset();

  Json diskStore() const;
  void diskLoad(Json const& diskStore);

  bool setItem(uint8_t slot, ArmorItemPtr item);
  void setHeadItem(HeadArmorPtr headItem);
  void setChestItem(ChestArmorPtr chestItem);
  void setLegsItem(LegsArmorPtr legsItem);
  void setBackItem(BackArmorPtr backItem);
  void setHeadCosmeticItem(HeadArmorPtr headCosmeticItem);
  void setChestCosmeticItem(ChestArmorPtr chestCosmeticItem);
  void setLegsCosmeticItem(LegsArmorPtr legsCosmeticItem);
  void setBackCosmeticItem(BackArmorPtr backCosmeticItem);

  ArmorItemPtr item(uint8_t slot) const;
  HeadArmorPtr headItem() const;
  ChestArmorPtr chestItem() const;
  LegsArmorPtr legsItem() const;
  BackArmorPtr backItem() const;
  HeadArmorPtr headCosmeticItem() const;
  ChestArmorPtr chestCosmeticItem() const;
  LegsArmorPtr legsCosmeticItem() const;
  BackArmorPtr backCosmeticItem() const;

  ItemDescriptor itemDescriptor(uint8_t slot) const;
  ItemDescriptor headItemDescriptor() const;
  ItemDescriptor chestItemDescriptor() const;
  ItemDescriptor legsItemDescriptor() const;
  ItemDescriptor backItemDescriptor() const;
  ItemDescriptor headCosmeticItemDescriptor() const;
  ItemDescriptor chestCosmeticItemDescriptor() const;
  ItemDescriptor legsCosmeticItemDescriptor() const;
  ItemDescriptor backCosmeticItemDescriptor() const;

  // slot is automatically offset
  bool setCosmeticItem(uint8_t slot, ArmorItemPtr cosmeticItem);
  ArmorItemPtr cosmeticItem(uint8_t slot) const;
  ItemDescriptor cosmeticItemDescriptor(uint8_t slot) const;
private:
  void netElementsNeedLoad(bool full) override;
  void netElementsNeedStore() override;

  struct Armor {
    ArmorItemPtr item;
    bool needsSync = true;
    bool needsStore = true;
    bool isCosmetic = false;
    NetElementData<ItemDescriptor> netState;
  };

  Array<Armor, 20> m_armors;
  Array<uint8_t, 4> m_wornCosmeticTypes;
  // only works under the assumption that this ArmorWearer
  // will only ever touch one Humanoid (which is true!)
  Maybe<Gender> m_lastGender;
  Maybe<Direction> m_lastDirection;
  bool m_lastNude;
  Direction facingDirection;
};

}
