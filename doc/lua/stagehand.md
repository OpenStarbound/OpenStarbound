The stagehand table contains bindings specific to stagehands which are available in addition to their common tables.

---

#### `EntityId` stagehand.id()

Returns the stagehand's entity id. Identical to entity.id(), so use that instead.

---

#### `Vec2F` stagehand.position()

Returns the stagehand's position. This is identical to entity.position(), so use that instead.

---

#### `void` stagehand.setPosition(`Vec2F` position)

Moves the stagehand to the specified position.

---

#### `void` stagehand.die()

Destroys the stagehand.

---

#### `String` stagehand.typeName()

Returns the stagehand's type name.

---

#### `void` stagehand.setUniqueId([`String` uniqueId])

Sets the stagehand's unique entity id, or clears it if unspecified.
