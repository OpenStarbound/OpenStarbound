---@meta

--- renderer API
---@class renderer
renderer = {}

--- Enables or disables a post process shader group. If save is true, this change is saved to configuration as well. ---
---@param group string
---@param enabled boolean
---@param save boolean
---@return void
function renderer.setPostProcessGroupEnabled(group, enabled, save) end

--- Returns true if the specified post process group is enabled. ---
---@param group string
---@return boolean
function renderer.postProcessGroupEnabled(group) end

--- Returns every post process group. Identical to grabbing them from client.config with root.assetJson. ---
---@return Json
function renderer.postProcessGroups() end

--- Sets the specified scriptable parameter of the specified shader effect to the provided value. This is accessed from the shader as a uniform and must be defined in the effect's configuration. ---
---@param effectName string
---@param parameterName string
---@param value Json
---@return Json
function renderer.setEffectParameter(effectName, parameterName, value) end

--- Returns the specified scriptable parameter of the specified shader effect.
---@param effectName string
---@param parameterName string
---@return Json
function renderer.getEffectParameter(effectName, parameterName) end
