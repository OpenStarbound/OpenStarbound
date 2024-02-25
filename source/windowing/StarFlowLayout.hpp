#pragma once

#include "StarLayout.hpp"

namespace Star {

// Super simple, only supports left to right, top to bottom flow layouts
// currently
STAR_CLASS(FlowLayout);
class FlowLayout : public Layout {
public:
  FlowLayout();
  virtual void update(float dt) override;
  void setSpacing(Vec2I const& spacing);
  void setWrapping(bool wrap);

private:
  Vec2I m_spacing;
  bool m_wrap;
};

}
