#include "StarUtilityLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarUuid.hpp"
#include "StarRandom.hpp"
#include "StarPerlin.hpp"
#include "StarXXHash.hpp"
#include "StarLogging.hpp"
#include "StarInterpolation.hpp"
#include "StarLuaConverters.hpp"

namespace Star {

template <>
struct LuaConverter<RandomSource> : LuaUserDataConverter<RandomSource> {};

template <>
struct LuaUserDataMethods<RandomSource> {
  static LuaMethods<RandomSource> make() {
    LuaMethods<RandomSource> methods;

    methods.registerMethod("init", [](RandomSource& randomSource, Maybe<uint64_t> seed)
      { seed ? randomSource.init(*seed) : randomSource.init(); });
    methods.registerMethod("addEntropy", [](RandomSource& randomSource, Maybe<uint64_t> seed)
      { seed ? randomSource.addEntropy(*seed) : randomSource.addEntropy(); });

    methods.registerMethodWithSignature<uint32_t, RandomSource&>("randu32", mem_fn(&RandomSource::randu32));
    methods.registerMethodWithSignature<uint64_t, RandomSource&>("randu64", mem_fn(&RandomSource::randu64));
    methods.registerMethodWithSignature<int32_t, RandomSource&>("randi32", mem_fn(&RandomSource::randi32));
    methods.registerMethodWithSignature<int64_t, RandomSource&>("randi64", mem_fn(&RandomSource::randi64));

    methods.registerMethod("randf", [](RandomSource& randomSource, Maybe<float> arg1, Maybe<float> arg2)
      { return (arg1 && arg2) ? randomSource.randf(*arg1, *arg2) : randomSource.randf(); });
    methods.registerMethod("randd", [](RandomSource& randomSource, Maybe<double> arg1, Maybe<double> arg2)
      { return (arg1 && arg2) ? randomSource.randd(*arg1, *arg2) : randomSource.randd(); });

    methods.registerMethodWithSignature<bool, RandomSource&>("randb", mem_fn(&RandomSource::randb));

    methods.registerMethod("randInt", [](RandomSource& randomSource, int64_t arg1, Maybe<int64_t> arg2)
      { return arg2 ? randomSource.randInt(arg1, *arg2) : randomSource.randInt(arg1); });

    methods.registerMethod("randUInt", [](RandomSource& randomSource, uint64_t arg1, Maybe<uint64_t> arg2)
      { return arg2 ? randomSource.randUInt(arg1, *arg2) : randomSource.randUInt(arg1); });

    return methods;
  }
};

template <>
struct LuaConverter<PerlinF> : LuaUserDataConverter<PerlinF> {};

template <>
struct LuaUserDataMethods<PerlinF> {
  static LuaMethods<PerlinF> make() {
    LuaMethods<PerlinF> methods;

    methods.registerMethod("get",
        [](PerlinF& perlinF, float x, Maybe<float> y, Maybe<float> z) {
          if (y && z)
            return perlinF.get(x, *y, *z);
          else if (y)
            return perlinF.get(x, *y);
          else
            return perlinF.get(x);
        });

    return methods;
  }
};

String LuaBindings::formatLua(String const& string, List<LuaValue> const& args) {
  auto argsIt = args.begin();
  auto argsEnd = args.end();
  auto popArg = [&argsIt, &argsEnd]() -> LuaValue {
    if (argsIt == argsEnd)
      return LuaNil;
    return *argsIt++;
  };

  auto stringIt = string.begin();
  auto stringEnd = string.end();

  String result;

  while (stringIt != stringEnd) {
    if (*stringIt == '%') {
      auto next = stringIt;
      ++next;

      if (next == stringEnd)
        throw StarException("No specifier following '%'");
      else if (*next == '%')
        result += '%';
      else if (*next == 's')
        result += toString(popArg());
      else
        throw StarException::format("Improper lua log format specifier {}", (char)*next);
      ++next;
      stringIt = next;
    } else {
      result += *stringIt++;
    }
  }

  return result;
}

LuaCallbacks LuaBindings::makeUtilityCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("nrand", UtilityCallbacks::nrand);
  callbacks.registerCallback("makeUuid", UtilityCallbacks::makeUuid);
  callbacks.registerCallback("logInfo", UtilityCallbacks::logInfo);
  callbacks.registerCallback("logWarn", UtilityCallbacks::logWarn);
  callbacks.registerCallback("logError", UtilityCallbacks::logError);
  callbacks.registerCallback("setLogMap", UtilityCallbacks::setLogMap);
  callbacks.registerCallback("parseJson", UtilityCallbacks::parseJson);
  callbacks.registerCallback("printJson", UtilityCallbacks::printJson);
  callbacks.registerCallback("print", UtilityCallbacks::print);
  callbacks.registerCallback("interpolateSinEase", UtilityCallbacks::interpolateSinEase);
  callbacks.registerCallback("replaceTags", UtilityCallbacks::replaceTags);
  callbacks.registerCallback("parseJsonSequence", [](String const& json) { return Json::parseSequence(json); });
  callbacks.registerCallback("jsonMerge", [](Json const& a, Json const& b) { return jsonMerge(a, b); });
  callbacks.registerCallback("jsonQuery", [](Json const& json, String const& path, Json const& def) { return json.query(path, def); });
  callbacks.registerCallback("makeRandomSource", [](Maybe<uint64_t> seed) { return seed ? RandomSource(*seed) : RandomSource(); });
  callbacks.registerCallback("makePerlinSource", [](Json const& config) { return PerlinF(config); });

