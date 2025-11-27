---@meta

--- mcontroller API
---@class mcontroller
mcontroller = {}

--- Returns a table containing the movement parameters for the movement controller. ---
---@return MovementParameters
function mcontroller.parameters() end

--- Applies the given parameters to the movement controller. The provided parameters are merged into the current movement parameters. ---
---@param parameters Json
---@return void
function mcontroller.applyParameters(parameters) end

--- Resets movement parameters to their original state. ---
---@return void
function mcontroller.resetParameters() end

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

--- Returns the collision poly of the movement controller, in local coordinates. ---
---@return PolyF
function mcontroller.collisionPoly() end

--- Returns the collision poly of the movement controller, in world coordinates. ---
---@return PolyF
function mcontroller.collisionBody() end

--- Returns a rect containing the entire collision poly of the movement controller, in world coordinates. ---
---@return RectF
function mcontroller.collisionBoundBox() end

--- Returns a rect containing the entire collision of the movement controller, in local coordinates. ---
---@return RectF
function mcontroller.localBoundBox() end

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

--- Rotates the movement controller by an angle relative to its current angle. Angle is in radians. ---
---@param angle number
---@return void
function mcontroller.rotate(angle) end

--- Accelerates the movement controller by the given acceleration for one tick. ---
---@param acceleration Vec2F
---@return void
function mcontroller.accelerate(acceleration) end

--- Accelerates the movement controller by (force / mass) for one tick. ---
---@param force Vec2F
---@return void
function mcontroller.force(force) end

--- Approaches the targetVelocity using the force provided. If the current velocity is higher than the provided targetVelocity, the targetVelocity will still be approached, effectively slowing down the entity. ---
---@param targetVelocity Vec2F
---@param maxControlForce number
---@return void
function mcontroller.approachVelocity(targetVelocity, maxControlForce) end

--- Approaches the targetVelocity but only along the provided angle, not affecting velocity in the perpendicular axis. If positiveOnly, then it will not slow down the movementController if it is already moving faster than targetVelocity. ---
---@param angle number
---@param targetVelocity number
---@param maxControlForce number
---@param positiveOnly boolean
---@return void
function mcontroller.approachVelocityAlongAngle(angle, targetVelocity, maxControlForce, positiveOnly) end

--- Approaches an X velocity. Same as using approachVelocityAlongAngle with angle 0. ---
---@param targetVelocity number
---@param maxControlForce number
---@return void
function mcontroller.approachXVelocity(targetVelocity, maxControlForce) end

--- Approaches a Y velocity. Same as using approachVelocityAlongAngle with angle (Pi / 2).
---@param targetVelocity number
---@param maxControlForce number
---@return void
function mcontroller.approachYVelocity(targetVelocity, maxControlForce) end
