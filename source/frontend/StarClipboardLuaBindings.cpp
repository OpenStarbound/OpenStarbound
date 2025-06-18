#include "StarClipboardLuaBindings.hpp"
#include "StarLuaConverters.hpp"
#include "StarInput.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeClipboardCallbacks(ApplicationControllerPtr appController, bool alwaysAllow) {
  LuaCallbacks callbacks;

  auto available = [=]() { return alwaysAllow || (appController->isFocused() && Input::singleton().clipboardAllowed()); };

  callbacks.registerCallback("available", [=]() -> bool {
    return available();
  });

  callbacks.registerCallback("hasText", [=]() -> bool {
    return available() && appController->hasClipboard();
  });

  callbacks.registerCallback("getText", [=]() -> Maybe<String> {
    if (!available())
      return {};
    else
      return appController->getClipboard();
  });

  callbacks.registerCallback("setText", [=](String const& text) -> bool {
    if (appController->isFocused()) {
      appController->setClipboard(text);
      return true;
    }
    return false;
  });

  return callbacks;
};

}