  callbacks.copyCallback("parseJson", "jsonFromString"); // SE compat

  auto hash64LuaValues = [](LuaVariadic<LuaValue> const& values) -> uint64_t {
    XXHash64 hash;

    for (auto const& value : values) {
      if (auto b = value.ptr<LuaBoolean>())
        xxHash64Push(hash, *b);
      else if (auto i = value.ptr<LuaInt>())
        xxHash64Push(hash, *i);
      else if (auto f = value.ptr<LuaFloat>())
        xxHash64Push(hash, *f);
      else if (auto s = value.ptr<LuaString>())
        xxHash64Push(hash, s->ptr());
      else
        throw LuaException("Unhashable lua type passed to staticRandomXX binding");
    }

    return hash.digest();
  };

  callbacks.registerCallback("staticRandomI32",
      [hash64LuaValues](LuaVariadic<LuaValue> const& hashValues) { return (int32_t)hash64LuaValues(hashValues); });

  callbacks.registerCallback("staticRandomI32Range",
      [hash64LuaValues](int32_t min, int32_t max, LuaVariadic<LuaValue> const& hashValues) {
        if (max < min)
          throw LuaException("Maximum bound in staticRandomI32Range must be >= minimum bound!");
        uint64_t denom = (uint64_t)(-1) / ((uint64_t)(max - min) + 1);
        return (int32_t)(hash64LuaValues(hashValues) / denom + min);
      });

  callbacks.registerCallback("staticRandomDouble",
      [hash64LuaValues](LuaVariadic<LuaValue> const& hashValues) {
        return (hash64LuaValues(hashValues) & 0x7fffffffffffffff) / 9223372036854775808.0;
      });

  callbacks.registerCallback("staticRandomDoubleRange",
      [hash64LuaValues](double min, double max, LuaVariadic<LuaValue> const& hashValues) {
        if (max < min)
          throw LuaException("Maximum bound in staticRandomDoubleRange must be >= minimum bound!");
        return (hash64LuaValues(hashValues) & 0x7fffffffffffffff) / 9223372036854775808.0 * (max - min) + min;
      });

  return callbacks;
}

double LuaBindings::UtilityCallbacks::nrand(Maybe<double> const& stdev, Maybe<double> const& mean) {
  return Random::nrandd(stdev.value(1.0), mean.value(0));
}

String LuaBindings::UtilityCallbacks::makeUuid() {
  return Uuid().hex();
}

void LuaBindings::UtilityCallbacks::logInfo(String const& str, LuaVariadic<LuaValue> const& args) {
  Logger::log(LogLevel::Info, formatLua(str, args).utf8Ptr());
}

void LuaBindings::UtilityCallbacks::logWarn(String const& str, LuaVariadic<LuaValue> const& args) {
  Logger::log(LogLevel::Warn, formatLua(str, args).utf8Ptr());
}

void LuaBindings::UtilityCallbacks::logError(String const& str, LuaVariadic<LuaValue> const& args) {
  Logger::log(LogLevel::Error, formatLua(str, args).utf8Ptr());
}

void LuaBindings::UtilityCallbacks::setLogMap(String const& key, String const& value, LuaVariadic<LuaValue> const& args) {
  LogMap::set(key, formatLua(value, args));
}

Json LuaBindings::UtilityCallbacks::parseJson(String const& str) {
  return Json::parse(str);
}

String LuaBindings::UtilityCallbacks::printJson(Json const& arg, Maybe<int> pretty) {
  return arg.repr(pretty.value());
}

String LuaBindings::UtilityCallbacks::print(LuaValue const& value) {
  return toString(value);
}

LuaValue LuaBindings::UtilityCallbacks::interpolateSinEase(LuaEngine& engine, double offset, LuaValue const& value1, LuaValue const& value2) {
  if (auto floatValue1 = engine.luaMaybeTo<double>(value1)) {
    auto floatValue2 = engine.luaMaybeTo<double>(value2);
    return sinEase(offset, *floatValue1, *floatValue2);
  } else {
    return engine.luaFrom<Vec2F>(sinEase(offset, engine.luaTo<Vec2F>(value1), engine.luaTo<Vec2F>(value2)));
  }
}

String LuaBindings::UtilityCallbacks::replaceTags(String const& str, StringMap<String> const& tags) {
  return str.replaceTags(tags);
}

}
