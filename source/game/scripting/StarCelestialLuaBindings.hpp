#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(Universe);

namespace LuaBindings {
  LuaCallbacks makeCelestialCallbacks(Universe* universe);
}
}
