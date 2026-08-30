# World

The `world` table now contains extra bindings.

---

#### `bool` world.isServer()

Returns whether the script is running on the server or client.

---

#### `bool` world.isClient()

Returns whether the script is running on the server or client.

---

#### `void` world.wire(`Vec2I` outputPosition, `int` outputIndex, `Vec2I` inputPosition, `int` inputIndex)

Attempts a wire connection between the two objects positioned at `outputPosition` and `inputPosition`. The `outputIndex` and `inputIndex` parameters specify which output and input nodes to connect.

---

#### `Maybe<LuaValue>` world.callScriptContext(`String` contextName, `String` functionName, [`LuaValue` args ...])

Calls a function in the specified world script context.

---

#### `List<EntityId>` world.entities([`Json` options])

Queries the entire world for entities. Takes the same options as entity queries, but should be more efficient especially on very large worlds.
For server worlds, this returns every entity on the world with the given criteria, or every entity if none are provided.
For client worlds, this returns every entity that is loaded and known to the client.

---

#### `Entity` world.entity(`EntityId` id)

Returns the entity with the given id as a userdata. 
This has some additional methods that can be used to acquire data from the entity that cannot be obtained otherwise.

Clean this up properly when the entity no longer exists or is no longer being used.
Besides the client main player, Entity instances are not reused.
Holding an Entity instance in Lua will keep the entity present in memory even after its destruction.

---

The following additional world bindings are available only for scripts running on the client.

---

#### List<EntityId> world.players()

A client version of the server binding. Returns every player loaded on the client world.

---

#### `entityId` world.mainPlayer()

Returns the entity ID of the client world's main player. Returns `nil` if this is a subworld.

---

#### `Vec2F` world.entityAimPosition(`entityId` entityId)

Returns the current cursor aim position of the specified entity.

---

#### `bool` world.inWorld()

Returns if the client world is actually in a world.

---

#### `bool` world.headless()

Returns true if the client world is a subworld.

---

#### `ClientSubWorldId` world.subWorldId()

Returns the current world's subworld id. If the world is not a subworld, this is 0.
This is the same number visible in the `/debug` log map.

---

#### `void` world.unload()

If this is a subworld, requests to unload it.

---

The following additional world bindings are available only for scripts running on the server.

---

#### `void` world.setExpiryTime(`float` expiryTime)

Sets the amount of time to persist an ephemeral world when it is inactive.

---

#### `float` world.expiryTime()

Returns the amount of time an ephemeral world persists when it is inactive.

---

#### `String` world.id()

Returns a `String` representation of the world's id.

---

#### `bool` world.sendPacket(`ConnectionId` clientId, `String` packetType, `Json` packetData)

Sends a raw network packet to the specified client that is currently connected to this world. The `packetType` must be the name
of a known packet in `Star::PacketTypeNames` (for example `"EntityInteractResult"`) and `packetData` must contain the JSON
representation expected by `Star::createPacket` for that packet. Returns `true` when the packet is queued successfully.

---

#### `bool` world.replaceMaterials(`List<Vec2I>` positions, `String` layerName, `String` materialName, [`int` hueShift], [`bool` enableDrops])

Attempts to replace existing materials with the specified material in the specified positions and layer. Positions with no preexisting material will be skipped. Returns `true` if the placement succeeds and `false` otherwise.

---

#### `bool` world.replaceMaterialArea(`Vec2F` center, `float` radius, `String` layerName, `String` materialName, [`int` hueShift], [`bool` enableDrops])

Identical to world.replaceMaterials but applies to tiles in a circular radius around the specified center point.

---

#### `Json` world.biomeAt(`Vec2I` position)

Returns a json object containing configuation values for the biome generated at the given position.

---

#### `Json` world.blockInfoAt(`Vec2I` position)

Returns a json object containing information about the position in the world's template.
