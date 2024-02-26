#pragma once

#include "StarLua.hpp"

namespace Star {

namespace LuaBindings {
  LuaCallbacks makeConfigCallbacks(function<Json(String const&, Json const&)> getParameter);
}
}
