---@meta

--- mcontroller API
---@class mcontroller
mcontroller = {}

--- Returns a rect containing the entire collision of the movement controller, in local coordinates. ---
---@return RectF
function mcontroller.boundBox() end

--- Returns the collision poly of the movement controller, in local coordinates. ---
---@return PolyF
function mcontroller.collisionPoly() end

--- Returns the collision poly of the movement controller, in world coordinates. ---
---@return PolyF
function mcontroller.collisionBody() end

--- Returns the configured mass of the movement controller. ---
---@return number
function mcontroller.mass() end

--- Returns the current position of the movement controller. ---
---@return Vec2F
function mcontroller.position() end

--- Returns the current horizontal position of the movement controller. ---
---@return number
function mcontroller.xPosition() end

--- Returns the current vertical position of the movement controller. ---
---@return number
function mcontroller.yPosition() end

--- Returns the current velocity of the movement controller. ---
---@return Vec2F
function mcontroller.velocity() end

--- Returns the current horizontal speed of the movement controller. ---
---@return number
function mcontroller.xVelocity() end

--- Returns the current vertical speed of the movement controller. ---
---@return number
function mcontroller.yVelocity() end

--- Returns the current rotation of the movement controller in radians. ---
---@return number
function mcontroller.rotation() end

--- Returns whether the movement controller is currently colliding with world geometry or a PhysicsMovingCollision. ---
---@return boolean
function mcontroller.isColliding() end

--- Returns whether the movement controller is currently colliding with null world geometry. Null collision occurs in unloaded sectors. ---
---@return boolean
function mcontroller.isNullColliding() end

--- Returns whether the movement controller is currently stuck colliding. Movement controllers can stick if the `stickyCollision` movement parameter is set. ---
---@return boolean
function mcontroller.isCollisionStuck() end

--- Returns the angle that the movement controller is currently stuck at, in radians. ---
---@return number
function mcontroller.stickingDirection() end

--- Returns the percentage of the collision poly currently submerged in liquid; ---
---@return number
function mcontroller.liquidPercentage() end

--- Returns the liquid ID of the liquid that the movement controller is currently submerged in. If this is several liquids this returns the most plentiful one. ---
---@return LiquidId
function mcontroller.liquidId() end

--- Returns whether the movement controller is currently on ground. ---
---@return boolean
function mcontroller.onGround() end

--- Returns `true` if the movement controller is at a world position without gravity or if gravity has been disabled. ---
---@return boolean
function mcontroller.zeroG() end

--- Returns `true` if the movement controller is touching the bottom or the top (unless bottomOnly is specified) of the world. ---
---@param bottomOnly boolean
---@return boolean
function mcontroller.atWorldLimit(bottomOnly) end

--- Anchors the movement controller to an anchorable entity at the given anchor index. ---
---@param anchorableEntity EntityId
---@return void
function mcontroller.setAnchorState(anchorableEntity) end

--- Reset the anchor state. ---
---@return void
function mcontroller.resetAnchorState() end

--- Sets the position of the movement controller. ---
---@param position Vec2F
---@return void
function mcontroller.setPosition(position) end

--- Sets the horizontal position of the movement controller. ---
---@param x number
---@return void
function mcontroller.setXPosition(x) end

--- Sets the vertical position of the movement controller. ---
---@param y number
---@return void
function mcontroller.setYPosition(y) end

--- Moves the movement controller by the vector provided. ---
---@param direction Vec2F
---@return void
function mcontroller.translate(direction) end

--- Sets the velocity of the movement controller. ---
---@param velocity Vec2F
---@return void
function mcontroller.setVelocity(velocity) end

--- Sets the horizontal velocity of the movement controller. ---
---@param xVelocity Vec2F
---@return void
function mcontroller.setXVelocity(xVelocity) end

--- Sets the vertical velocity of the movement controller. ---
---@param yVelocity Vec2F
---@return void
function mcontroller.setYVelocity(yVelocity) end

--- Adds (momentum / mass) velocity to the movement controller. ---
---@param momentum Vec2F
---@return void
function mcontroller.addMomentum(momentum) end

--- Sets the rotation of the movement controller. Angle is in radians. ---
---@param angle number
---@return void
function mcontroller.setRotation(angle) end

--- Returns the base movement parameters. ---
---@return ActorMovementParameters
function mcontroller.baseParameters() end

--- Returns whether the actor movement controller is currently walking. ---
---@return boolean
function mcontroller.walking() end

--- Returns whether the actor movement controller is currently running. ---
---@return boolean
function mcontroller.running() end

--- Returns the direction that the actor movement controller is currently moving in. -1 for left, 1 for right. ---
---@return number
function mcontroller.movingDirection() end

