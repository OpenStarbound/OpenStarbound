#ifndef STAR_BEHAVIOR_LUA_BINDINGS_HPP
#define STAR_BEHAVIOR_LUA_BINDINGS_HPP

#include "StarLua.hpp"
#include "StarBehaviorState.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeBehaviorLuaCallbacks(List<BehaviorStatePtr>* list);
}
}

#endif
