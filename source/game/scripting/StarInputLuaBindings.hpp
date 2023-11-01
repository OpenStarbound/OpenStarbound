#ifndef STAR_INPUT_LUA_BINDINGS_HPP
#define STAR_INPUT_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Input);

namespace LuaBindings {
  LuaCallbacks makeInputCallbacks();
}

}

#endif
