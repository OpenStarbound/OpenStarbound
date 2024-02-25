#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Input);

namespace LuaBindings {
  LuaCallbacks makeInputCallbacks();
}

}
