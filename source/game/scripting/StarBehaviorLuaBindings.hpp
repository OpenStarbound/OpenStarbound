#pragma once

#include "StarLua.hpp"
#include "StarBehaviorState.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeBehaviorLuaCallbacks(List<BehaviorStatePtr>* list);
}
}
