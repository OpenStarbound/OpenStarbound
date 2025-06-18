#pragma once

#include "StarMathCommon.hpp"
#include "StarItemDescriptor.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(ItemBag);

// Manages a collection of items with non-zero counts, and putting them in /
// stacking them / consuming them.  As items are taken out of the ItemBag, any
// Item with a zero count is set to null, so that no ItemPtr returned by this
// class should ever be empty.  They will either be null, or of count >= 1.
// All methods are safe to call with null ItemPtrs.  Any non-const ItemPtr
// given to the ItemBag may be used internally depending on how the item
// stacks, so should not be used after passing to the method.
class ItemBag {
public:
  struct ItemsFitWhereResult {
    uint64_t leftover;
    List<size_t> slots;
  };

  ItemBag();
  explicit ItemBag(size_t size);

  static ItemBag fromJson(Json const& spec);
  static ItemBag loadStore(Json const& store);

  Json toJson() const;
  Json diskStore() const;

  size_t size() const;
  // May reshape the container, but will try not to lose any container
  // contents.  Returns overflow.
  List<ItemPtr> resize(size_t size);
  // Clears all item slots, does not change ItemBag size
  void clearItems();

  // Force a cleanup of any empty items from the ItemBag.  Even though no
  // methods should ever return a null item, it can be usefull to force cleanup
  // to remove empty items from memory.  If any action was done, will return
  // true.
  bool cleanup() const;

  // Direct access to item list
  List<ItemPtr>& items();
  List<ItemPtr> const& items() const;

  ItemPtr const& at(size_t i) const;
  ItemPtr& at(size_t i);

  // Returns all non-empty items and clears container contents
  List<ItemPtr> takeAll();

  // Directly set the value of an item at a given slot
  void setItem(size_t pos, ItemPtr item);

  // Put items into the given slot.  Returns number of items left over
  ItemPtr putItems(size_t pos, ItemPtr items);
  // Take a maximum number of items from the given position, defaults to all.
  ItemPtr takeItems(size_t pos, uint64_t count = highest<uint64_t>());
  // Put items in the slot by combining, or swap the current items with the
  // given items.
  ItemPtr swapItems(size_t pos, ItemPtr items, bool tryCombine = true);

  // Destroys the given number of items, only if the entirety of count is
  // available, returns success.
  bool consumeItems(size_t pos, uint64_t count);

  // Consume any items from any stack that matches the given item descriptor,
  // only if the entirety of the count is available.  Returns success.
  bool consumeItems(ItemDescriptor const& descriptor, bool exactMatch = false);

  // Returns the number of times this ItemDescriptor could be consumed using
  // the items in this container.
  uint64_t available(ItemDescriptor const& descriptor, bool exactMatch = false) const;

  // Returns the number of items that can fit anywhere in the bag, including
  // being split up.
  uint64_t itemsCanFit(ItemConstPtr const& items) const;
  // Returns the number of items that can be stacked with existing items
  // anywhere in the bag.
  uint64_t itemsCanStack(ItemConstPtr const& items) const;

  // Returns where the items would fit if inserted, including any splitting up
  ItemsFitWhereResult itemsFitWhere(ItemConstPtr const& items, uint64_t max = highest<uint64_t>()) const;

  // Add items anywhere in the bag. Tries to stack items first.  If any items
  // are left over, addItems returns them, otherwise null.
  ItemPtr addItems(ItemPtr items);

  // Add items to the bag, but only if they stack with existing items in the
  // bag.
  ItemPtr stackItems(ItemPtr items);

  // Attempt to condense all stacks in the given bag
  void condenseStacks();

  // Uses ItemDatabase to serialize / deserialize all items
  void read(DataStream& ds);
  void write(DataStream& ds) const;

private:
  // If the from item can stack into the given to item, returns the amount that
  // would be transfered.
  static uint64_t stackTransfer(ItemConstPtr const& to, ItemConstPtr const& from);

  // Returns the slot that contains the item already and has the *highest*
  // stack count but not full, or an empty slot, or NPos for no room.
  size_t bestSlotAvailable(ItemConstPtr const& item, bool stacksOnly, std::function<bool(size_t)> test) const;
  size_t bestSlotAvailable(ItemConstPtr const& item, bool stacksOnly) const;

  List<ItemPtr> m_items;
};

}
