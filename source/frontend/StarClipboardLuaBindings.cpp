#include "StarClipboardLuaBindings.hpp"
#include "StarLuaConverters.hpp"

#include "SDL2/SDL.h"

namespace Star {

LuaCallbacks LuaBindings::makeClipboardCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("hasText", []() -> bool { return SDL_HasClipboardText() == SDL_TRUE; });

  callbacks.registerCallback("getText", []() -> Maybe<String> {
    if (SDL_HasClipboardText() == SDL_FALSE)
      return {};

    char* clipText = SDL_GetClipboardText();
    String text(clipText);

    SDL_free(clipText);

    return text;
  });

  callbacks.registerCallback("setText", [](String const& text) -> Maybe<String> {
    int errorCode = SDL_SetClipboardText(text.utf8().c_str());
    if (errorCode != 0) {
      return "SDL_SetClipboardText failed: " + String(SDL_GetError());
    }
    return {};
  });

  return callbacks;
};

}// namespace Star
