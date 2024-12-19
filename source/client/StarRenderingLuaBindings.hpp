#pragma once

#include "StarLua.hpp"

namespace Star {
  
STAR_CLASS(ClientApplication);

namespace LuaBindings {
  LuaCallbacks makeRenderingCallbacks(ClientApplication* app);
}

}
