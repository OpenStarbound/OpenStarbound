# entity

The *entity* table contains functions that are common among all entities. Every function refers to the entity the script context is running on.

Accessible in:

* companion system scripts
* quests
* tech
* primary status script
* status effects
* monsters
* npcs
* objects
* active items

---

#### `EntityId` entity.id()

Returns the id number of the entity.

---

#### `LuaTable` entity.damageTeam()

Returns a table of the entity's damage team type and team number. Ex: {type = "enemy", team = 0}

---

#### `bool` entity.isValidTarget(`EntityId` entityId)

Returns whether the provided entity is a valid target of the current entity. An entity is a valid target if they can be damaged, and in the case of monsters and NPCs if they are aggressive.

---

#### `Vec2F` entity.distanceToEntity(`EntityId` entityId)

Returns the vector distance from the current entity to the provided entity.

---

#### `bool` entity.entityInSight(`EntityId` entityId)

Returns whether the provided entity is in line of sight of the current entity.

---

#### `Vec2F` entity.position()

Returns the position of the current entity.

---

#### `String` entity.entityType()

Returns the  type of the current entity.

---

#### `String` entity.uniqueId()

Returns the unique ID of the entity. Returns nil if there is no unique ID.

---

#### `bool` entity.persistent()

Returns `true` if the entity is persistent (will be saved to disk on sector unload) or `false` otherwise.
