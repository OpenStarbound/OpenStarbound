#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Voice);

namespace LuaBindings {
  LuaCallbacks makeVoiceCallbacks();
}

}
