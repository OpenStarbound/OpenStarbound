#include "StarInputLuaBindings.hpp"
#include "StarLuaConverters.hpp"
#include "StarInput.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeInputCallbacks() {
  LuaCallbacks callbacks;

  auto input = Input::singletonPtr();

  callbacks.registerCallbackWithSignature<Maybe<unsigned>, String, String>("bindDown", bind(mem_fn(&Input::bindDown), input, _1, _2));
  callbacks.registerCallbackWithSignature<bool,            String, String>("bindHeld", bind(mem_fn(&Input::bindHeld), input, _1, _2));
  callbacks.registerCallbackWithSignature<bool,            String, String>("bind",     bind(mem_fn(&Input::bindHeld), input, _1, _2));
  callbacks.registerCallbackWithSignature<Maybe<unsigned>, String, String>("bindUp",   bind(mem_fn(&Input::bindUp),   input, _1, _2));

  callbacks.registerCallback("keyDown", [input](String const& keyName, Maybe<StringList> const& modNames) -> Maybe<unsigned> {
    Key key = KeyNames.getLeft(keyName);
    Maybe<KeyMod> mod;
    if (modNames) {
      mod = KeyMod::NoMod;
      for (auto& modName : *modNames)
        *mod |= KeyModNames.getLeft(modName);
    }
    return input->keyDown(key, mod);
  });
  auto keyHeld = [input](String const& keyName) -> bool { return input->keyHeld(KeyNames.getLeft(keyName)); };
  callbacks.registerCallback("keyHeld", keyHeld);
  callbacks.registerCallback("key",     keyHeld);
  callbacks.registerCallback("keyUp",   [input](String const& keyName) -> Maybe<unsigned> { return input->keyUp(  KeyNames.getLeft(keyName)); });

  callbacks.registerCallback("mouseDown", [input](String const& buttonName) -> Maybe<List<Vec2I>> { return input->mouseDown(MouseButtonNames.getLeft(buttonName)); });
  
  auto mouseHeld = [input](String const& buttonName) -> bool { return input->mouseHeld(MouseButtonNames.getLeft(buttonName)); };
  callbacks.registerCallback("mouseHeld", mouseHeld);
  callbacks.registerCallback("mouse",     mouseHeld);
  callbacks.registerCallback("mouseUp",   [input](String const& buttonName) -> Maybe<List<Vec2I>> { return input->mouseUp(  MouseButtonNames.getLeft(buttonName)); });

  callbacks.registerCallbackWithSignature<void, String, String>("resetBinds",      bind(mem_fn(&Input::resetBinds),      input, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, String, Json>("setBinds",  bind(mem_fn(&Input::setBinds),        input, _1, _2, _3));
  callbacks.registerCallbackWithSignature<Json, String, String>("getDefaultBinds", bind(mem_fn(&Input::getDefaultBinds), input, _1, _2));
  callbacks.registerCallbackWithSignature<Json, String, String>("getBinds",        bind(mem_fn(&Input::getBinds),        input, _1, _2));

  callbacks.registerCallback("events", [input]() -> Json {
    JsonArray result;

    for (auto& pair : input->inputEventsThisFrame()) {
      if (auto jEvent = Input::inputEventToJson(pair.first))
        result.emplace_back(jEvent.set("processed", pair.second));
    }

    return move(result);
  });

  callbacks.registerCallbackWithSignature<Vec2I>("mousePosition", bind(mem_fn(&Input::mousePosition), input));

  return callbacks;
}


}
