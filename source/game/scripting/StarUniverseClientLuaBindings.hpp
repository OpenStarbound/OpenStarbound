#pragma once

#include "StarLua.hpp"
#include "StarGameTypes.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeUniverseClientCallbacks(UniverseClientPtr universe);

  namespace UniverseClientCallbacks {
  }
}
}
