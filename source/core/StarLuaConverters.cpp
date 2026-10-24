#include "StarLuaConverters.hpp"
#include "StarColor.hpp"

namespace Star {

LuaValue LuaConverter<Color>::from(LuaEngine& engine, Color const& c) {
  if (c.alpha() == 255)
    return engine.createArrayTable(initializer_list<float>{c.redF()*255.0f, c.greenF()*255.0f, c.blueF()*255.0f});
  else
    return engine.createArrayTable(initializer_list<float>{c.redF()*255.0f, c.greenF()*255.0f, c.blueF()*255.0f, c.alphaF()*255.0f});
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
