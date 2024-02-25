#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(MainInterface);

namespace LuaBindings {
  LuaCallbacks makeInterfaceCallbacks(MainInterface* mainInterface);
}

}
