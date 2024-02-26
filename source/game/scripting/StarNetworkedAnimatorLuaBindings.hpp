#pragma once

#include "StarLua.hpp"
#include "StarPoly.hpp"
#include "StarColor.hpp"

namespace Star {

STAR_CLASS(NetworkedAnimator);

namespace LuaBindings {
  LuaCallbacks makeNetworkedAnimatorCallbacks(NetworkedAnimator* networkedAnimator);
}

}
