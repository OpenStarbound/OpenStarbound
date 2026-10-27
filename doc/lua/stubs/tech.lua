---@meta

--- tech API
---@class tech
tech = {}

--- Returns the current cursor aim position. ---
---@return Vec2F
function tech.aimPosition() end

--- Sets whether the tech should be visible. ---
---@param visible boolean
---@return void
function tech.setVisible(visible) end

--- Set the animation state of the player. Valid states: * "Stand" * "Fly" * "Fall" * "Sit" * "Lay" * "Duck" * "Walk" * "Run" * "Swim" ---
---@param state string
---@return void
function tech.setParentState(state) end

--- Sets the image processing directives for the player. ---
---@param directives string
---@return void
function tech.setParentDirectives(directives) end

--- Sets whether to make the player invisible. Will still show the tech. ---
---@param hidden boolean
---@return void
function tech.setParentHidden(hidden) end

--- Sets the position of the player relative to the tech. ---
---@param offset Vec2F
---@return void
function tech.setParentOffset(offset) end

--- Returns whether the player is lounging. ---
---@return boolean
function tech.parentLounging() end

--- Sets whether to suppress tool usage on the player. When tool usage is suppressed no items can be used.
---@param suppressed boolean
---@return void
function tech.setToolUsageSuppressed(suppressed) end
