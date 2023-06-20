#ifndef STAR_ITEMGRID_WIDGET_HPP
#define STAR_ITEMGRID_WIDGET_HPP

#include "StarItemBag.hpp"
#include "StarWidget.hpp"
#include "StarItemSlotWidget.hpp"
#include "StarItem.hpp"

namespace Star {

STAR_CLASS(ItemGridWidget);

class ItemGridWidget : public Widget {
public:
  ItemGridWidget(ItemBagConstPtr bag, Vec2I const& dimensions, Vec2I const& spacing, String const& backingImage, unsigned bagOffset);
  ItemGridWidget(ItemBagConstPtr bag, Vec2I const& dimensions, Vec2I const& rowSpacing, Vec2I const& columnSpacing, String const& backingImage, unsigned bagOffset);

  ItemBagConstPtr bag() const;

  ItemPtr itemAt(Vec2I const& position) const;
  ItemPtr itemAt(size_t index) const;
  ItemPtr selectedItem() const;

  ItemSlotWidgetPtr itemWidgetAt(Vec2I const& position) const;
  ItemSlotWidgetPtr itemWidgetAt(size_t index) const;

  // Returns the dimensions of the item grid
  Vec2I dimensions() const;

  // Returns the number of item slots in the grid (dimensions.x() * dimensions.y())
  size_t itemSlots() const;

  // Returns the size of the underlying bag.
  size_t bagSize() const;

  // Returns the min of bagSize() and itemSlots()
  size_t effectiveSize() const;

  size_t bagLocationAt(Vec2I const& position) const;
  Vec2I positionOfSlot(size_t slotNumber);

  bool sendEvent(InputEvent const& event) override;
  void setCallback(WidgetCallbackFunc callback);
  void setRightClickCallback(WidgetCallbackFunc callback);
  void setItemBag(ItemBagConstPtr bag);
  void setProgress(float progress);

  size_t selectedIndex() const;

  void updateAllItemSlots();

  // Item states, keeping track of new items
  void updateItemState();
  void clearChangedSlots();
  bool slotsChanged();
  void indicateChangedSlots();

  void setHighlightEmpty(bool highlight);

  void setBackingImageAffinity(bool full, bool empty);
  void showDurability(bool show);

  virtual RectI getScissorRect() const override;

protected:
  void renderImpl() override;
  HashSet<ItemDescriptor> uniqueItemState();
  List<String> slotItemNames();

private:
  Vec2I locOfItemSlot(unsigned slot) const;

  ItemBagConstPtr m_bag;
  List<ItemSlotWidgetPtr> m_slots;
  unsigned m_bagOffset;
  Vec2I m_dimensions;
  Vec2I m_rowSpacing;
  Vec2I m_columnSpacing;

  List<String> m_itemNames;
  Set<size_t> m_changedSlots;

  RectI m_itemDraggableArea;

  String m_backingImage;
  bool m_drawBackingImageWhenFull;
  bool m_drawBackingImageWhenEmpty;
  bool m_showDurability;

  float m_progress;

  bool m_highlightEmpty;

  unsigned m_selectedIndex;
  WidgetCallbackFunc m_callback;
  WidgetCallbackFunc m_rightClickCallback;
};

}

#endif
