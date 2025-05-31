#pragma once

#include "StarInventoryTypes.hpp"
#include "StarMultiArray.hpp"
#include "StarNetElementSystem.hpp"
#include "StarItemDescriptor.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(ItemBag);
STAR_CLASS(HeadArmor);
STAR_CLASS(ChestArmor);
STAR_CLASS(LegsArmor);
STAR_CLASS(BackArmor);

STAR_CLASS(PlayerInventory);

STAR_EXCEPTION(InventoryException, StarException);

// Describes a player's entire inventory, including the main bag, material bag,
// object bag, reagent bag, food bag, weapon and armor slots, swap slot, trash
// slot, essential items, and currencies.
//
// Items in the inventory can be shorcutted in the "Action Bar", and one
// location in the action bar is selected at a time and the primary and
// secondary held items are the items pointed to in that action bar location.
//
// The special slot called the "swap" slot is used specifically for inventory
// management and is attached to the cursor.  When the swap slot is active,
// then whatever is in the slot swap temporarily becomes the only held item.
//
// The essential items are items that are not manageable and not pointable to
// by an ItemSlot, but are part of the action bar shortcut system.  They are
// used for permanent tools that need to be always quickly available.
//
// Currency items that enter the inventory are immediately put in the common currencies
// pool, and are also not manageable items.
class PlayerInventory : public NetElementSyncGroup {
public:
  // Whether the given item is allowed to go in the given slot type
  static bool itemAllowedInBag(ItemPtr const& item, String const& bagType);
  static bool itemAllowedAsEquipment(ItemPtr const& item, EquipmentSlot equipmentSlot);

  PlayerInventory();

  ItemPtr itemsAt(InventorySlot const& slot) const;

  // Attempts to combine the items with the given slot, and returns the items
  // left over (if any).
  ItemPtr stackWith(InventorySlot const& slot, ItemPtr const& items);

  // Empty the slot and take what it contains, if any.
  ItemPtr takeSlot(InventorySlot const& slot);

  // Try to exchange items between any two slots, returns true on success.
  bool exchangeItems(InventorySlot const& first, InventorySlot const& second);

  // Forces the given item into the given slot, overriding what was already
  // there.  If the item is not allowed in the given location, does nothing and
  // returns false.
  bool setItem(InventorySlot const& slot, ItemPtr const& item);

  bool consumeSlot(InventorySlot const& slot, uint64_t count = 1);

  bool slotValid(InventorySlot const& slot) const;

  // Adds items to any slot except the trash or swap slots, returns stack left
  // over.
  ItemPtr addItems(ItemPtr items);

  // Adds items to the first matching item bag, avoiding the equipment, swap,
  // or trash slots
  ItemPtr addToBags(ItemPtr items);

  // Returns number of items in the given set that can fit anywhere in any item
  // slot except the trash slot (the number of items that would be added by a
  // call to addItems).
  uint64_t itemsCanFit(ItemPtr const& items) const;

  bool hasItem(ItemDescriptor const& descriptor, bool exactMatch = false) const;
  uint64_t hasCountOfItem(ItemDescriptor const& descriptor, bool exactMatch = false) const;

  // Consume items based on ItemDescriptor. Can take from any manageable item slot.
  bool consumeItems(ItemDescriptor const& descriptor, bool exactMatch = false);
  ItemDescriptor takeItems(ItemDescriptor const& descriptor, bool takePartial = false, bool exactMatch = false);
  // Return a summary of every item that can be consumed by ItemDescriptor.
  HashMap<ItemDescriptor, uint64_t> availableItems() const;

  HeadArmorPtr headArmor() const;
  ChestArmorPtr chestArmor() const;
  LegsArmorPtr legsArmor() const;
  BackArmorPtr backArmor() const;

  HeadArmorPtr headCosmetic() const;
  ChestArmorPtr chestCosmetic() const;
  LegsArmorPtr legsCosmetic() const;
  BackArmorPtr backCosmetic() const;

  ItemBagConstPtr bagContents(String const& bag) const;

  void condenseBagStacks(String const& bag);

  // Sorting a bag will not change the contents of an action bar location.  It
  // will instead potentially change the pointed to slot of an action bar
  // location to point to the new slot that contains the same item.
  void sortBag(String const& bag);

  // Either move the contents of the given slot into the swap slot, move the
  // contents of the swap slot into the given inventory slot, or swap the
  // contents of the swap slot and the inventory slot, or combine them,
  // whichever makes the most sense.
  void shiftSwap(InventorySlot const& slot);

  // Puts the swap slot back into the inventory, if there is room.  Returns
  // true if this was successful, and the swap slot is now empty.
  bool clearSwap();

  ItemPtr swapSlotItem() const;
  void setSwapSlotItem(ItemPtr const& items);

