#pragma once

#include "StarWidget.hpp"
#include "StarGuiReader.hpp"

namespace Star {

STAR_CLASS(ListWidget);

class ListWidget : public Widget {
public:
  ListWidget(Json const& schema);
  ListWidget();

  RectI relativeBoundRect() const override;

  // Callback is called when the selection changes
  void setCallback(WidgetCallbackFunc callback);

  bool sendEvent(InputEvent const& event) override;
  void setSchema(Json const& schema);
  WidgetPtr constructWidget();
  WidgetPtr addItem();
  WidgetPtr addItem(size_t at);
  WidgetPtr addItem(WidgetPtr existingItem);
  void removeItem(size_t at);
  void removeItem(WidgetPtr item);
  void clear();
  size_t selectedItem() const;
  size_t itemPosition(WidgetPtr item) const;
  WidgetPtr itemAt(size_t n) const;
  WidgetPtr selectedWidget() const;
  List<WidgetPtr> const& list() const;
  size_t listSize() const;
  void setEnabled(size_t pos, bool enabled);
  void setHovered(size_t pos, bool hovered);
  void setSelected(size_t pos);
  void clearSelected();
  void setSelectedWidget(WidgetPtr selected);

  void registerMemberCallback(String const& name, WidgetCallbackFunc const& callback);

  void setFillDown(bool fillDown);
  void setColumns(uint64_t columns);

private:
  void updateSizeAndPosition();

  Json m_schema;
  GuiReader m_reader;

  Set<size_t> m_disabledItems;
  size_t m_selectedItem;
  WidgetCallbackFunc m_callback;

  String m_selectedBG;
  String m_unselectedBG;
  String m_hoverBG;
  String m_disabledBG;
  Vec2I m_spacing;

  bool m_fillDown;
  uint64_t m_columns;
};

}
