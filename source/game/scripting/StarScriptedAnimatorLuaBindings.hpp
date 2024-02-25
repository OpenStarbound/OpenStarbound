#pragma once

#include "StarLua.hpp"
#include "StarNetworkedAnimator.hpp"

namespace Star {

namespace LuaBindings {
  LuaCallbacks makeScriptedAnimatorCallbacks(const NetworkedAnimator* animator, function<Json(String const&, Json const&)> getParameter);
}
}
