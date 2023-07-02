#include "StarInputLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarInput.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeInputCallbacks() {
  LuaCallbacks callbacks;

  auto input = Input::singletonPtr();

  callbacks.registerCallbackWithSignature<Maybe<unsigned>, String, String>("bindDown", bind(mem_fn(&Input::bindDown), input, _1, _2));
  callbacks.registerCallbackWithSignature<bool,            String, String>("bindHeld", bind(mem_fn(&Input::bindHeld), input, _1, _2));
  callbacks.registerCallbackWithSignature<Maybe<unsigned>, String, String>("bindUp",   bind(mem_fn(&Input::bindUp),   input, _1, _2));

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

  return callbacks;
}


}
