#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(TeamClient);

namespace LuaBindings {
  LuaCallbacks makeTeamClientCallbacks(TeamClient* teamClient);
}
}
