#include "StarItemBag.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

ItemBag::ItemBag() {}

ItemBag::ItemBag(size_t size) {
  m_items.resize(size);
}

ItemBag ItemBag::fromJson(Json const& store) {
  auto itemDatabase = Root::singleton().itemDatabase();
  ItemBag res;
  res.m_items = store.toArray().transformed([itemDatabase](Json const& v) { return itemDatabase->fromJson(v); });

  return res;
}

ItemBag ItemBag::loadStore(Json const& store) {
  auto itemDatabase = Root::singleton().itemDatabase();
  ItemBag res;
  res.m_items = store.toArray().transformed([itemDatabase](Json const& v) { return itemDatabase->diskLoad(v); });

  return res;
}

Json ItemBag::toJson() const {
  auto itemDatabase = Root::singleton().itemDatabase();
  return m_items.transformed([itemDatabase](ItemConstPtr const& item) { return itemDatabase->toJson(item); });
}

Json ItemBag::diskStore() const {
  auto itemDatabase = Root::singleton().itemDatabase();
  return m_items.transformed([itemDatabase](ItemConstPtr const& item) { return itemDatabase->diskStore(item); });
}

size_t ItemBag::size() const {
  return m_items.size();
}

List<ItemPtr> ItemBag::resize(size_t size) {
  List<ItemPtr> lost;
  while (m_items.size() > size) {
    auto lastItem = m_items.takeLast();
    lastItem = addItems(lastItem);
    if (lastItem && !lastItem->empty())
      lost.append(lastItem);
  }

  m_items.resize(size);

  return lost;
}

void ItemBag::clearItems() {
  size_t oldSize = m_items.size();
  m_items.clear();
  m_items.resize(oldSize);
}

bool ItemBag::cleanup() const {
  bool cleanupDone = false;
  for (auto& items : const_cast<ItemBag*>(this)->m_items) {
    if (items && items->empty()) {
      cleanupDone = true;
      items = {};
    }
  }

  return cleanupDone;
}

List<ItemPtr>& ItemBag::items() {
  // When returning the entire item collection, need to make sure that there
  // are no empty items before returning.
  cleanup();

  return m_items;
}

List<ItemPtr> const& ItemBag::items() const {
  return const_cast<ItemBag*>(this)->items();
}

ItemPtr const& ItemBag::at(size_t i) const {
  return const_cast<ItemBag*>(this)->at(i);
}

ItemPtr& ItemBag::at(size_t i) {
  auto& item = m_items.at(i);
  if (item && item->empty())
    item = {};
  return item;
}

List<ItemPtr> ItemBag::takeAll() {
  List<ItemPtr> taken;
  for (size_t i = 0; i < size(); ++i) {
    if (auto& item = at(i))
      taken.append(move(item));
  }
  return taken;
}

void ItemBag::setItem(size_t pos, ItemPtr item) {
  auto& storedItem = at(pos);

  storedItem = item;
}

ItemPtr ItemBag::putItems(size_t pos, ItemPtr items) {
  if (!items || items->empty())
    return {};

  auto& storedItem = at(pos);

  if (storedItem) {
    // Try to stack with an item that is already there
    storedItem->stackWith(items);
    if (!items->empty())
      return items;
    else
      return {};
  } else {
    // Otherwise just put the items there and return nothing.
    storedItem = items;

    return {};
  }
}

ItemPtr ItemBag::takeItems(size_t pos, uint64_t count) {
  if (auto& storedItem = at(pos)) {
    auto taken = storedItem->take(count);
    if (storedItem->empty())
      storedItem = {};

    return taken;
  } else {
    return {};
  }
}

ItemPtr ItemBag::swapItems(size_t pos, ItemPtr items, bool tryCombine) {
  auto& storedItem = at(pos);

  auto swapItems = items;
  if (!swapItems || swapItems->empty()) {
    // If we are passed in nothing, simply return what's there, if anything.
    swapItems = storedItem;
    storedItem = {};
  } else if (storedItem) {
    // If something is there, try to stack with it first.  If we can't stack,
    // then swap.
    if (!tryCombine || !storedItem->stackWith(swapItems))
      std::swap(storedItem, swapItems);
  } else {
    // Otherwise just place the given items in the slot.
    storedItem = swapItems;
    swapItems = {};
  }

  return swapItems;
}

bool ItemBag::consumeItems(size_t pos, uint64_t count) {
  bool consumed = false;
  if (auto& storedItem = at(pos)) {
    consumed = storedItem->consume(count);
    if (storedItem->empty())
      storedItem = {};
  }

  return consumed;
}

