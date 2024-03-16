#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Image);

template <>
struct LuaConverter<Image> : LuaUserDataConverter<Image> {};

template <>
struct LuaUserDataMethods<Image> {
  static LuaMethods<Image> make();
};

}
