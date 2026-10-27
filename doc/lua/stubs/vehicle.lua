---@meta

--- vehicle API
---@class vehicle
vehicle = {}

--- Returns `true` if the specified control is currently being held by an occupant of the specified lounge position and `false` otherwise. ---
---@param loungeName string
---@param controlName string
---@return boolean
function vehicle.controlHeld(loungeName, controlName) end

--- Returns the world aim position for the specified lounge position. ---
---@param loungeName string
---@return Vec2F
function vehicle.aimPosition(loungeName) end

--- Returns the entity id of the entity currently occupying the specified lounge position, or `nil` if the lounge position is unoccupied. ---
---@param loungeName string
---@return EntityId
function vehicle.entityLoungingIn(loungeName) end

--- Enables or disables the specified lounge position. ---
---@param loungeName string
---@param enabled boolean
---@return void
function vehicle.setLoungeEnabled(loungeName, enabled) end

--- Sets the lounge orientation for the specified lounge position. Valid orientations are "sit", "stand" or "lay". ---
---@param loungeName string
---@param orientation string
---@return void
function vehicle.setLoungeOrientation(loungeName, orientation) end

--- Sets the emote to be performed by entities occupying the specified lounge position, or clears it if no emote is specified. ---
---@param loungeName string
---@param emote string
---@return void
function vehicle.setLoungeEmote(loungeName, emote) end

--- Sets the dance to be performed by entities occupying the specified lounge position, or clears it if no dance is specified. ---
---@param loungeName string
---@param dance string
---@return void
function vehicle.setLoungeDance(loungeName, dance) end

--- Sets the list of status effects to be applied to entities occupying the specified lounge position. To clear the effects, set an empty list. ---
---@param loungeName string
---@param statusEffects JsonArray
---@return void
function vehicle.setLoungeStatusEffects(loungeName, statusEffects) end

--- Sets whether the vehicle is persistent, i.e. whether it will be stored when the world is unloaded and reloaded. ---
---@param persistent boolean
---@return void
function vehicle.setPersistent(persistent) end

--- Sets whether the vehicle is currently interactive. ---
---@param interactive boolean
---@return void
function vehicle.setInteractive(interactive) end

--- Sets the vehicle's current damage team type and number. ---
---@param team DamageTeam
---@return void
function vehicle.setDamageTeam(team) end

--- Enables or disables the specified collision region. ---
---@param collisionName string
---@param enabled boolean
---@return void
function vehicle.setMovingCollisionEnabled(collisionName, enabled) end

--- Enables or disables the specified force region. ---
---@param regionName string
---@param enabled boolean
---@return void
function vehicle.setForceRegionEnabled(regionName, enabled) end

--- Enables or disables the specified damage source. ---
---@param damageSourceName string
---@param enabled boolean
---@return void
function vehicle.setDamageSourceEnabled(damageSourceName, enabled) end

--- Destroys the vehicle.
---@return void
function vehicle.destroy() end
