#ifndef STAR_MOVEMENT_CONTROLLER_LUA_BINDINGS_HPP
#define STAR_MOVEMENT_CONTROLLER_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(MovementController);

namespace LuaBindings {
  LuaCallbacks makeMovementControllerCallbacks(MovementController* movementController);
}
}

#endif
