#include "StarVerticalLayout.hpp"

namespace Star {

VerticalLayout::VerticalLayout(VerticalAnchor verticalAnchor, int verticalSpacing) {
  setVerticalAnchor(verticalAnchor);
  setVerticalSpacing(verticalSpacing);

  disableScissoring();
}

void VerticalLayout::update(float) {
  m_size = Vec2I(0, 0);

  if (m_members.empty())
    return;

  for (auto const& child : m_members) {
    auto childSize = child->size();
    m_size[1] += childSize[1];
    m_size[0] = max(m_size[0], childSize[0]);
  }
  m_size[1] += (m_members.size() - 1) * m_verticalSpacing;

  auto bounds = contentBoundRect();

  int verticalPos = m_fillDown ? bounds.yMax() : bounds.yMin();
  for (auto const& child : reverseIterate(m_members)) {
    auto childSize = child->size();

    Vec2I targetPosition;

    if (m_horizontalAnchor == HorizontalAnchor::LeftAnchor)
      targetPosition[0] = bounds.xMin();
    else if (m_horizontalAnchor == HorizontalAnchor::RightAnchor)
      targetPosition[0] = bounds.xMax() - childSize[0];
    else if (m_horizontalAnchor == HorizontalAnchor::HMidAnchor)
      targetPosition[0] = -childSize[0] / 2;

    if (m_fillDown) {
      verticalPos -= childSize[1];
      targetPosition[1] = verticalPos;
      verticalPos -= m_verticalSpacing;
    } else {
      targetPosition[1] = verticalPos;
      verticalPos -= childSize[1];
      verticalPos -= m_verticalSpacing;
    }

    // needed because position is included in relativeBoundRect
    child->setPosition(Vec2I(0, 0));

    child->setPosition(targetPosition - child->relativeBoundRect().min());
  }
}

Vec2I VerticalLayout::size() const {
  return m_size;
}

RectI VerticalLayout::relativeBoundRect() const {
  return contentBoundRect().translated(relativePosition());
}

void VerticalLayout::setHorizontalAnchor(HorizontalAnchor horizontalAnchor) {
  m_horizontalAnchor = horizontalAnchor;
  update(0);
}

void VerticalLayout::setVerticalAnchor(VerticalAnchor verticalAnchor) {
  m_verticalAnchor = verticalAnchor;
  update(0);
}

void VerticalLayout::setVerticalSpacing(int verticalSpacing) {
  m_verticalSpacing = verticalSpacing;
  update(0);
}

void VerticalLayout::setFillDown(bool fillDown) {
  m_fillDown = fillDown;
  update(0);
}

RectI VerticalLayout::contentBoundRect() const {
  auto min = Vec2I(0, 0);

  if (m_horizontalAnchor == HorizontalAnchor::RightAnchor)
    min[0] -= m_size[0];
  else if (m_horizontalAnchor == HorizontalAnchor::HMidAnchor)
    min[0] -= m_size[0] / 2;

  if (m_verticalAnchor == VerticalAnchor::TopAnchor)
    min[1] -= m_size[1];
  else if (m_verticalAnchor == VerticalAnchor::VMidAnchor)
    min[1] -= m_size[1] / 2;

  return RectI::withSize(min, m_size);
}

}
