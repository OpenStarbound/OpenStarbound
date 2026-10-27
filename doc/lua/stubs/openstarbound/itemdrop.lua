---@meta

--- itemDrop API
---@class itemDrop
itemDrop = {}

--- If the item drop is in the middle of being picked up, returns the entity it is being picked up by. ---
---@return EntityId
function itemDrop.takingEntity() end

--- Returns true if the item drop will never despawn. ---
---@return boolean
function itemDrop.eternal() end

--- Changes whether the item drop despawns or not. ---
---@param eternal boolean
---@return void
function itemDrop.setEternal(eternal) end

--- Returns how long until this item becomes tangible (can be picked up). ---
---@return float
function itemDrop.intangibleTime() end

--- Sets the time before this item drop becomes tangible (can be picked up). ---
---@param intangibleTime float
---@return void
function itemDrop.setIntangibleTime(intangibleTime) end

--- Returns the mode override. Can be Intangible, Available, Taken, and Dead. Returns nil if there is no override. ---
---@return Maybe<String>
function itemDrop.overrideMode() end

--- Sets the mode override. Can be Intangible, Available, Taken, and Dead. If no mode is specified, clears the mode override. ---
---@param overrideMode String
---@return void
function itemDrop.setOverrideMode(overrideMode) end
