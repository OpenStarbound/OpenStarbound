#pragma once

#include "StarEntity.hpp"
#include "StarActorMovementController.hpp"
#include "StarStatusController.hpp"

STAR_CLASS(ActorMovementController);
STAR_CLASS(StatusController);

namespace Star {

class ActorEntity : public virtual Entity {
public:
  virtual ActorMovementController* movementController() = 0;
  virtual StatusController* statusController() = 0;

};

}
