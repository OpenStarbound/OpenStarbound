
# Item Drop

The `itemDrop` table is accessible from scripted item drops.

---

#### `Maybe<EntityId>` itemDrop.takingEntity()

If the item drop is in the middle of being picked up, returns the entity it is being picked up by.

---

#### `bool` itemDrop.eternal()

Returns true if the item drop will never despawn.

---

#### `void` itemDrop.setEternal(`bool` eternal)

Changes whether the item drop despawns or not.

---

#### `float` itemDrop.intangibleTime()

Returns how long until this item becomes tangible (can be picked up).

---

#### `void` itemDrop.setIntangibleTime(`float` intangibleTime)

Sets the time before this item drop becomes tangible (can be picked up).

---

#### `Maybe<String>` itemDrop.overrideMode()

Returns the mode override.
Can be Intangible, Available, Taken, and Dead. Returns nil if there is no override.

---

#### `void` itemDrop.setOverrideMode([String mode])

Sets the mode override.
Can be Intangible, Available, Taken, and Dead.
If no mode is specified, clears the mode override.
