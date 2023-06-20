#ifndef STAR_POINTABLE_ITEM_HPP
#define STAR_POINTABLE_ITEM_HPP

#include "StarGameTypes.hpp"
#include "StarDrawable.hpp"

namespace Star {

STAR_CLASS(PointableItem);

class PointableItem {
public:
  virtual ~PointableItem() {}

  virtual float getAngleDir(float aimAngle, Direction facingDirection);
  virtual float getAngle(float angle);
  virtual List<Drawable> drawables() const = 0;
};

}

#endif
