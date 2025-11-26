#pragma once

#include "StarLua.hpp"

namespace Star::LuaBindings {

// Creates the http callback table for Lua. All callbacks return RpcPromise values
// so scripts can wait on asynchronous-style results even though the requests
// are currently resolved synchronously. The callbacks are only usable when
// safe.luaHttp.eabled is truq
LuaCallbacks makeHttpCallbacks(bool enabled);
}


