#ifndef STAR_SCRIPTED_ANIMATOR_LUA_BINDINGS_HPP
#define STAR_SCRIPTED_ANIMATOR_LUA_BINDINGS_HPP

#include "StarLua.hpp"
#include "StarNetworkedAnimator.hpp"

namespace Star {

namespace LuaBindings {
  LuaCallbacks makeScriptedAnimatorCallbacks(const NetworkedAnimator* animator, function<Json(String const&, Json const&)> getParameter);
}
}

#endif