--- Returns the facing direction. -1 for left, 1 for right. ---
---@return number
function mcontroller.facingDirection() end

--- Returns whether the controller is currently crouching. ---
---@return boolean
function mcontroller.crouching() end

--- Returns whether the controller is currently flying. ---
---@return boolean
function mcontroller.flying() end

--- Returns whether the controller is currently falling. ---
---@return boolean
function mcontroller.falling() end

--- Returns whether the controller can currently jump. ---
---@return boolean
function mcontroller.canJump() end

--- Returns whether the controller is currently jumping. ---
---@return boolean
function mcontroller.jumping() end

--- Returns whether the controller is currently in a ground movement state. Movement controllers can be in ground movement even when onGround returns false. ---
---@return boolean
function mcontroller.groundMovement() end

--- Returns whether the controller is currently in liquid movement mode. --- ## controls The actor movement controller has a set of controls. Controls can be set anywhere and are accumulated and evaluated after all scripts are run. Controls will either override previously set controls, or combine them. Controls are either cleared before every script update, or can be set to be manually cleared. ---
---@return boolean
function mcontroller.liquidMovement() end

--- Rotates the controller. Each control adds to the previous one. ---
---@param rotation number
---@return void
function mcontroller.controlRotation(rotation) end

--- Controls acceleration. Each control adds to the previous one. ---
---@param acceleration Vec2F
---@return void
function mcontroller.controlAcceleration(acceleration) end

--- Controls force. Each control adds to the previous one. ---
---@return void
function mcontroller.controlForce() end

--- Approaches the targetVelocity using the force provided. If the current velocity is higher than the provided targetVelocity, the targetVelocity will still be approached, effectively slowing down the entity. Each control overrides the previous one. ---
---@param targetVelocity Vec2F
---@param maxControlForce number
---@return void
function mcontroller.controlApproachVelocity(targetVelocity, maxControlForce) end

--- Approaches the targetVelocity but only along the provided angle, not affecting velocity in the perpendicular axis. If positiveOnly, then it will not slow down the movementController if it is already moving faster than targetVelocity. Each control overrides the previous one. ---
---@param angle number
---@param targetVelocity number
---@param maxControlForce number
---@param positiveOnly boolean
---@return void
function mcontroller.controlApproachVelocityAlongAngle(angle, targetVelocity, maxControlForce, positiveOnly) end

--- Approaches an X velocity. Same as using approachVelocityAlongAngle with angle 0. Each control overrides the previous one. ---
---@param targetVelocity number
---@param maxControlForce number
---@return void
function mcontroller.controlApproachXVelocity(targetVelocity, maxControlForce) end

--- Approaches a Y velocity. Same as using approachVelocityAlongAngle with angle (Pi / 2). Each control overrides the previous one. ---
---@param targetVelocity number
---@param maxControlForce number
---@return void
function mcontroller.controlApproachYVelocity(targetVelocity, maxControlForce) end

--- Changes movement parameters. Parameters are merged into the base parameters. Each control is merged into the previous one. ---
---@param parameters ActorMovementParameters
---@return void
function mcontroller.controlParameters(parameters) end

--- Changes movement modifiers. Modifiers are merged into the base modifiers. Each control is merged into the previous one. ---
---@param modifiers ActorMovementModifiers
---@return void
function mcontroller.controlModifiers(modifiers) end

--- Controls movement in a direction. Each control replaces the previous one. ---
---@param direction number
---@param run boolean
---@return void
function mcontroller.controlMove(direction, run) end

--- Controls the facing direction. Each control replaces the previous one. ---
---@param direction number
---@return void
function mcontroller.controlFace(direction) end

--- Controls dropping through platforms. ---
---@return void
function mcontroller.controlDown() end

--- Controls crouching. ---
---@return void
function mcontroller.controlCrouch() end

--- Controls starting a jump. Only has an effect if canJump is true. ---
---@return void
function mcontroller.controlJump() end

--- Keeps holding jump. Will not trigger a new jump, and can be held in the air. ---
---@return void
function mcontroller.controlHoldJump() end

--- Controls flying in the specified direction (or {0, 0} to stop) with the configured flightSpeed parameter. Each control overrides the previous one. ---
---@param direction Vec2F
---@return void
function mcontroller.controlFly(direction) end

--- Returns whether the controller is currently set to auto clear controls before each script update. ---
---@return boolean
function mcontroller.autoClearControls() end

--- Set whether to automatically clear controls before each script update. ---
---@param enabled boolean
---@return void
function mcontroller.setAutoClearControls(enabled) end

--- Manually clear all controls.
---@return void
function mcontroller.clearControls() end
