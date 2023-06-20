The monster table contains bindings specific to monsters which are available in addition to their common tables.

---

#### `String` monster.type()

Returns the monster's configured monster type.

---

#### `String` monster.seed()

Returns a string representation of the monster's random seed.

---

#### `Json` monster.uniqueParameters()

Returns a table of the monster's unique (override) parameters.

---

#### `unsigned` monster.familyIndex()

Returns the monster's family index.

---

#### `float` monster.level()

Returns the monster's level.

---

#### `void` monster.setDamageOnTouch(`bool` enabled)

Enables or disables the monster's touch damage.

---

#### `void` monster.setDamageSources([`List<DamageSource>` damageSources])

Sets the monster's active damage sources (or clears them if unspecified).

---

#### `void` monster.setDamageParts(`StringSet` damageParts)

Sets the monster's active damage parts. Damage parts must be defined in the monster's configuration parameters. A damage part specifies a damage source and an animation part to anchor the damage source to. The anchor part's transformation will be applied to the damage source's damage poly, and if a vector, the damage source's knockback.

```js
"animationDamageParts" : {
  "beam" : {
    "anchorPart" : "partName", // animation part to anchor the damage source to
    "checkLineCollision" : false, // optional, if the damage source is a line, check for collision along the line
    "bounces" : 0, // optional, if the damage source is a line and it checks for collision
    "damageSource" : {
      "line" : [ [0.0, 0.0], [5.0, 0.0] ],
      "damage" : 10,
      "damageSourceKind" : "default",
      "teamType" : "enemy",
      "teamNumber" : 2
    }
  }
}
```

```lua
monster.setDamageParts({"beam"}) -- sets the "beam" damage part active
```

---

#### `void` monster.setAggressive(`bool` aggressive)

Sets whether the monster is currently aggressive.

---

#### `void` monster.setDropPool(`Json` dropPool)

Sets the monster's drop pool, which determines the items that it will drop on death. This can be specified as the `String` name of a treasure pool, or as a `Map<String, String>` to specify different drop pools for different damage types. If specified as a map, the pool should contain a "default" entry for unhandled damage types.

---

#### `Vec2F` monster.toAbsolutePosition(`Vec2F` relativePosition)

Returns an absolute world position calculated from the given relative position.

---

#### `Vec2F` monster.mouthPosition()

Returns the world position of the monster's mouth.

---

#### `void` monster.flyTo(`Vec2F` position)

Causes the monster to controlFly toward the given world position.

---

#### `void` monster.setDeathParticleBurst([`String` particleEmitter)

Sets the name of the particle emitter (configured in the animation) to burst when the monster dies, or clears it if unspecified.

---

#### `void` monster.setDeathSound([`String` sound])

Sets the name of the sound (configured in the animation) to play when the monster dies, or clears it if unspecified.

---

#### `void` monster.setPhysicsForces(`List<PhysicsForceRegion>` forceRegions)

Sets a list of physics force regions that the monster will project, used for applying forces to other nearby entities. Set an empty list to clear the force regions.

---

#### `void` monster.setName(`String` name)

Sets the monster's name.

---

#### `void` monster.setDisplayNametag(`bool` enabled)

Sets whether the monster should display its nametag.

---

#### `bool` monster.say(`String` line, [`Map<String, String>` tags])

Causes the monster to say the line, optionally replacing any specified tags in the text. Returns `true` if anything is said (i.e. the line is not empty) and `false` otherwise.

---

#### `bool` monster.sayPortrait(`String` line, `String` portrait, [`Map<String, String>` tags])

Similar to monster.say, but uses a portrait chat bubble with the specified portrait image.

---

#### `void` monster.setDamageTeam(`DamageTeam` team)

Sets the monster's current damage team type and number.

---

#### `void` monster.setUniqueId([`String` uniqueId])

Sets the monster's unique entity id, or clears it if unspecified.

---

#### `void` monster.setDamageBar(`String` damageBarType)

Sets the type of damage bar that the monster should display. Valid options are "default", "none" and "special".

---

#### `void` monster.setInteractive(`bool` interactive)

Sets whether the monster is currently interactive.

---

#### `void` monster.setAnimationParameter(`String` key, `Json` value)

Sets a networked scripted animator parameter to be used in a client side rendering script using animationConfig.getParameter.
