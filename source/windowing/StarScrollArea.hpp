#ifndef _STAR_SCROLL_AREA_HPP_
#define _STAR_SCROLL_AREA_HPP_

#include "StarWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarImageWidget.hpp"

namespace Star {

class ScrollThumb : public Widget {
public:
  ScrollThumb(GuiDirection direction);
  virtual ~ScrollThumb() {}

  void setDirection(GuiDirection direction);
  void setImages(ImageStretchSet const& base,
      ImageStretchSet const& hover = ImageStretchSet(),
      ImageStretchSet const& pressed = ImageStretchSet());
  void setImages(Json const& images);

  bool isHovered() const;
  bool isPressed() const;

  void setHovered(bool hovered);
  void setPressed(bool pressed);

  void mouseOver() override;
  void mouseOut() override;

  Vec2U baseSize() const;

protected:
  virtual void renderImpl() override;

private:
  void readDefaults();

  GuiDirection m_direction;

  ImageStretchSet m_baseThumb;
  ImageStretchSet m_hoverThumb;
  ImageStretchSet m_pressedThumb;

  bool m_hovered;
  bool m_pressed;
};
typedef shared_ptr<ScrollThumb> ScrollThumbPtr;

class ScrollBar : public Widget {
public:
  ScrollBar(GuiDirection direction, WidgetCallbackFunc forwardFunc, WidgetCallbackFunc backwardFunc);

  void setButtonImages(Json const& images);

  int trackSize() const;
  float sizeRatio() const;
  virtual Vec2I size() const override;
  float scrollRatio() const;
  Vec2I offsetFromThumbPosition(Vec2I const& thumbPosition) const;

  ButtonWidgetPtr forwardButton() const;
  ButtonWidgetPtr backwardButton() const;
  ScrollThumbPtr thumb() const;

protected:
  void drawChildren() override;

private:
  GuiDirection m_direction;

  ButtonWidgetPtr m_forward; // up or right, makes the offset higher
  ButtonWidgetPtr m_backward; // down or left, makes the offset lower
  ScrollThumbPtr m_thumb;

  ImageStretchSet m_track;
};
typedef shared_ptr<ScrollBar> ScrollBarPtr;

class ScrollArea : public Widget {
public:
  ScrollArea();

  void setButtonImages(Json const& images);
  void setThumbImages(Json const& images);

  RectI contentBoundRect() const;
  Vec2I contentSize() const;

  Vec2I areaSize() const;

  void scrollAreaBy(Vec2I const& offset);

  Vec2I scrollOffset() const;
  Vec2I maxScrollPosition() const;

  bool horizontalScroll() const;
  void setHorizontalScroll(bool horizontal);

  bool verticalScroll() const;
  void setVerticalScroll(bool vertical);

  virtual bool sendEvent(InputEvent const& event) override;
  virtual void update() override;

protected:
  void drawChildren() override;

private:
  int advanceFactorHelper();

  int m_buttonAdvance;
  int64_t m_advanceLimiter;

  Vec2I m_scrollOffset;
  Vec2I m_lastMaxScroll;
  Vec2I m_contentSize;

  bool m_dragActive;
  GuiDirection m_dragDirection;
  Vec2I m_dragOffset;

  ScrollBarPtr m_vBar;
  ScrollBarPtr m_hBar;
  ImageWidgetPtr m_cornerBlock;

  bool m_horizontalScroll;
  bool m_verticalScroll;
};
typedef shared_ptr<ScrollArea> ScrollAreaPtr;
}

#endif
