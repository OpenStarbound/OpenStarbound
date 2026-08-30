# Hooks

OpenStarbound adds several new script contexts as well as Lua hooks for these contexts.

---

## Player

Some additional callbacks are added to generic player script contexts.

---

#### `void` postUpdate()

Invoked on update, after everything else on the player updates.

---

#### `void` refreshHumanoidParameters()

Invoked on humanoid parameters being refreshed.

---

## UniverseServer

UniverseServer script contexts can be created by adding them to `universe_server.config` under `scriptContexts`, functionally similar to generic player scripts.

---

#### `void` acceptConnection(`ConnectionId` clientId)

Invoked when a client connects to the server.

---

#### `void` doDisconnection(`ConnectionId` clientId)

Invoked when a client disconnects from the server.

---

#### `Json` overrideWarp(`Json` warpAction, `ConnectionId` clientId, `bool` deploy)

Invoked when a client warps anywhere.
If a value is returned, the warp is overridden:
    `worldId` specifies which world the client is overridden to warp to. If unspecified, defaults to the client's current world.
    `spawnTarget` specifies a spawn target.

---

## UniverseClient

UniverseClient script contexts can be created by adding them to `client.config` under `universeScriptContexts`, functionally similar to generic player scripts.

---

#### `void` clientCustomWorldLoaded(`String` name, `String` worldId)

Invoked when one of this client's custom worlds is loaded, if the server is new enough to notify this client.

---

#### `void` shipWorldLoaded(`String` worldId)

Invoked when this client's ship world is loaded, if the server is new enough to notify this client.

---

#### `void` serverWorldLoaded(`String` worldId)

Invoked when any world is loaded on the server, if the server is configured to notify this client.

---

#### `void` subWorldRejected(`ClientSubWorldId` subWorldId)

Invoked when a requested subworld is rejected.

---

## WorldServer

WorldServer script contexts can be created by adding them to `worldserver.config` under `scriptContexts`, functionally similar to generic player scripts.

---

#### `void` addClient(`ConnectionId` clientId, `bool` isLocal)

Invoked when a client or subworld enters the world.
Subworlds have the `ConnectionId` of their client + 16383.

---

#### `void` removeClient(`ConnectionId` clientId)

Invoked when a client or subworld leaves the world.

---

## WorldClient

WorldClient script contexts can be created by adding them to `client.config` under `worldScriptContexts`, `mainWorldScriptContexts`, and `subWorldScriptContexts`.
They are functionally similar to generic player scripts.

---

#### `void` preUninit()

Invoked when the world is unloading, prior to entities being removed.
