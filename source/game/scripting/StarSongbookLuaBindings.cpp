#include "StarSongbookLuaBindings.hpp"
#include "StarLuaConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeSongbookCallbacks(Songbook* songbook) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<void, Json, String>("play", bind(mem_fn(&Songbook::play), songbook, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Vec2F>("keepAlive", bind(mem_fn(&Songbook::keepAlive), songbook, _1, _2));
  callbacks.registerCallbackWithSignature<void>("stop", bind(mem_fn(&Songbook::stop), songbook));
  callbacks.registerCallbackWithSignature<bool>("active", bind(mem_fn(&Songbook::active), songbook));
  callbacks.registerCallbackWithSignature<String>("band", bind(mem_fn(&Songbook::timeSource), songbook));
  callbacks.registerCallbackWithSignature<String>("instrument", bind(mem_fn(&Songbook::instrument), songbook));
  callbacks.registerCallbackWithSignature<bool>("instrumentPlaying", bind(mem_fn(&Songbook::instrumentPlaying), songbook));
  callbacks.registerCallbackWithSignature<Json>("song", bind(mem_fn(&Songbook::song), songbook));

  return callbacks;
}

}
