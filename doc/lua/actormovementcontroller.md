# actor mcontroller

The `mcontroller` table sometimes contains functions relating to the actor movement controller.

This section of mcontroller documentation refers to the ActorMovementController lua bindings. There is other documentation referring to the mcontroller table for base MovementControllers.

* monsters
* npcs
* tech
* companion system scripts
* status effects
* quest scripts
* active items

---

#### `RectF` mcontroller.boundBox()

Returns a rect containing the entire collision of the movement controller, in local coordinates.

---

#### `PolyF` mcontroller.collisionPoly()

Returns the collision poly of the movement controller, in local coordinates.

---

#### `PolyF` mcontroller.collisionBody()

Returns the collision poly of the movement controller, in world coordinates.

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

##### `void` mcontroller.setAnchorState(`EntityId` anchorableEntity, size_t anchorPosition)

Anchors the movement controller to an anchorable entity at the given anchor index.

---

##### `void` mcontroller.resetAnchorState()

Reset the anchor state.

---

##### `EntityId`, `int` mcontroller.anchorState()

Returns ID of anchored entity and index of the anchor position.

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

##### `ActorMovementParameters` mcontroller.baseParameters()

Returns the base movement parameters.

---

##### `bool` mcontroller.walking()

Returns whether the actor movement controller is currently walking.

---

##### `bool` mcontroller.running()

Returns whether the actor movement controller is currently running.

---

##### `int` mcontroller.movingDirection()

Returns the direction that the actor movement controller is currently moving in. -1 for left, 1 for right.

---

##### `int` mcontroller.facingDirection()

Returns the facing direction. -1 for left, 1 for right.

---

##### `bool` mcontroller.crouching()

Returns whether the controller is currently crouching.

---

##### `bool` mcontroller.flying()

Returns whether the controller is currently flying.

---

##### `bool` mcontroller.falling()

Returns whether the controller is currently falling.

---

##### `bool` mcontroller.canJump()

Returns whether the controller can currently jump.

---

##### `bool` mcontroller.jumping()

Returns whether the controller is currently jumping.

---

##### `bool` mcontroller.groundMovement()

Returns whether the controller is currently in a ground movement state. Movement controllers can be in ground movement even when onGround returns false.

---

##### `bool` mcontroller.liquidMovement()

Returns whether the controller is currently in liquid movement mode.

---

## controls

The actor movement controller has a set of controls. Controls can be set anywhere and are accumulated and evaluated after all scripts are run. Controls will either override previously set controls, or combine them.

Controls are either cleared before every script update, or can be set to be manually cleared.

---

##### `void` mcontroller.controlRotation(`float` rotation)

Rotates the controller. Each control adds to the previous one.

---

##### `void` mcontroller.controlAcceleration(`Vec2F` acceleration)

Controls acceleration. Each control adds to the previous one.

---

##### `void` mcontroller.controlForce()

Controls force. Each control adds to the previous one.

---

##### `void` mcontroller.controlApproachVelocity(`Vec2F` targetVelocity, `float` maxControlForce)

Approaches the targetVelocity using the force provided. If the current velocity is higher than the provided targetVelocity, the targetVelocity will still be approached, effectively slowing down the entity.

Each control overrides the previous one.

---

#### `void` mcontroller.controlApproachVelocityAlongAngle(`float` angle, `float` targetVelocity, `float` maxControlForce, `bool` positiveOnly = false)

Approaches the targetVelocity but only along the provided angle, not affecting velocity in the perpendicular axis. If positiveOnly, then it will not slow down the movementController if it is already moving faster than targetVelocity.

Each control overrides the previous one.

---

#### `void` mcontroller.controlApproachXVelocity(`float` targetVelocity, `float` maxControlForce)

Approaches an X velocity. Same as using approachVelocityAlongAngle with angle 0.

Each control overrides the previous one.

---

#### `void` mcontroller.controlApproachYVelocity(`float` targetVelocity, `float` maxControlForce)

Approaches a Y velocity. Same as using approachVelocityAlongAngle with angle (Pi / 2).

Each control overrides the previous one.

---

##### `void` mcontroller.controlParameters(`ActorMovementParameters` parameters)

Changes movement parameters. Parameters are merged into the base parameters.

Each control is merged into the previous one.

---

##### `void` mcontroller.controlModifiers(`ActorMovementModifiers` modifiers)

Changes movement modifiers. Modifiers are merged into the base modifiers.

Each control is merged into the previous one.

---

##### `void` mcontroller.controlMove(`float` direction, `bool` run)

Controls movement in a direction.

Each control replaces the previous one.

---

##### `void` mcontroller.controlFace(`float` direction)

Controls the facing direction.

Each control replaces the previous one.

---

##### `void` mcontroller.controlDown()

Controls dropping through platforms.

---

##### `void` mcontroller.controlCrouch()

Controls crouching.

---

##### `void` mcontroller.controlJump()

Controls starting a jump. Only has an effect if canJump is true.

---

##### `void` mcontroller.controlHoldJump()

Keeps holding jump. Will not trigger a new jump, and can be held in the air.

---

##### `void` mcontroller.controlFly(`Vec2F` direction)

Controls flying in the specified direction (or {0, 0} to stop) with the configured flightSpeed parameter.

Each control overrides the previous one.

---

##### `bool` mcontroller.autoClearControls()

Returns whether the controller is currently set to auto clear controls before each script update.

---

##### `void` mcontroller.setAutoClearControls(`bool` enabled)

Set whether to automatically clear controls before each script update.

---

##### `void` mcontroller.clearControls()

Manually clear all controls.
