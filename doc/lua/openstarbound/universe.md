# Universe

Server-side and client-side scripts gain access to a `universe` table that exposes helpers for worlds, as well as administrative helpers for connected clients on server-side scripts.

--- 

The following callbacks are only available in contexts that run on the server (such as world scripts or the command
processor).

---

#### `Maybe<String>` universe.uuidForClient(`ConnectionId` clientId)

Returns the UUID for the given client connection, or `nil` if the client is unknown.

---

#### `ConnectionId[]` universe.clientIds()

Returns a list of all currently connected client IDs.

---

#### `unsigned` universe.numberOfClients()

Returns the number of connected clients.

---

#### `bool` universe.isConnectedClient(`ConnectionId` clientId)

Returns whether the specified client ID is currently connected.

---

#### `String` universe.clientNick(`ConnectionId` clientId)

Returns the nickname associated with the client.

---

#### `Maybe<ConnectionId>` universe.findNick(`String` nick)

Returns the client ID for the supplied nickname if that player is connected.

---

#### `void` universe.adminBroadcast(`String` message)

Sends a server broadcast message to every connected client.

---

#### `void` universe.adminWhisper(`ConnectionId` clientId, `String` message)

Sends a direct whisper to the specified client.

---

#### `bool` universe.isAdmin(`ConnectionId` clientId)

Returns `true` if the client is flagged as a server administrator.

---

#### `bool` universe.isPvp(`ConnectionId` clientId)

Returns whether the client currently has PVP enabled.

---

#### `void` universe.setPvp(`ConnectionId` clientId, [`bool` enabled])

Sets or clears the PVP flag for the client. When `enabled` is omitted the flag defaults to `true`.

---

#### `bool` universe.isWorldActive(`String` worldId)

Returns `true` if the world identified by the string world ID is currently loaded on the server.

---

#### `String[]` universe.activeWorlds()

Returns a list of world IDs for every active world.

---

#### `RpcPromise<Json>` universe.sendWorldMessage(`String` worldId, `String` messageName, [`Json` args ...])

Queues a remote message to the specified world. `messageName` is dispatched to scripts running in that world and any additional
arguments are forwarded as JSON values.

---

#### `bool` universe.sendPacket(`ConnectionId` clientId, `String` packetType, `Json` packetData)

Sends a raw network packet to the specified client. See `world.sendPacket` for details on constructing packet payloads.

---

#### `String` universe.clientWorld(`ConnectionId` clientId)

Returns the world ID for the world that the client currently occupies.

---

#### `void` universe.disconnectClient(`ConnectionId` clientId, [`String` reason])

Disconnects the client, optionally providing a disconnect reason.

---

#### `void` universe.banClient(`ConnectionId` clientId, [`String` reason], `bool` banIp, `bool` banUuid, [`int` timeout])

Bans a client by connection ID. The `banIp` and `banUuid` parameters control which identifiers are recorded; `timeout` (in
seconds) performs a temporary ban when provided.

---

#### `void` universe.warpClient(`ConnectionId` clientId, `String` action, [`bool` deploy])

Warps a client to a given world.

---

#### `unsigned` universe.clientOpenProtocolVersion(`ConnectionId` clientId)

Returns the OpenProtocolVersion used for networking with the client.

---

#### `void` universe.createCustomWorld(`String` name, `Json` template)

Creates a new custom world with the given name from the given template.
Its world ID will be `CustomWorld:<name>`, and it will be stored under the server's `universe/custom_<name>.world`

---

#### `void` universe.createCustomWorldFromConfig(`String` name, `Json` config)

Creates a new custom world with the given name from the given config. This config functions similarly to instance world configs.
Otherwise, functionally similar to `universe.createCustomWorld`.

---

The following functions are available on all scripts on the client main thread.

---

#### `void` universe.createClientCustomWorld(`String` name, `Json` template)

Creates a new client custom world with the given name from the given template.
Its world ID will be `ClientCustomWorld:<client uuid>:<name>`, and it will be stored under the client's `universeclient/<name>.world`

---

#### `void` universe.createClientCustomWorldFromConfig(`String` name, `Json` config)

Creates a new custom world with the given name from the given config. This config functions similarly to instance world configs.
Otherwise, functionally similar to `universe.createClientCustomWorld`.

---

#### `unsigned` universe.clientUuid()

Returns the client uuid. This is the uuid of the player the client joined with, which is the uuid present in client custom world IDs and the client's shipworld ID.

---

#### `unsigned` universe.serverOpenProtocolVersion()

Returns the OpenProtocolVersion used for networking with the server.

---

#### `Vec2U` universe.playerCount()

Returns the server's current player count and maximum players.

---

#### `bool` universe.subWorldActive(`String` worldId)

Returns if a subworld is currently active or requested on the given world.

---

#### `void` universe.loadSubWorld(`String` worldId)

Loads the given world as a subworld if it is not loaded.
Subworlds are client worlds that are considered headless. 
They do not render, they do not have a main player, and they do not have a screen.
World script contexts are required to do anything with them.
`clientPresenceMaster` entities are required to load anything in them.
They automatically unload after a few seconds of no activity. (No entities, no incoming messages.)

They function independently of the client main world, persisting even if the player warps elsewhere. 
Subworld master entities are considered different master to client main world entities.

---

#### `void` universe.unloadSubWorld(`String` worldId)

If a subworld is active on the given world, unloads it.

---

#### `RpcThreadPromise<Json>` universe.sendSubWorldMessage(`String` worldId, `String` messageName, [`Json` args ...])

Loads the given world as a subworld if it is not loaded.
If it is loaded, sends a message to the subworld on the given world.

---

#### `RpcPromise<Json>` universe.sendMainWorldMessage(`String` messageName, [`Json` args ...])

Sends a message to the client's main world. The message is handled immediately, so the RpcPromise will immediately be finished once returned.

---

#### `LuaValue` universe.callScriptContext(`String` contextName, `String` function, [`LuaValue` args ...])

Calls a function on the given universe client script context.

---

#### `LuaValue` universe.callMainWorldScriptContext(`String` contextName, `String` function, [`LuaValue` args ...])

Calls a function on the given script context on the client's main world.


