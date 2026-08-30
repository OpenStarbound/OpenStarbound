#pragma once

#include "StarMovementController.hpp"
#include "StarEntity.hpp"


namespace Star {

STAR_CLASS(StatusController);
STAR_CLASS(ActorMovementController);

// A base for 'mobile' entities, which all have a MovementController.
class MobileEntity : public virtual Entity {
public:
  virtual MovementController* movementController() = 0;
};
}
