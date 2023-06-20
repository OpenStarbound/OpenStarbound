The vehicle table contains bindings specific to vehicles which are available in addition to their common tables.

---

#### `bool` vehicle.controlHeld(`String` loungeName, `String` controlName)

Returns `true` if the specified control is currently being held by an occupant of the specified lounge position and `false` otherwise.

---

#### `Vec2F` vehicle.aimPosition(`String` loungeName)

Returns the world aim position for the specified lounge position.

---

#### `EntityId` vehicle.entityLoungingIn(`String` loungeName)

Returns the entity id of the entity currently occupying the specified lounge position, or `nil` if the lounge position is unoccupied.

---

#### `void` vehicle.setLoungeEnabled(`String` loungeName, `bool` enabled)

Enables or disables the specified lounge position.

---

#### `void` vehicle.setLoungeOrientation(`String` loungeName, `String` orientation)

Sets the lounge orientation for the specified lounge position. Valid orientations are "sit", "stand" or "lay".

---

#### `void` vehicle.setLoungeEmote(`String` loungeName, [`String` emote])

Sets the emote to be performed by entities occupying the specified lounge position, or clears it if no emote is specified.

---

#### `void` vehicle.setLoungeDance(`String` loungeName, [`String` dance])

Sets the dance to be performed by entities occupying the specified lounge position, or clears it if no dance is specified.

---

#### `void` vehicle.setLoungeStatusEffects(`String` loungeName, `JsonArray` statusEffects)

Sets the list of status effects to be applied to entities occupying the specified lounge position. To clear the effects, set an empty list.

---

#### `void` vehicle.setPersistent(`bool` persistent)

Sets whether the vehicle is persistent, i.e. whether it will be stored when the world is unloaded and reloaded.

---

#### `void` vehicle.setInteractive(`bool` interactive)

Sets whether the vehicle is currently interactive.

---

#### `void` vehicle.setDamageTeam(`DamageTeam` team)

Sets the vehicle's current damage team type and number.

---

#### `void` vehicle.setMovingCollisionEnabled(`String` collisionName, `bool` enabled)

Enables or disables the specified collision region.

---

#### `void` vehicle.setForceRegionEnabled(`String` regionName, `bool` enabled)

Enables or disables the specified force region.

---

#### `void` vehicle.setDamageSourceEnabled(`String` damageSourceName, `bool` enabled)

Enables or disables the specified damage source.

---

#### `void` vehicle.destroy()

Destroys the vehicle.
