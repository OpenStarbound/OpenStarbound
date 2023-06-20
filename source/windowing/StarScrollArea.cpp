#include "StarScrollArea.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

// TODO: I hate these hardcoded values.  Please smite with fire.
static int const ScrollAreaBorder = 9;
static int const ScrollButtonStackSize = 6;
static int const ScrollThumbSize = 3;
static int const ScrollThumbOverhead = ScrollThumbSize + ScrollThumbSize;
static int const ScrollBarTrackOverhead = ScrollButtonStackSize + ScrollButtonStackSize + ScrollThumbOverhead;
static int64_t const ScrollAdvanceTimer = 100;

ScrollThumb::ScrollThumb(GuiDirection direction) {
  m_hovered = false;
  m_pressed = false;
  m_direction = direction;
  auto assets = Root::singleton().assets();
  setImages(assets->json("/interface.config:scrollArea.thumbs"));
}

void ScrollThumb::setImages(ImageStretchSet const& base, ImageStretchSet const& hover, ImageStretchSet const& pressed) {
  m_baseThumb = base;
  m_hoverThumb = hover;
  m_pressedThumb = pressed;
}

void ScrollThumb::setImages(Json const& images) {
  String directionString;
  if (m_direction == GuiDirection::Vertical) {
    directionString = "vertical";
  } else {
    directionString = "horizontal";
  }

  m_baseThumb.begin = images.get(directionString).get("base").getString("begin");
  m_baseThumb.end = images.get(directionString).get("base").getString("end");
  m_baseThumb.inner = images.get(directionString).get("base").getString("inner");

  m_hoverThumb.begin = images.get(directionString).get("hover").getString("begin");
  m_hoverThumb.end = images.get(directionString).get("hover").getString("end");
  m_hoverThumb.inner = images.get(directionString).get("hover").getString("inner");

  m_pressedThumb.begin = images.get(directionString).get("pressed").getString("begin");
  m_pressedThumb.end = images.get(directionString).get("pressed").getString("end");
  m_pressedThumb.inner = images.get(directionString).get("pressed").getString("inner");
}

bool ScrollThumb::isHovered() const {
  return m_hovered;
}

bool ScrollThumb::isPressed() const {
  return m_pressed;
}

void ScrollThumb::setHovered(bool hovered) {
  m_hovered = hovered;
}

void ScrollThumb::setPressed(bool pressed) {
  m_pressed = pressed;
}

void ScrollThumb::mouseOver() {
  setHovered(true);
}

void ScrollThumb::mouseOut() {
  setHovered(false);
}

void ScrollThumb::renderImpl() {
  ImageStretchSet workingSet = m_baseThumb;
  if (isHovered())
    workingSet = m_hoverThumb;
  if (isPressed())
    workingSet = m_pressedThumb;

  if (workingSet.fullyPopulated()) {
    context()->drawImageStretchSet(workingSet,
        RectF::withSize(Vec2F(m_parent->screenPosition() + position()), Vec2F(size())),
        m_direction);
  }
}

Vec2U ScrollThumb::baseSize() const {
  return context()->textureSize(m_baseThumb.begin);
}

ScrollBar::ScrollBar(GuiDirection direction, WidgetCallbackFunc forwardFunc, WidgetCallbackFunc backwardFunc)
  : m_direction(direction) {
  m_forward = make_shared<ButtonWidget>();
  m_forward->setCallback(forwardFunc);
  m_forward->setSustainCallbackOnDownHold(true);
  m_forward->setPressedOffset({0, 0});

  m_backward = make_shared<ButtonWidget>();
  m_backward->setCallback(backwardFunc);
  m_backward->setSustainCallbackOnDownHold(true);
  m_backward->setPressedOffset({0, 0});

  m_thumb = make_shared<ScrollThumb>(m_direction);

  auto assets = Root::singleton().assets();
  setButtonImages(assets->json("/interface.config:scrollArea.buttons"));

  addChild("thumb", m_thumb);
  addChild("forward", m_forward);
  addChild("backward", m_backward);
}

