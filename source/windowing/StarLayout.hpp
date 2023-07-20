#ifndef STAR_LAYOUT_HPP
#define STAR_LAYOUT_HPP
#include "StarWidget.hpp"

namespace Star {

// VERY simple base class for a layout container object.
class Layout : public Widget {
public:
  Layout();
  virtual void update(float dt) override;
};

}

#endif
