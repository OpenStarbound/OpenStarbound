#pragma once

#include "StarLua.hpp"
#include "StarNetworkedAnimator.hpp"

namespace Star {

namespace LuaBindings {
  LuaCallbacks makeScriptedAnimatorCallbacks(NetworkedAnimator* networkedAnimator, function<Json(String const&, Json const&)> getParameter);
}
}
