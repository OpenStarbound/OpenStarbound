#pragma once

#include "StarWidget.hpp"
#include "StarProgressWidget.hpp"
#include "StarAnimation.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(ItemSlotWidget);

static float const ItemIndicateNewTime = 1.5f;

class ItemSlotWidget : public Widget {
public:
  ItemSlotWidget(ItemPtr const& item, String const& backingImage);

  virtual void update(float dt) override;
  bool sendEvent(InputEvent const& event) override;
  void setCallback(WidgetCallbackFunc callback);
  void setRightClickCallback(WidgetCallbackFunc callback);
  void setMiddleClickCallback(WidgetCallbackFunc callback);
  void setItem(ItemPtr const& item);
  ItemPtr item() const;
  void setProgress(float progress);
  void setBackingImageAffinity(bool full, bool empty);
  void setCountPosition(TextPositioning textPositioning);
  void setCountFontMode(FontMode fontMode);

  void showDurability(bool show);
  void showCount(bool show);
  void showRarity(bool showRarity);
  void showLinkIndicator(bool showLinkIndicator);
  void showSecondaryIcon(bool show);

  void indicateNew();

  void setHighlightEnabled(bool highlight);

protected:
  virtual void renderImpl() override;

private:
  ItemPtr m_item;

  String m_backingImage;
  bool m_drawBackingImageWhenFull;
  bool m_drawBackingImageWhenEmpty;
  bool m_showDurability;
  bool m_showCount;
  bool m_showRarity;
  bool m_showLinkIndicator;
  bool m_showSecondaryIcon;

  TextPositioning m_countPosition;
  FontMode m_countFontMode;

  Vec2I m_durabilityOffset;
  RectI m_itemDraggableArea;

  TextStyle m_textStyle;

  WidgetCallbackFunc m_callback;
  WidgetCallbackFunc m_rightClickCallback;
  WidgetCallbackFunc m_middleClickCallback;
  float m_progress;

  ProgressWidgetPtr m_durabilityBar;

  Animation m_newItemIndicator;

  bool m_highlightEnabled;
  Animation m_highlightAnimation;
};

}
