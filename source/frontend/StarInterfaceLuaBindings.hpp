#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(MainInterface);
STAR_CLASS(UniverseClient);

namespace LuaBindings {
  LuaCallbacks makeInterfaceCallbacks(MainInterface* mainInterface);
  LuaCallbacks makeChatCallbacks(MainInterface* mainInterface, UniverseClient* client);
}

}
