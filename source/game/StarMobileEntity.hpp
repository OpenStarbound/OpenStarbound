#pragma once

#include "StarEntity.hpp"

STAR_CLASS(MovementController);

namespace Star {

class MobileEntity : public virtual Entity {
public:
  virtual MovementController* movementController() = 0;

};

}
