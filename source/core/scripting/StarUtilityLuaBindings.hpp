#pragma once

#include "StarLua.hpp"

namespace Star {

namespace LuaBindings {
  LuaCallbacks makeUtilityCallbacks();

  String formatLua(String const& string, List<LuaValue> const& args);

  namespace UtilityCallbacks {
    double nrand(Maybe<double> const& stdev, Maybe<double> const& mean);
    String makeUuid();
    void logInfo(String const& str, LuaVariadic<LuaValue> const& args);
    void logWarn(String const& str, LuaVariadic<LuaValue> const& args);
    void logError(String const& str, LuaVariadic<LuaValue> const& args);
    void setLogMap(String const& key, String const& value, LuaVariadic<LuaValue> const& args);
    Json parseJson(String const& str);
    String printJson(Json const& arg, Maybe<int> pretty);
    String print(LuaValue const& arg);
    LuaValue interpolateSinEase(LuaEngine& engine, double offset, LuaValue const& value1, LuaValue const& value2);
    String replaceTags(String const& str, StringMap<String> const& tags);
  }
}
}
