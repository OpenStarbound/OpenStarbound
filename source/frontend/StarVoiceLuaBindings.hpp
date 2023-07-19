#ifndef STAR_VOICE_LUA_BINDINGS_HPP
#define STAR_VOICE_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Voice);

namespace LuaBindings {
  LuaCallbacks makeVoiceCallbacks();
}

}

#endif
