#ifndef STAR_NETWORKED_ANIMATOR_LUA_BINDINGS_HPP
#define STAR_NETWORKED_ANIMATOR_LUA_BINDINGS_HPP

#include "StarLua.hpp"
#include "StarPoly.hpp"
#include "StarColor.hpp"

namespace Star {

STAR_CLASS(NetworkedAnimator);

namespace LuaBindings {
  LuaCallbacks makeNetworkedAnimatorCallbacks(NetworkedAnimator* networkedAnimator);
}

}

#endif
