---@meta

--- effect API
---@class effect
effect = {}

--- Returns the remaining duration of the status effect. ---
---@return number
function effect.duration() end

--- Adds the specified duration to the current remaining duration. ---
---@param duration number
---@return void
function effect.modifyDuration(duration) end

--- Immediately expire the effect, setting the duration to 0. ---
---@return void
function effect.expire() end

--- Returns the source entity id of the status effect, if any. ---
---@return EntityId
function effect.sourceEntity() end

--- Sets image processing directives for the entity the status effect is active on. ---
---@param directives string
---@return void
function effect.setParentDirectives(directives) end

--- Returns the value associated with the parameter name in the effect configuration. If no value is set, returns the default specified. ---
---@param name string
---@param def Json
---@return Json
function effect.getParameter(name, def) end

--- Adds a new stat modifier group and returns the ID created for the group. Stat modifier groups will stay active until the effect expires. Stat modifiers are of the format: ```lua { stat = "health", amount = 50 --OR baseMultiplier = 1.5 --OR effectiveMultiplier = 1.5 } ``` ---
---@param modifiers List<StatModifier>
---@return StatModifierGroupId
function effect.addStatModifierGroup(modifiers) end

--- Replaces the list of stat modifiers in a group with the specified modifiers. ---
---@param modifiers List<StatModifier>
---@return void
function effect.setStatModifierGroup(modifiers) end

--- Removes the specified stat modifier group.
---@param groupId StatModifierGroupId
---@return void
function effect.removeStatModifierGroup(groupId) end
