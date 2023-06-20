The projectile table contains bindings specific to projectiles which are available in addition to their common tables.

---

#### `Json` projectile.getParameter(`String` parameter, `Json` default)

Returns the value for the specified config parameter. If there is no value set, returns the default.

---

#### `void` projectile.die()

Destroys the projectile.

---

#### `EntityId` projectile.sourceEntity()

Returns the entity id of the projectile's source entity, or `nil` if no source entity is set.

---

#### `float` projectile.powerMultiplier()

Returns the projectile's power multiplier.

---

#### `float` projectile.power()

Returns the projectile's power (damage).

---

#### `void` projectile.setPower(`float` power)

Sets the projectile's power (damage).

---

#### `float` projectile.timeToLive()

Returns the projectile's current remaining time to live.

---

#### `void` projectile.setTimeToLive(`float` timeToLive)

Sets the projectile's current remaining time to live. Altering the time to live may cause visual disparity between the projectile's master and slave entities.

---

#### `bool` projectile.collision()

Returns `true` if the projectile has collided and `false` otherwise.

---

#### `void` projectile.processAction(`Json` action)

Immediately performs the specified action. Action should be specified in a format identical to a single entry in e.g. actionOnReap in the projectile's configuration. This function will not properly perform rendering actions as they will not be networked.

---

#### 'void' projectile.setReferenceVelocity(Maybe<`Vec2F`> velocity)

Sets the projectile's reference velocity (a base velocity to which movement is relative)
