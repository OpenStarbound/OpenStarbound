#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeCelestialCallbacks(UniverseClient* client);
}
}
