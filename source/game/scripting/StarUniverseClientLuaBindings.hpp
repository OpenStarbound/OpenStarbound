#pragma once

#include "StarLua.hpp"
#include "StarGameTypes.hpp"
#include "StarRpcThreadPromise.hpp"

namespace Star {

STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeUniverseClientCallbacks(UniverseClientPtr universe);

  namespace UniverseClientCallbacks {
  }
}
}
