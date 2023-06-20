#ifndef STAR_ROOT_LUA_BINDINGS_HPP
#define STAR_ROOT_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeCelestialCallbacks(UniverseClient* client);
}
}

#endif
