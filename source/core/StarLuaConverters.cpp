#include "StarLuaConverters.hpp"
#include "StarColor.hpp"

namespace Star {

// Colors are 0-255 in Lua, but may be out of range for HDR.
// Snap values that are float noise away from a whole number so ordinary colors stay integers.
static float luaColorComponent(float f) {
  float scaled = f * 255.0f;
  float rounded = std::round(scaled);
  return std::abs(scaled - rounded) < 0.001f ? rounded : scaled;
}
LuaValue LuaConverter<Color>::from(LuaEngine& engine, Color const& c) {
  float r = luaColorComponent(c.redF());
  float g = luaColorComponent(c.greenF());
  float b = luaColorComponent(c.blueF());
  float a = luaColorComponent(c.alphaF());
  if (a == 255.0f)
    return engine.createArrayTable(initializer_list<float>{r, g, b});
  else
    return engine.createArrayTable(initializer_list<float>{r, g, b, a});
}

Maybe<Color> LuaConverter<Color>::to(LuaEngine& engine, LuaValue const& v) {
  if (auto t = v.ptr<LuaTable>()) {
    Maybe<float> r = engine.luaMaybeTo<float>(t->get(1));
    Maybe<float> g = engine.luaMaybeTo<float>(t->get(2));
    Maybe<float> b = engine.luaMaybeTo<float>(t->get(3));
    if (!r || !g || !b)
      return {};

    Color c = Color::rgbaf(*r/255.0f, *g/255.0f, *b/255.0f, 1.0f);

    if (Maybe<float> a = engine.luaMaybeTo<float>(t->get(4))) {
      if (!a)
        return {};
      c.setAlphaF(*a/255.0f);
    }

    return c;
  } else if (auto s = v.ptr<LuaString>()) {
    try {
      return Color(s->ptr());
    } catch (ColorException const&) {}
  }

  return {};
}

LuaValue LuaConverter<LuaCallbacks>::from(LuaEngine& engine, LuaCallbacks const& c) {
  auto table = engine.createTable(0, c.callbacks().size());
  for (auto& callback : c.callbacks())
    table.set(callback.first, engine.createWrappedFunction(callback.second));

  return table;
}

Maybe<LuaCallbacks> LuaConverter<LuaCallbacks>::to(LuaEngine&, LuaValue const&) {
  return {};
}

}
