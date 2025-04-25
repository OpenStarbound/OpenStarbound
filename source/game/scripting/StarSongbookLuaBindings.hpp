#pragma once

#include "StarLua.hpp"
#include "StarSongbook.hpp"

namespace Star {

namespace LuaBindings {
  LuaCallbacks makeSongbookCallbacks(Songbook* songbook);
}
}
