#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(Universe);
STAR_CLASS(CelestialDatabase);

namespace LuaBindings {
  LuaCallbacks makeCelestialCallbacks(CelestialDatabasePtr database);
  LuaCallbacks makeCelestialCallbacks(Universe* universe);
}
}
