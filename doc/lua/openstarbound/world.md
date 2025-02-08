# World

The world table now contains extra bindings.

---

#### `bool` world.isServer()

Returns whether the script is running on the server or client.

---

#### `bool` world.isClient()

Returns whether the script is running on the server or client.

---

The following additional world bindings are available only for scripts running on the client.

---

#### `entityId` world.mainPlayer()

Returns the entity ID of the player hosting the world.

---

#### `Vec2F` world.entityAimPosition(`entityId` entityId)

Returns the current cursor aim position of the specified entity.

---

#### `bool` world.inWorld()

Returns whether any players are in the world.

---

The following additional world bindings are available only for scripts running on the server.

---

#### `void` world.setExpiryTime(`float` expiryTime)

Sets the amount of time to persist a ephemeral world when it is inactive.

---

#### `string` world.id()

Returns a `String` representation of the world's id.

---

#### `Maybe<LuaValue>` world.callScriptContext(`String` contextName, `String` function, `String` contextName, [LuaValue args ...])

Calls a function in the specified world script context.

---

#### `?` world.sendPacket(`?` ?)

?

---
