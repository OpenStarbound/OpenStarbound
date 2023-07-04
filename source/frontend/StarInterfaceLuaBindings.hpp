#ifndef STAR_INTERFACE_LUA_BINDINGS_HPP
#define STAR_INTERFACE_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(MainInterface);

namespace LuaBindings {
  LuaCallbacks makeInterfaceCallbacks(MainInterface* mainInterface);
}

}

#endif
