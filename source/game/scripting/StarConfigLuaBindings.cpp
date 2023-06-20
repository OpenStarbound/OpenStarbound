#include "StarConfigLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeConfigCallbacks(function<Json(String const&, Json const&)> getParameter) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("getParameter", getParameter);

  return callbacks;
}

}
