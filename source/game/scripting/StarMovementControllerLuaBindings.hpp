#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(MovementController);

namespace LuaBindings {
  LuaCallbacks makeMovementControllerCallbacks(MovementController* movementController);
}
}
