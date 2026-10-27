---@meta

--- pane API
---@class pane
pane = {}

--- Returns the entity id of the container that this pane is connected to. ---
---@return EntityId
function pane.containerEntityId() end

--- Returns the entity id of the player that opened this pane. ---
---@return EntityId
function pane.playerEntityId() end

--- Closes the pane.
---@return void
function pane.dismiss() end
