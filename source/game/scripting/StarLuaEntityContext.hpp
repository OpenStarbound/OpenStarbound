#pragma once

#include "StarLuaGameConverters.hpp"

namespace Star {

// can be replaced with an MVariant in case non-server non-entity world contexts are added in the future
// if no caller specified, caller origin connection is assumed to be server
typedef Maybe<Entity*> WorldCaller;
  
  
inline ConnectionId callerConnection(WorldCaller caller) {
    if (caller)
        return (*caller)->originConnection();
    return ServerConnectionId;
}

struct EntityContext {
    EntityPtr entity;
    WorldCaller caller;
};

template <>
struct LuaConverter<EntityContext> : LuaUserDataConverter<EntityContext> {};

template <>
struct LuaUserDataMethods<EntityContext> {
  static LuaMethods<EntityContext> make();
};

}
