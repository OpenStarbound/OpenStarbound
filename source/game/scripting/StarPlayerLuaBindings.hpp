#ifndef STAR_PLAYER_LUA_BINDINGS_HPP
#define STAR_PLAYER_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Player);

namespace LuaBindings {
  LuaCallbacks makePlayerCallbacks(Player* player);
}
}

#endif
