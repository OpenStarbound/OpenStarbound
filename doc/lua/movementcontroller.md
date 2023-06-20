# mcontroller

The `mcontroller` table contains functions relating to the movement controller.

This section of mcontroller documentation refers to the MovementController lua bindings. Other documentation may refer to ActorMovementController lua bindings. MovementController is used in:

* projectiles
* vehicles

---

#### `MovementParameters` mcontroller.parameters()

Returns a table containing the movement parameters for the movement controller.

---

#### `void` mcontroller.applyParameters(`Json` parameters)

Applies the given parameters to the movement controller. The provided parameters are merged into the current movement parameters.

---

#### `void` mcontroller.resetParameters()

Resets movement parameters to their original state.

---

#### `float` mcontroller.mass()

Returns the configured mass of the movement controller.

---

#### `Vec2F` mcontroller.position()

Returns the current position of the movement controller.

---

#### `float` mcontroller.xPosition()

Returns the current horizontal position of the movement controller.

---

#### `float` mcontroller.yPosition()

Returns the current vertical position of the movement controller.

---

#### `Vec2F` mcontroller.velocity()

Returns the current velocity of the movement controller.

---

#### `float` mcontroller.xVelocity()

Returns the current horizontal speed of the movement controller.

---

#### `float` mcontroller.yVelocity()

Returns the current vertical speed of the movement controller.

---

#### `float` mcontroller.rotation()

Returns the current rotation of the movement controller in radians.

---

#### `PolyF` mcontroller.collisionPoly()

Returns the collision poly of the movement controller, in local coordinates.

---

#### `PolyF` mcontroller.collisionBody()

Returns the collision poly of the movement controller, in world coordinates.

---

#### `RectF` mcontroller.collisionBoundBox()

Returns a rect containing the entire collision poly of the movement controller, in world coordinates.

---

#### `RectF` mcontroller.localBoundBox()

Returns a rect containing the entire collision of the movement controller, in local coordinates.

---

#### `bool` mcontroller.isColliding()

Returns whether the movement controller is currently colliding with world geometry or a PhysicsMovingCollision.

---

#### `bool` mcontroller.isNullColliding()

Returns whether the movement controller is currently colliding with null world geometry. Null collision occurs in unloaded sectors.

---

#### `bool` mcontroller.isCollisionStuck()

Returns whether the movement controller is currently stuck colliding. Movement controllers can stick if the `stickyCollision` movement parameter is set.

---

#### `float` mcontroller.stickingDirection()

Returns the angle that the movement controller is currently stuck at, in radians.

---

#### `float` mcontroller.liquidPercentage()

Returns the percentage of the collision poly currently submerged in liquid;

---

#### `LiquidId` mcontroller.liquidId()

Returns the liquid ID of the liquid that the movement controller is currently submerged in. If this is several liquids this returns the most plentiful one.

---

#### `bool` mcontroller.onGround()

Returns whether the movement controller is currently on ground.

---

#### `bool` mcontroller.zeroG()

Returns `true` if the movement controller is at a world position without gravity or if gravity has been disabled.

---

#### `bool` mcontroller.atWorldLimit([`bool` bottomOnly])

Returns `true` if the movement controller is touching the bottom or the top (unless bottomOnly is specified) of the world.

---

#### `void` mcontroller.setPosition(`Vec2F` position)

Sets the position of the movement controller.

---

#### `void` mcontroller.setXPosition(`float` x)

Sets the horizontal position of the movement controller.

---

#### `void` mcontroller.setYPosition(`float` y)

Sets the vertical position of the movement controller.

---

#### `void` mcontroller.translate(`Vec2F` direction)

Moves the movement controller by the vector provided.

---

#### `void` mcontroller.setVelocity(`Vec2F` velocity)

Sets the velocity of the movement controller.

---

#### `void` mcontroller.setXVelocity(`Vec2F` xVelocity)

Sets the horizontal velocity of the movement controller.

---

#### `void` mcontroller.setYVelocity(`Vec2F` yVelocity)

Sets the vertical velocity of the movement controller.

---

#### `void` mcontroller.addMomentum(`Vec2F` momentum)

Adds (momentum / mass) velocity to the movement controller.

---

#### `void` mcontroller.setRotation(`float` angle)

Sets the rotation of the movement controller. Angle is in radians.

---

#### `void` mcontroller.rotate(`float` angle)

Rotates the movement controller by an angle relative to its current angle. Angle is in radians.

---

#### `void` mcontroller.accelerate(`Vec2F` acceleration)

Accelerates the movement controller by the given acceleration for one tick.

---

#### `void` mcontroller.force(`Vec2F` force)

Accelerates the movement controller by (force / mass) for one tick.

---

#### `void` mcontroller.approachVelocity(`Vec2F` targetVelocity, `float` maxControlForce)

Approaches the targetVelocity using the force provided. If the current velocity is higher than the provided targetVelocity, the targetVelocity will still be approached, effectively slowing down the entity.

---

#### `void` mcontroller.approachVelocityAlongAngle(`float` angle, `float` targetVelocity, `float` maxControlForce, `bool` positiveOnly = false)

Approaches the targetVelocity but only along the provided angle, not affecting velocity in the perpendicular axis. If positiveOnly, then it will not slow down the movementController if it is already moving faster than targetVelocity.

---

#### `void` mcontroller.approachXVelocity(`float` targetVelocity, `float` maxControlForce)

Approaches an X velocity. Same as using approachVelocityAlongAngle with angle 0.

---

#### `void` mcontroller.approachYVelocity(`float` targetVelocity, `float` maxControlForce)

Approaches a Y velocity. Same as using approachVelocityAlongAngle with angle (Pi / 2).
