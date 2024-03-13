#pragma once

#include "StarLua.hpp"
#include "StarApplicationController.hpp"

namespace Star {

namespace LuaBindings {
LuaCallbacks makeClipboardCallbacks(ApplicationControllerPtr appController);
}

}// namespace Star
