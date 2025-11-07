---@meta

--- pane API
---@class pane
pane = {}

--- Returns the root widget of the pane as a table of widget callbacks, equivalent to calling `widget` helpers on the pane itself. This is especially useful when a script pane needs to call functions such as `widget.getChildAt` on its root. ---
---@return LuaCallbacks
function pane.toWidget() end

--- Returns the pane's current on-screen position. ---
---@return Vec2I
function pane.getPosition() end

--- Moves the pane to the specified screen position. ---
---@param position Vec2I
---@return void
function pane.setPosition(position) end

--- Returns the pane's current size in pixels. ---
---@return Vec2I
function pane.getSize() end

--- Resizes the pane to the supplied dimensions. ---
---@param size Vec2I
---@return void
function pane.setSize(size) end

--- Returns the interface scale currently applied to the pane. This mirrors `interface.scale()` but is available directly on the pane object. ---
---@return number
function pane.scale() end

--- Returns `true` if the pane is currently being displayed. ---
---@return boolean
function pane.isDisplayed() end

--- Returns `true` if the pane currently has keyboard focus. ---
---@return boolean
function pane.hasFocus() end

--- Shows the pane if it is hidden. ---
---@return void
function pane.show() end

--- Hides the pane if it is currently shown. ---
---@return void
function pane.hide() end

--- In vanilla Starbound this function returns nothing. OpenStarbound now returns widget callbacks for the newly created widget, allowing scripts to immediately operate on it without a separate `widget` lookup. ---
---@param config Json
---@param widgetName string
---@return LuaCallbacks
function pane.addWidget(config, widgetName) end

--- `pane.removeWidget` now returns whether the named widget existed and was removed.
---@param widgetName string
---@return boolean
function pane.removeWidget(widgetName) end
