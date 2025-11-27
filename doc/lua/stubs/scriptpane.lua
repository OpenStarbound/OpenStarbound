---@meta

--- pane API
---@class pane
pane = {}

--- Returns the entity id of the pane's source entity. ---
---@return EntityId
function pane.sourceEntity() end

--- Closes the pane. ---
---@return void
function pane.dismiss() end

--- Plays the specified sound asset, optionally looping the specified number of times or at the specified volume. ---
---@param sound string
---@param loops number
---@param volume number
---@return void
function pane.playSound(sound, loops, volume) end

--- Stops all instances of the given sound asset, and returns `true` if any sounds were stopped and `false` otherwise. ---
---@param sound string
---@return boolean
function pane.stopAllSounds(sound) end

--- Sets the window title and subtitle. ---
---@param title string
---@param subtitle string
---@return void
function pane.setTitle(title, subtitle) end

--- Sets the window icon. ---
---@param image string
---@return void
function pane.setTitleIcon(image) end

--- Creates a new widget with the specified config and adds it to the pane, optionally with the specified name. ---
---@param widgetConfig Json
---@param widgetName string
---@return void
function pane.addWidget(widgetConfig, widgetName) end

--- Removes the specified widget from the pane.
---@param widgetName string
---@return void
function pane.removeWidget(widgetName) end
