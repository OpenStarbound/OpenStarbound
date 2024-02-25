#pragma once

#include "StarFireableItem.hpp"

namespace Star {

STAR_CLASS(SwingableItem);

class SwingableItem : public FireableItem {
public:
  SwingableItem();
  SwingableItem(Json const& params);
  virtual ~SwingableItem() {}

  // These can be different
  // Default implementation is the same though
  virtual float getAngleDir(float aimAngle, Direction facingDirection);
  virtual float getAngle(float aimAngle);
  virtual float getItemAngle(float aimAngle);
  virtual String getArmFrame();

  virtual List<Drawable> drawables() const = 0;

  void setParams(Json const& params);

protected:
  float m_swingStart;
  float m_swingFinish;
  float m_swingAimFactor;
  Maybe<float> m_coolingDownAngle;
};

}
