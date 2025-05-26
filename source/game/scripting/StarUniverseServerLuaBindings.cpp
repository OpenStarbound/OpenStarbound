#include "StarUniverseServerLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarUniverseServer.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeUniverseServerCallbacks(UniverseServer* universe) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<Maybe<String>, ConnectionId>("uuidForClient", bind(UniverseServerCallbacks::uuidForClient, universe, _1));
  callbacks.registerCallbackWithSignature<List<ConnectionId>>("clientIds", bind(UniverseServerCallbacks::clientIds, universe));
  callbacks.registerCallbackWithSignature<size_t>("numberOfClients", bind(UniverseServerCallbacks::numberOfClients, universe));
  callbacks.registerCallbackWithSignature<bool, ConnectionId>("isConnectedClient", bind(UniverseServerCallbacks::isConnectedClient, universe, _1));
  callbacks.registerCallbackWithSignature<String, ConnectionId>("clientNick", bind(UniverseServerCallbacks::clientNick, universe, _1));
  callbacks.registerCallbackWithSignature<Maybe<ConnectionId>, String>("findNick", bind(UniverseServerCallbacks::findNick, universe, _1));
  callbacks.registerCallbackWithSignature<void, String>("adminBroadcast", bind(UniverseServerCallbacks::adminBroadcast, universe, _1));
  callbacks.registerCallbackWithSignature<void, ConnectionId, String>("adminWhisper", bind(UniverseServerCallbacks::adminWhisper, universe, _1, _2));
  callbacks.registerCallbackWithSignature<bool, ConnectionId>("isAdmin", bind(UniverseServerCallbacks::isAdmin, universe, _1));
  callbacks.registerCallbackWithSignature<bool, ConnectionId>("isPvp", bind(UniverseServerCallbacks::isPvp, universe, _1));
  callbacks.registerCallbackWithSignature<void, ConnectionId, bool>("setPvp", bind(UniverseServerCallbacks::setPvp, universe, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String>("isWorldActive", bind(UniverseServerCallbacks::isWorldActive, universe, _1));
  callbacks.registerCallbackWithSignature<StringList>("activeWorlds", bind(UniverseServerCallbacks::activeWorlds, universe));
  callbacks.registerCallbackWithSignature<RpcThreadPromise<Json>, String, String, LuaVariadic<Json>>("sendWorldMessage", bind(UniverseServerCallbacks::sendWorldMessage, universe, _1, _2, _3));
  callbacks.registerCallbackWithSignature<bool, ConnectionId, String, Json>("sendPacket", bind(UniverseServerCallbacks::sendPacket, universe, _1, _2, _3));
  callbacks.registerCallbackWithSignature<String, ConnectionId>("clientWorld", bind(UniverseServerCallbacks::clientWorld, universe, _1));
  callbacks.registerCallbackWithSignature<void, ConnectionId, String>("disconnectClient", bind(UniverseServerCallbacks::disconnectClient, universe, _1, _2));

  return callbacks;
}

// Gets a list of client ids
//
// @return A list of numerical client IDs.
Maybe<String> LuaBindings::UniverseServerCallbacks::uuidForClient(UniverseServer* universe, ConnectionId arg1) {
  return universe->uuidForClient(arg1).apply([](Uuid const& str) { return str.hex(); });
}

// Gets a list of client ids
//
// @return A list of numerical client IDs.
List<ConnectionId> LuaBindings::UniverseServerCallbacks::clientIds(UniverseServer* universe) {
  return universe->clientIds();
}

// Gets the number of logged in clients
//
// @return An integer containing the number of logged in clients
size_t LuaBindings::UniverseServerCallbacks::numberOfClients(UniverseServer* universe) {
  return universe->numberOfClients();
}

// Returns whether or not the provided client ID is currently connected
//
// @param clientId the client ID in question
// @return A bool that is true if the client is connected and false otherwise
bool LuaBindings::UniverseServerCallbacks::isConnectedClient(UniverseServer* universe, ConnectionId arg1) {
  return universe->isConnectedClient(arg1);
}

// Returns the nickname for the given client ID
//
// @param clientId the client ID in question
// @return A string containing the nickname of the given client
String LuaBindings::UniverseServerCallbacks::clientNick(UniverseServer* universe, ConnectionId arg1) {
  return universe->clientNick(arg1);
}

// Returns the client ID for the given nick
//
// @param nick the nickname of the client to search for
// @return An integer containing the clientID of the nick in question
Maybe<ConnectionId> LuaBindings::UniverseServerCallbacks::findNick(UniverseServer* universe, String const& arg1) {
  return universe->findNick(arg1);
}

// Sends a message to all logged in clients
//
// @param message the message to broadcast
// @return nil
void LuaBindings::UniverseServerCallbacks::adminBroadcast(UniverseServer* universe, String const& arg1) {
  universe->adminBroadcast(arg1);
}

// Sends a message to a specific client
//
// @param clientId the client id to whisper
// @param message the message to whisper
// @return nil
void LuaBindings::UniverseServerCallbacks::adminWhisper(UniverseServer* universe, ConnectionId arg1, String const& arg2) {
  ConnectionId client = arg1;
  String message = arg2;
  universe->adminWhisper(client, message);
}

// Returns whether or not a specific client is flagged as an admin
//
// @param clientId the client id to check
// @return a boolean containing true if the client is an admin, false otherwise
bool LuaBindings::UniverseServerCallbacks::isAdmin(UniverseServer* universe, ConnectionId arg1) {
  return universe->isAdmin(arg1);
}

// Returns whether or not a specific client is flagged as pvp
//
// @param clientId the client id to check
// @return a boolean containing true if the client is flagged as pvp, false
// otherwise
bool LuaBindings::UniverseServerCallbacks::isPvp(UniverseServer* universe, ConnectionId arg1) {
  return universe->isPvp(arg1);
}

// Set (or unset) the pvp status of a specific user
//
// @param clientId the client id to check
// @param setPvp set pvp status to this bool, defaults to true
// @return nil
void LuaBindings::UniverseServerCallbacks::setPvp(UniverseServer* universe, ConnectionId arg1, Maybe<bool> arg2) {
  ConnectionId client = arg1;
  bool setPvpTo = arg2.value(true);
  universe->setPvp(client, setPvpTo);
}

bool LuaBindings::UniverseServerCallbacks::isWorldActive(UniverseServer* universe, String const& worldId) {
  return universe->isWorldActive(parseWorldId(worldId));
}

StringList LuaBindings::UniverseServerCallbacks::activeWorlds(UniverseServer* universe) {
  return universe->activeWorlds().transformed(printWorldId);
}

RpcThreadPromise<Json> LuaBindings::UniverseServerCallbacks::sendWorldMessage(UniverseServer* universe, String const& worldId, String const& message, LuaVariadic<Json> args) {
  return universe->sendWorldMessage(parseWorldId(worldId), message, JsonArray::from(std::move(args)));
}

bool LuaBindings::UniverseServerCallbacks::sendPacket(UniverseServer* universe, ConnectionId clientId, String const& packetTypeName, Json const& args) {
  auto packetType = PacketTypeNames.getLeft(packetTypeName);
  auto packet = createPacket(packetType, args);
  return universe->sendPacket(clientId, packet);
}

String LuaBindings::UniverseServerCallbacks::clientWorld(UniverseServer* universe, ConnectionId clientId) {
  return printWorldId(universe->clientWorld(clientId));
}

void LuaBindings::UniverseServerCallbacks::disconnectClient(UniverseServer* universe, ConnectionId clientId, Maybe<String> const& reason) {
  return universe->disconnectClient(clientId, reason.value());
}

}
