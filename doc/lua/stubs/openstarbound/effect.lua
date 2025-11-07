---@meta

--- effect API
---@class effect
effect = {}

--- Returns the effect asset name associated with the unique effect instance. ---
---@return string
function effect.name() end

--- Sets the remaining duration of the effect to **duration** seconds when the duration is tracked. ---
---@param duration number
---@return void
function effect.setDuration(duration) end

--- Toggles whether this effect suppresses tool usage for the entity it is applied to. ---
---@param suppressed boolean
---@return void
function effect.setToolUsageSuppressed(suppressed) end
