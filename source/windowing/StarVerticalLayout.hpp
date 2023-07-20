#ifndef STAR_VERTICAL_LAYOUT_HPP
#define STAR_VERTICAL_LAYOUT_HPP

#include "StarLayout.hpp"

namespace Star {

STAR_CLASS(VerticalLayout);

class VerticalLayout : public Layout {
public:
  VerticalLayout(VerticalAnchor verticalAnchor = VerticalAnchor::TopAnchor, int verticalSpacing = 0);

  void update(float dt) override;
  Vec2I size() const override;
  RectI relativeBoundRect() const override;

  void setHorizontalAnchor(HorizontalAnchor horizontalAnchor);
  void setVerticalAnchor(VerticalAnchor verticalAnchor);
  void setVerticalSpacing(int verticalSpacing);
  void setFillDown(bool fillDown);

private:
  RectI contentBoundRect() const;

  HorizontalAnchor m_horizontalAnchor;
  VerticalAnchor m_verticalAnchor;
  int m_verticalSpacing;
  bool m_fillDown;
  Vec2I m_size;
};

}

#endif