void ScrollBar::setButtonImages(Json const& images) {
  String directionString;
  if (m_direction == GuiDirection::Vertical) {
    directionString = "vertical";
  } else {
    directionString = "horizontal";
  }

  m_forward->setImages(images.get(directionString).get("forward").getString("base"),
      images.get(directionString).get("forward").getString("hover"),
      images.get(directionString).get("forward").getString("pressed"));

  m_backward->setImages(images.get(directionString).get("backward").getString("base"),
      images.get(directionString).get("backward").getString("hover"),
      images.get(directionString).get("backward").getString("pressed"));
}

Vec2I ScrollBar::size() const {
  if (m_parent)
    return m_parent->size();
  return {};
}

int ScrollBar::trackSize() const {
  if (m_parent) {
    auto scrollArea = convert<ScrollArea>(m_parent);
    auto size = scrollArea->size();
    if (m_direction == GuiDirection::Vertical) {
      if (scrollArea->horizontalScroll()) {
        return size[1] - (ScrollBarTrackOverhead + ScrollAreaBorder);
      } else {
        return size[1] - ScrollBarTrackOverhead;
      }
    } else {
      return size[0] - ScrollBarTrackOverhead;
    }
  }

  throw GuiException("Somehow have a Scroll Bar without a parent.");
}

float ScrollBar::sizeRatio() const {
  if (m_parent) {
    auto scrollArea = convert<ScrollArea>(m_parent);
    if (m_direction == GuiDirection::Vertical) {
      return scrollArea->contentSize()[1] / (float)(scrollArea->areaSize()[1]);
    } else {
      return scrollArea->contentSize()[0] / (float)(scrollArea->areaSize()[0]);
    }
  }

  throw GuiException("Somehow have a Scroll Bar without a parent.");
}

float ScrollBar::scrollRatio() const {
  if (m_parent) {
    auto scrollArea = convert<ScrollArea>(m_parent);
    if (m_direction == GuiDirection::Vertical) {
      if (scrollArea->maxScrollPosition()[1] == 0)
        return 0;
      return scrollArea->scrollOffset()[1] / (float)(scrollArea->maxScrollPosition()[1]);
    } else {
      if (scrollArea->maxScrollPosition()[0] == 0)
        return 0;
      return scrollArea->scrollOffset()[0] / (float)(scrollArea->maxScrollPosition()[0]);
    }
  }
  return 0;
}

ButtonWidgetPtr ScrollBar::forwardButton() const {
  return m_forward;
}

ButtonWidgetPtr ScrollBar::backwardButton() const {
  return m_backward;
}

ScrollThumbPtr ScrollBar::thumb() const {
  return m_thumb;
}

void ScrollBar::drawChildren() {
  if (m_parent) {
    auto scrollArea = convert<ScrollArea>(m_parent);

    float ratio = sizeRatio();
    if (ratio < 1)
      ratio = 1;

    int innerSize = (int)max(0.0f, ceil(trackSize() / ratio));
    int offsetBegin = (int)ceil((trackSize() - innerSize) * scrollRatio());
    innerSize += ScrollThumbOverhead;

    if (m_direction == GuiDirection::Vertical) {
      if (scrollArea->horizontalScroll()) {
        m_forward->setPosition(m_parent->size() - Vec2I(ScrollAreaBorder, ScrollButtonStackSize));
        m_backward->setPosition({m_parent->size()[0] - ScrollAreaBorder, ScrollAreaBorder});
        m_thumb->setPosition({m_parent->size()[0] - ScrollAreaBorder, ScrollAreaBorder + ScrollButtonStackSize + offsetBegin});
      } else {
        m_forward->setPosition(m_parent->size() - Vec2I(ScrollAreaBorder, ScrollButtonStackSize));
        m_backward->setPosition({m_parent->size()[0] - ScrollAreaBorder, 0});
        m_thumb->setPosition({m_parent->size()[0] - ScrollAreaBorder, ScrollButtonStackSize + offsetBegin});
      }
      m_thumb->setSize(Vec2I(m_thumb->baseSize()[0], innerSize));
    } else {
      m_forward->setPosition({m_parent->size()[0] - ScrollButtonStackSize, 0});
      m_backward->setPosition({0, 0});
      m_thumb->setPosition({ScrollButtonStackSize + offsetBegin, 0});
      m_thumb->setSize(Vec2I(innerSize, m_thumb->baseSize()[1]));
    }

    for (auto child : m_members) {
      child->render(m_drawingArea);
    }
  }
}

