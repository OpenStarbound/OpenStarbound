---@meta

--- physics API
---@class physics
physics = {}

--- Enables or disables the specified physics force region. ---
---@param force string
---@param enabled boolean
---@return void
function physics.setForceEnabled(force, enabled) end

--- Moves the specified physics collision region to the specified position. ---
---@param collision string
---@param position Vec2F
---@return void
function physics.setCollisionPosition(collision, position) end

--- Enables or disables the specified physics collision region.
---@param collision string
---@param enabled boolean
---@return void
function physics.setCollisionEnabled(collision, enabled) end
