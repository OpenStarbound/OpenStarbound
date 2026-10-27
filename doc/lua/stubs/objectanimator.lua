---@meta

--- objectAnimator API
---@class objectAnimator
objectAnimator = {}

--- Returns the value for the specified object parameter. If there is no value set, returns the default. ---
---@param parameter string
---@param default Json
---@return Json
function objectAnimator.getParameter(parameter, default) end

--- Returns the object's facing direction. This will be 1 for right or -1 for left. ---
---@return number
function objectAnimator.direction() end

--- Returns the object's tile position.
---@return Vec2F
function objectAnimator.position() end