Vec2I ScrollBar::offsetFromThumbPosition(Vec2I const& thumbPosition) const {
  auto scrollArea = convert<ScrollArea>(m_parent);
  if (m_direction == GuiDirection::Vertical) {
    int scrollSpan = trackSize() - m_thumb->size()[1];
    int scrollOffset = clamp((thumbPosition[1] - ScrollButtonStackSize), 0, scrollSpan);
    float scrollRatio = (float)scrollOffset / (float)scrollSpan;
    return {scrollArea->scrollOffset()[0], ceil(scrollArea->maxScrollPosition()[1] * scrollRatio)};
  } else {
    int scrollSpan = trackSize() - m_thumb->size()[0];
    int scrollOffset = clamp((thumbPosition[0] - ScrollButtonStackSize), 0, scrollSpan);
    float scrollRatio = (float)scrollOffset / (float)scrollSpan;
    return {(int)ceil(scrollArea->maxScrollPosition()[0] * scrollRatio), scrollArea->scrollOffset()[1]};
  }
}

ScrollArea::ScrollArea() {
  auto assets = Root::singleton().assets();
  m_buttonAdvance = assets->json("/interface.config:scrollArea.buttonAdvance").toInt();
  m_advanceLimiter = Time::monotonicMilliseconds();

  WidgetCallbackFunc vAdvance = [this](Widget*) { scrollAreaBy({0, advanceFactorHelper()}); };
  WidgetCallbackFunc vRetreat = [this](Widget*) { scrollAreaBy({0, -advanceFactorHelper()}); };
  WidgetCallbackFunc hAdvance = [this](Widget*) { scrollAreaBy({advanceFactorHelper(), 0}); };
  WidgetCallbackFunc hRetreat = [this](Widget*) { scrollAreaBy({-advanceFactorHelper(), 0}); };

  m_vBar = make_shared<ScrollBar>(GuiDirection::Vertical, vAdvance, vRetreat);
  m_hBar = make_shared<ScrollBar>(GuiDirection::Horizontal, hAdvance, hRetreat);

  m_dragActive = false;

  addChild("vScrollBar", m_vBar);
  addChild("hScrollBar", m_hBar);

  m_horizontalScroll = false;
  m_verticalScroll = true;
}

void ScrollArea::setButtonImages(Json const& images) {
  m_vBar->setButtonImages(images);
  m_hBar->setButtonImages(images);
}

void ScrollArea::setThumbImages(Json const& images) {
  m_vBar->thumb()->setImages(images);
  m_hBar->thumb()->setImages(images);
}

RectI ScrollArea::contentBoundRect() const {
  RectI res = RectI::null();
  for (auto child : m_members) {
    if (child == m_vBar || child == m_hBar) // scroll bars don't count
      continue;
    if (!child->active()) // neither do hidden members
      continue;
    res.combine(child->relativeBoundRect());
  }
  res.setMax({res.max()[0], res.max()[1] + 1});
  return res;
}

Vec2I ScrollArea::contentSize() const {
  return contentBoundRect().size();
}

Vec2I ScrollArea::areaSize() const {
  Vec2I s = size();
  if (horizontalScroll())
    s[1] -= ScrollAreaBorder;
  if (verticalScroll())
    s[0] -= ScrollAreaBorder;
  return s;
}

void ScrollArea::scrollAreaBy(Vec2I const& offset) {
  m_scrollOffset += offset;
}

Vec2I ScrollArea::scrollOffset() const {
  return m_scrollOffset;
}

bool ScrollArea::horizontalScroll() const {
  return m_horizontalScroll;
}

void ScrollArea::setHorizontalScroll(bool horizontal) {
  m_horizontalScroll = horizontal;
}

bool ScrollArea::verticalScroll() const {
  return m_verticalScroll;
}

void ScrollArea::setVerticalScroll(bool vertical) {
  m_verticalScroll = vertical;
}

