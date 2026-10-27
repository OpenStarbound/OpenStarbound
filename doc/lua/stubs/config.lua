---@meta

--- config API
---@class config
config = {}

--- Returns the value for the specified config parameter. If there is no value set, returns the default.
---@param parameter string
---@param default Json
---@return Json
function config.getParameter(parameter, default) end
