#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(WorldCamera);

namespace LuaBindings {
  LuaCallbacks makeCameraCallbacks(WorldCamera* camera);
}
}
