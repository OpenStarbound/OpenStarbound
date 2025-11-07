---@meta

--- playerCompanions API
---@class playerCompanions
playerCompanions = {}

--- Returns a list of configurations for all companions of the specified type. ---
---@param companionType string
---@return JsonArray
function playerCompanions.getCompanions(companionType) end

--- Sets the player's companions of the specified type to the specified list of companion configurations.
---@param companionType string
---@param companions JsonArray
---@return void
function playerCompanions.setCompanions(companionType, companions) end