bool ScrollArea::sendEvent(InputEvent const& event) {
  if (!m_visible)
    return false;

  if (m_dragActive) {
    if (event.is<MouseButtonUpEvent>()) {
      blur();
      m_dragActive = false;
      m_vBar->thumb()->setPressed(false);
      m_hBar->thumb()->setPressed(false);
      return true;
    } else if (event.is<MouseMoveEvent>()) {
      auto thumbDragPosition = *context()->mousePosition(event) - screenPosition() - m_dragOffset;
      if (m_dragDirection == GuiDirection::Vertical) {
        m_scrollOffset = m_vBar->offsetFromThumbPosition(thumbDragPosition);
      } else {
        m_scrollOffset = m_hBar->offsetFromThumbPosition(thumbDragPosition);
      }
      return true;
    }
  }

  auto mousePos = context()->mousePosition(event);
  if (mousePos && !inMember(*mousePos))
    return false;

  if (event.is<MouseButtonDownEvent>()) {
    if (m_vBar->thumb()->inMember(*mousePos)) {
      focus();
      m_dragOffset = *mousePos - screenPosition() - m_vBar->thumb()->position();
      m_dragDirection = GuiDirection::Vertical;
      m_dragActive = true;
      m_vBar->thumb()->setPressed(true);
      return true;
    } else if (m_hBar->thumb()->inMember(*mousePos)) {
      focus();
      m_dragOffset = *mousePos - screenPosition() - m_hBar->thumb()->position();
      m_dragDirection = GuiDirection::Horizontal;
      m_dragActive = true;
      m_hBar->thumb()->setPressed(true);
      return true;
    }
  }

  if (Widget::sendEvent(event))
    return true;

  if (auto mouseWheel = event.ptr<MouseWheelEvent>()) {
    if (mouseWheel->mouseWheel == MouseWheel::Up)
      scrollAreaBy({0, m_buttonAdvance * 3});
    else
      scrollAreaBy({0, -m_buttonAdvance * 3});
    return true;
  }

  return true;
}

void ScrollArea::update() {
  if (!m_visible)
    return;
  
  auto maxScroll = maxScrollPosition();

  // keep vertical scroll bars same distance from the *top* on resize
  if (m_verticalScroll && maxScroll != m_lastMaxScroll)
    m_scrollOffset += (maxScroll - m_lastMaxScroll);

  m_scrollOffset = m_scrollOffset.piecewiseClamp(Vec2I(), maxScroll);
  m_lastMaxScroll = maxScroll;
}

Vec2I ScrollArea::maxScrollPosition() const {
  return Vec2I().piecewiseMax(contentSize() - size());
}

void ScrollArea::drawChildren() {
  RectI innerDrawingArea = m_drawingArea;

  if (horizontalScroll())
    innerDrawingArea.setYMin(ScrollAreaBorder);
  if (verticalScroll())
    innerDrawingArea.setXMax(innerDrawingArea.xMax() - ScrollAreaBorder);

  auto contentBoundRect = ScrollArea::contentBoundRect();
  auto contentOffset = contentBoundRect.min();
  auto contentSize = contentBoundRect.size();

  auto offset = contentOffset + scrollOffset();

  auto areaSize = ScrollArea::areaSize();

  if (contentSize[1] < areaSize[1])
    offset[1] = offset[1] - (areaSize[1] - contentSize[1]);

  for (auto child : m_members) {
    if (child == m_vBar || child == m_hBar)
      continue;
    child->setDrawingOffset(-offset);
    child->render(innerDrawingArea);
  }

  if (m_horizontalScroll)
    m_hBar->render(m_drawingArea);
  if (m_verticalScroll)
    m_vBar->render(m_drawingArea);
}

int ScrollArea::advanceFactorHelper() {
  auto t = Time::monotonicMilliseconds() - m_advanceLimiter;
  m_advanceLimiter = Time::monotonicMilliseconds();
  if ((t > ScrollAdvanceTimer) || (t < 0))
    t = ScrollAdvanceTimer;
  return (int)std::ceil((m_buttonAdvance * t) / (float)ScrollAdvanceTimer);
}

}
