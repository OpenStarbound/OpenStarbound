#pragma once

#include "StarConfig.hpp"
#include "StarGameTypes.hpp"
#include "StarUuid.hpp"

namespace Star {

STAR_CLASS(Universe);
STAR_CLASS(CelestialDatabase);

// Shared base class for UniverseClient and UniverseServer
class Universe {
public:
  struct Message {
    String message;
    JsonArray args;
    Variant<pair<Uuid,ConnectionId>,RpcPromiseKeeper<Json>> keeper;
  };
  
  virtual CelestialDatabasePtr celestialDatabase() = 0;
  
  virtual void passMessage(Message&& message) = 0;
  // Sends a universe message to the target universe's main universe script context.
  virtual RpcPromise<Json> sendUniverseMessage(ConnectionId const& connectionId, String const& message, JsonArray const& args = {}) = 0;
  
  virtual Maybe<ChainableJsonMessageResponse> receiveMessage(String const& message, bool const& local, JsonArray const& args) = 0;
};

}
