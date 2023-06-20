#ifndef STAR_CONFIG_LUA_BINDINGS_HPP
#define STAR_CONFIG_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

namespace LuaBindings {
  LuaCallbacks makeConfigCallbacks(function<Json(String const&, Json const&)> getParameter);
}
}

#endif