bool ItemBag::consumeItems(ItemDescriptor const& descriptor, bool exactMatch) {
  uint64_t countLeft = descriptor.count();
  List<std::pair<size_t, uint64_t>> consumeLocations;
  for (size_t i = 0; i < m_items.size(); ++i) {
    auto& storedItem = at(i);
    if (storedItem && storedItem->matches(descriptor, exactMatch)) {
      uint64_t count = storedItem->count();
      uint64_t take = std::min(count, countLeft);
      consumeLocations.append({i, take});
      countLeft -= take;
      if (countLeft == 0)
        break;
    }
  }

  // Only consume any if we can consume them all
  if (countLeft > 0)
    return false;

  for (auto loc : consumeLocations) {
    bool res = consumeItems(loc.first, loc.second);
    _unused(res);
    starAssert(res);
  }

  return true;
}

uint64_t ItemBag::available(ItemDescriptor const& descriptor, bool exactMatch) const {
  uint64_t count = 0;
  for (auto const& items : m_items) {
    if (items && items->matches(descriptor, exactMatch))
      count += items->count();
  }

  return count / descriptor.count();
}

uint64_t ItemBag::itemsCanFit(ItemConstPtr const& items) const {
  auto itemsFit = itemsFitWhere(items);
  return items->count() - itemsFit.leftover;
}

uint64_t ItemBag::itemsCanStack(ItemConstPtr const& items) const {
  auto itemsFit = itemsFitWhere(items);
  uint64_t stackable = 0;
  for (auto slot : itemsFit.slots)
    if (m_items[slot])
      stackable += stackTransfer(at(slot), items);
  return stackable;
}

auto ItemBag::itemsFitWhere(ItemConstPtr const& items, uint64_t max) const -> ItemsFitWhereResult {
  if (!items || items->empty())
    return ItemsFitWhereResult();

  List<size_t> slots;
  uint64_t count = std::min(items->count(), max);

  while (true) {
    if (count == 0)
      break;

    size_t slot = bestSlotAvailable(items, false);
    if (slot == NPos)
      break;
    else
      slots.append(slot);

    uint64_t available = stackTransfer(at(slot), items);
    if (available != 0)
      count -= std::min(available, count);
    else
      break;
  }

  return ItemsFitWhereResult{count, slots};
}

ItemPtr ItemBag::addItems(ItemPtr items) {
  if (!items || items->empty())
    return {};

  while (true) {
    size_t slot = bestSlotAvailable(items, false);
    if (slot == NPos)
      return items;

    auto& storedItem = at(slot);
    if (storedItem) {
      storedItem->stackWith(items);
      if (items->empty())
        return {};
    } else {
      storedItem = move(items);
      return {};
    }
  }
}

ItemPtr ItemBag::stackItems(ItemPtr items) {
  if (!items || items->empty())
    return {};

  while (true) {
    size_t slot = bestSlotAvailable(items, true);
    if (slot == NPos)
      return items;

    auto& storedItem = at(slot);
    if (storedItem) {
      storedItem->stackWith(items);
      if (items->empty())
        return {};
    } else {
      storedItem = move(items);
      return {};
    }
  }
}

void ItemBag::condenseStacks() {
  for (size_t i = size() - 1; i > 0; --i) {
    if (auto& item = at(i)) {
      for (size_t j = 0; j < i; j++) {
        if (auto& stackWithItem = at(j))
          item->stackWith(stackWithItem);
        if (item->empty())
          break;
      }
    }
  }
}

void ItemBag::read(DataStream& ds) {
  auto itemDatabase = Root::singleton().itemDatabase();

  m_items.clear();
  m_items.resize(ds.readVlqU());

  size_t setItemsSize = ds.readVlqU();
  for (size_t i = 0; i < setItemsSize; ++i)
    itemDatabase->loadItem(ds.read<ItemDescriptor>(), at(i));
}

void ItemBag::write(DataStream& ds) const {
  // Try not to write the whole bag if a large part of the end of the bag is
  // empty.

  ds.writeVlqU(m_items.size());

  size_t setItemsSize = 0;
  for (size_t i = 0; i < m_items.size(); ++i) {
    if (at(i))
      setItemsSize = i + 1;
  }

  ds.writeVlqU(setItemsSize);
  for (size_t i = 0; i < setItemsSize; ++i)
    ds.write(itemSafeDescriptor(at(i)));
}

uint64_t ItemBag::stackTransfer(ItemConstPtr const& to, ItemConstPtr const& from) {
  if (!from)
    return 0;
  else if (!to)
    return from->count();
  else if (!to->stackableWith(from))
    return 0;
  else
    return std::min(to->maxStack() - to->count(), from->count());
}

size_t ItemBag::bestSlotAvailable(ItemConstPtr const& item, bool stacksOnly) const {
  // First look for any slots that can stack, before empty slots.
  for (size_t i = 0; i < m_items.size(); ++i) {
    auto const& storedItem = at(i);
    if (storedItem && stackTransfer(storedItem, item) != 0)
      return i;
  }

  if (!stacksOnly) {
    // Then, look for any empty slots.
    for (size_t i = 0; i < m_items.size(); ++i) {
      if (!at(i))
        return i;
    }
  }

  return NPos;
}

}
