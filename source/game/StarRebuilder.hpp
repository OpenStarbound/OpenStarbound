#pragma once
#include "StarJson.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(LuaContext);
STAR_CLASS(LuaRoot);

class Rebuilder {
public:
  Rebuilder(String const& id);
  ~Rebuilder() = default;

  typedef function<String(Json const&)> AttemptCallback;
  bool rebuild(Json store, String last_error, AttemptCallback attempt) const;

private:
  LuaRootPtr m_luaRoot;
  mutable RecursiveMutex m_luaMutex;
  shared_ptr<List<LuaContext>> m_contexts; // this is a ptr to avoid having to include Lua.hpp here
};

}