#pragma once

#include "StarLua.hpp"
#include "StarGameTypes.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeUniverseClientThreadCallbacks(UniverseClient* universe); // thread-safe callbacks
  LuaCallbacks makeUniverseClientCallbacks(UniverseClient* universe); // non-thread-safe callbacks

  namespace UniverseClientCallbacks {
  }
}
}
