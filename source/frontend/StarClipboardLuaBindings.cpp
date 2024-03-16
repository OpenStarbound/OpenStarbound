#include "StarClipboardLuaBindings.hpp"
#include "StarLuaConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeClipboardCallbacks(ApplicationControllerPtr appController) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("hasText", [appController]() -> bool {
    return appController->hasClipboard();
  });

  callbacks.registerCallback("getText", [appController]() -> Maybe<String> {
    return appController->getClipboard();
  });

  callbacks.registerCallback("setText", [appController](String const& text) {
    appController->setClipboard(text);
  });

  return callbacks;
};

}