  // Non-manageable essential items that are always available as action bar
  // entries.
  ItemPtr essentialItem(EssentialItem essentialItem) const;
  void setEssentialItem(EssentialItem essentialItem, ItemPtr item);

  // Non-manageable currencies
  StringMap<uint64_t> availableCurrencies() const;
  uint64_t currency(String const& currencyType) const;
  void addCurrency(String const& currencyType, uint64_t amount);
  bool consumeCurrency(String const& currencyType, uint64_t amount);

  // A custom bar location primary and secondary cannot point to a slot that
  // has no item, and rather than set an empty slot to that location, the slot
  // will simply be cleared.  If a primary slot is set to a two handed item, it
  // will clear the secondary slot.  Any secondary slot that is set must be a
  // one handed item.
  Maybe<InventorySlot> customBarPrimarySlot(CustomBarIndex customBarIndex) const;
  Maybe<InventorySlot> customBarSecondarySlot(CustomBarIndex customBarIndex) const;
  void setCustomBarPrimarySlot(CustomBarIndex customBarIndex, Maybe<InventorySlot> slot);
  void setCustomBarSecondarySlot(CustomBarIndex customBarIndex, Maybe<InventorySlot> slot);

  // Add the given slot to a free place in the custom bar if one is available.
  void addToCustomBar(InventorySlot slot);

  // The custom bar has 'CustomBarGroups' groups that can be switched between.
  // This will not change the selected action bar location, but may change the
  // item if the selected location points to the custom bar and the contents
  // change.
  uint8_t customBarGroup() const;
  void setCustomBarGroup(uint8_t group);
  uint8_t customBarGroups() const;
  uint8_t customBarIndexes() const;

  // The action bar is the combination of the custom bar and the essential
  // items, and any of these locations can be selected.
  SelectedActionBarLocation selectedActionBarLocation() const;
  void selectActionBarLocation(SelectedActionBarLocation selectedActionBarLocation);

  // Held items are either the items shortcutted to in the currently selected
  // ActionBar primary / secondary locations, or if the swap slot is non-empty
  // then the swap slot.
  ItemPtr primaryHeldItem() const;
  ItemPtr secondaryHeldItem() const;

  // If the primary / secondary held items are valid manageable slots, returns
  // them.
  Maybe<InventorySlot> primaryHeldSlot() const;
  Maybe<InventorySlot> secondaryHeldSlot() const;

  List<ItemPtr> pullOverflow();

  void load(Json const& store);
  Json store() const;

  // Loop over every manageable item and potentially mutate it.
  void forEveryItem(function<void(InventorySlot const&, ItemPtr&)> function);
  // Loop over every manageable item.
  void forEveryItem(function<void(InventorySlot const&, ItemPtr const&)> function) const;
  // Return every manageable item
  List<ItemPtr> allItems() const;
  // Return summary of every manageable item name and the count of that item
  Map<String, uint64_t> itemSummary() const;

  // Clears away any empty items and sets them as null, and updates action bar
  // slots to maintain the rules for the action bar.  Should be called every
  // tick.
  void cleanup();

private:
  typedef pair<Maybe<InventorySlot>, Maybe<InventorySlot>> CustomBarLink;

  static bool checkInventoryFilter(ItemPtr const& items, String const& filterName);

  ItemPtr const& retrieve(InventorySlot const& slot) const;
  ItemPtr& retrieve(InventorySlot const& slot);

  void swapCustomBarLinks(InventorySlot a, InventorySlot b);
  void autoAddToCustomBar(InventorySlot slot);

  void netElementsNeedLoad(bool full) override;
  void netElementsNeedStore() override;

  Map<EquipmentSlot, ItemPtr> m_equipment;
  Map<String, ItemBagPtr> m_bags;
  ItemPtr m_swapSlot;
  Maybe<InventorySlot> m_swapReturnSlot;
  ItemPtr m_trashSlot;
  Map<EssentialItem, ItemPtr> m_essential;
  StringMap<uint64_t> m_currencies;
  uint8_t m_customBarGroup;
  MultiArray<CustomBarLink, 2> m_customBar;
  SelectedActionBarLocation m_selectedActionBar;

  Map<EquipmentSlot, NetElementData<ItemDescriptor>> m_equipmentNetState;
  Map<String, List<NetElementData<ItemDescriptor>>> m_bagsNetState;
  NetElementData<ItemDescriptor> m_swapSlotNetState;
  NetElementData<ItemDescriptor> m_trashSlotNetState;
  Map<EssentialItem, NetElementData<ItemDescriptor>> m_essentialNetState;
  NetElementData<StringMap<uint64_t>> m_currenciesNetState;
  NetElementUInt m_customBarGroupNetState;
  MultiArray<NetElementData<CustomBarLink>, 2> m_customBarNetState;
  NetElementData<SelectedActionBarLocation> m_selectedActionBarNetState;

  List<ItemPtr> m_inventoryLoadOverflow;
};

}
