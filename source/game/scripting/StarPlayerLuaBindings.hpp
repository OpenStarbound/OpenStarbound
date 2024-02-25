#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Player);

namespace LuaBindings {
  LuaCallbacks makePlayerCallbacks(Player* player);
}
}
