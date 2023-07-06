#include "StarItemGridWidget.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"

namespace Star {

ItemGridWidget::ItemGridWidget(ItemBagConstPtr bag, Vec2I const& dimensions, Vec2I const& spacing, String const& backingImage, unsigned bagOffset)
  : ItemGridWidget(bag, dimensions, {spacing[0], 0}, {0, spacing[1]}, backingImage, bagOffset) {}

ItemGridWidget::ItemGridWidget(ItemBagConstPtr bag, Vec2I const& dimensions, Vec2I const& rowSpacing, Vec2I const& columnSpacing, String const& backingImage, unsigned bagOffset)
  : m_bagOffset(bagOffset), m_dimensions(dimensions), m_rowSpacing(rowSpacing), m_columnSpacing(columnSpacing), m_backingImage(backingImage) {
  m_selectedIndex = 0;
  m_progress = 1;

  m_drawBackingImageWhenFull = false;
  m_drawBackingImageWhenEmpty = true;
  m_showDurability = false;
  m_highlightEmpty = false;

  setItemBag(bag);

  auto assets = Root::singleton().assets();
  m_itemDraggableArea = jsonToRectI(assets->json("/interface.config:itemDraggableArea"));
  Vec2I calculatedSize = {
    m_dimensions[0] * m_rowSpacing[0] + m_dimensions[1] * m_columnSpacing[0],
    m_dimensions[0] * m_rowSpacing[1] + m_dimensions[1] * m_columnSpacing[1]
  };
  setSize(calculatedSize.piecewiseMax(m_itemDraggableArea.size()));

  disableScissoring();
  markAsContainer();
}

ItemBagConstPtr ItemGridWidget::bag() const {
  return m_bag;
}

ItemPtr ItemGridWidget::itemAt(Vec2I const& position) const {
  auto pos = bagLocationAt(position);
  if (pos != NPos && m_bag)
    return m_bag->at(pos);

  return {};
}

ItemPtr ItemGridWidget::itemAt(size_t index) const {
  if (index < m_bag->size())
    return m_bag->at(index);
  return {};
}

ItemPtr ItemGridWidget::selectedItem() const {
  return itemAt(selectedIndex());
}

ItemSlotWidgetPtr ItemGridWidget::itemWidgetAt(Vec2I const& position) const {
  auto pos = bagLocationAt(position);
  if (pos != NPos)
    return m_slots[pos];
  return {};
}

ItemSlotWidgetPtr ItemGridWidget::itemWidgetAt(size_t index) const {
  if (index < m_slots.size())
    return m_slots[index];
  return {};
}

Vec2I ItemGridWidget::dimensions() const {
  return m_dimensions;
}

size_t ItemGridWidget::itemSlots() const {
  return m_dimensions.x() * m_dimensions.y();
}

size_t ItemGridWidget::bagSize() const {
  if (!m_bag)
    return 0;

  return m_bag->size();
}

size_t ItemGridWidget::effectiveSize() const {
  return min(itemSlots(), bagSize());
}

size_t ItemGridWidget::bagLocationAt(Vec2I const& position) const {
  if (m_bag) {
    for (size_t i = 0; i < (m_bag->size() - m_bagOffset) && i < unsigned(m_dimensions[0] * m_dimensions[1]); ++i) {
      Vec2I loc = locOfItemSlot(i);
      RectI bagItemArea = RectI(m_itemDraggableArea).translated(screenPosition() + loc);
      if (bagItemArea.contains(position))
        return i + m_bagOffset;
    }
  }

  return NPos;
}

Vec2I ItemGridWidget::positionOfSlot(size_t slotNumber) {
  return m_slots.at(slotNumber)->position() + position();
}

bool ItemGridWidget::sendEvent(InputEvent const& event) {
  if (m_visible) {
    if (auto mouseButton = event.ptr<MouseButtonDownEvent>()) {
      if (mouseButton->mouseButton == MouseButton::Left || (m_rightClickCallback && mouseButton->mouseButton == MouseButton::Right)) {
        Vec2I mousePos = *context()->mousePosition(event);
        for (size_t i = 0; i < (m_bag->size() - m_bagOffset) && i < unsigned(m_dimensions[0] * m_dimensions[1]); ++i) {
          Vec2I loc = locOfItemSlot(i);
          RectI bagItemArea = RectI(m_itemDraggableArea).translated(screenPosition() + loc);

          if (bagItemArea.contains(mousePos)) {
            m_selectedIndex = i;
            if (mouseButton->mouseButton == MouseButton::Right)
              m_rightClickCallback(this);
            else
              m_callback(this);
            return true;
          }
        }
      }
    }
  }
  return false;
}

Vec2I ItemGridWidget::locOfItemSlot(unsigned slot) const {
  Vec2I loc = {
    (int)slot % (int)m_dimensions[0] * m_rowSpacing[0] + // x contribution from row
    (int)slot / (int)m_dimensions[0] * m_columnSpacing[0], // x contribution from column
    (m_dimensions[0] - 1) * m_rowSpacing[1] - (int)slot % (int)m_dimensions[0] * m_rowSpacing[1] + // y contribution from row
    (m_dimensions[1] - 1) * m_columnSpacing[1] - (int)slot / (int)m_dimensions[0] * m_columnSpacing[1] // y contribution from column
  };
  return loc;
}

void ItemGridWidget::setCallback(WidgetCallbackFunc callback) {
  m_callback = callback;
}

void ItemGridWidget::setRightClickCallback(WidgetCallbackFunc callback) {
  m_rightClickCallback = callback;
}

void ItemGridWidget::setItemBag(ItemBagConstPtr bag) {
  m_bag = bag;

  if (!m_bag)
    return;

  removeAllChildren();
  m_slots.clear();
  for (size_t i = 0; i < m_bag->size() - m_bagOffset && i < (unsigned)m_dimensions[0] * m_dimensions[1]; ++i) {
    auto itemSlot = make_shared<ItemSlotWidget>(m_bag->at(i), m_backingImage);
    addChild(toString(i), itemSlot);
    m_slots.append(itemSlot);
    itemSlot->setBackingImageAffinity(m_drawBackingImageWhenFull, m_drawBackingImageWhenEmpty);
    itemSlot->setProgress(m_progress);
    itemSlot->setPosition(locOfItemSlot(i));
    itemSlot->showDurability(m_showDurability);
  }

  m_itemNames = slotItemNames();
}

void ItemGridWidget::setProgress(float progress) {
  m_progress = progress;

  if (!m_bag)
    return;

  for (size_t i = 0; i < m_bag->size() - m_bagOffset && i < (unsigned)m_dimensions[0] * m_dimensions[1]; ++i) {
    auto itemSlot = m_slots.at(i);
    itemSlot->setProgress(m_progress);
  }
}

size_t ItemGridWidget::selectedIndex() const {
  return m_selectedIndex + m_bagOffset;
}

void ItemGridWidget::updateAllItemSlots() {
  if (!m_bag)
    return;

  for (size_t i = 0; i < m_bag->size() - m_bagOffset && i < (unsigned)m_dimensions[0] * m_dimensions[1]; ++i) {
    auto item = m_bag->at(i + m_bagOffset);
    auto slot = m_slots.at(i);
    slot->setItem(item);
    slot->setHighlightEnabled(!item && m_highlightEmpty);
  }
}

void ItemGridWidget::updateItemState() {
  updateAllItemSlots();
  auto newState = slotItemNames();
  for (size_t i = 0; i < newState.size(); ++i) {
    if (newState[i].empty()) {
      m_changedSlots.remove(i);
      continue;
    }

    if (newState[i].compare(m_itemNames[i]) != 0)
      m_changedSlots.insert(i);
  }
  m_itemNames = newState;
}

void ItemGridWidget::indicateChangedSlots() {
  for (auto i : m_changedSlots)
    m_slots[i]->indicateNew();
}

void ItemGridWidget::setHighlightEmpty(bool highlight) {
  m_highlightEmpty = highlight;
}

void ItemGridWidget::clearChangedSlots() {
  m_changedSlots.clear();
}

bool ItemGridWidget::slotsChanged() {
  return m_changedSlots.size() > 0;
}

HashSet<ItemDescriptor> ItemGridWidget::uniqueItemState() {
  HashSet<ItemDescriptor> state;
  if (!m_bag)
    return state;
  for (auto item : m_bag->items()) {
    if (item)
      state.add(item->descriptor().singular());
  }
  return state;
}

List<String> ItemGridWidget::slotItemNames() {
  List<String> itemNames;
  for (auto slot : m_slots) {
    if (slot->item())
      itemNames.append(slot->item()->name());
    else
      itemNames.append(String());
  }
  return itemNames;
}

void ItemGridWidget::setBackingImageAffinity(bool full, bool empty) {
  m_drawBackingImageWhenFull = full;
  m_drawBackingImageWhenEmpty = empty;

  if (!m_bag)
    return;

  for (size_t i = 0; i < m_bag->size() - m_bagOffset && i < (unsigned)m_dimensions[0] * m_dimensions[1]; ++i) {
    auto itemSlot = m_slots.at(i);
    itemSlot->setBackingImageAffinity(m_drawBackingImageWhenFull, m_drawBackingImageWhenEmpty);
  }
}

void ItemGridWidget::showDurability(bool show) {
  m_showDurability = show;

  if (!m_bag)
    return;

  for (size_t i = 0; i < m_bag->size() - m_bagOffset && i < (unsigned)m_dimensions[0] * m_dimensions[1]; ++i) {
    auto itemSlot = m_slots.at(i);
    itemSlot->showDurability(m_showDurability);
  }
}

RectI ItemGridWidget::getScissorRect() const {
  auto assets = Root::singleton().assets();
  auto durabilityOffset = jsonToVec2I(assets->json("/interface.config:itemIconDurabilityOffset"));
  auto itemCountRightAnchor = jsonToVec2I(assets->json("/interface.config:itemCountRightAnchor"));

  Vec2I extra = Vec2I(durabilityOffset * -1).piecewiseMax(itemCountRightAnchor * -1).piecewiseMax(Vec2I());
  return RectI::withSize(screenPosition() - extra, size() + extra);
}

void ItemGridWidget::renderImpl() {
  updateAllItemSlots();
}

}
