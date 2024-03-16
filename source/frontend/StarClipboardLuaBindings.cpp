#include "StarClipboardLuaBindings.hpp"
#include "StarLuaConverters.hpp"

#include "SDL2/SDL.h"

namespace Star {

LuaCallbacks LuaBindings::makeClipboardCallbacks(ApplicationControllerPtr appController) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("hasText", []() -> bool { return SDL_HasClipboardText() == SDL_TRUE;});

  callbacks.registerCallback("getText", [appController]() -> Maybe<String> {
    return appController->getClipboard();
  });

  callbacks.registerCallback("setText", [appController](String const& text) {
    appController->setClipboard(text);
  });

  return callbacks;
};

}// namespace Star
