---@meta

--- input API
---@class input
input = {}

--- If this bind was pressed this frame, returns how many times it has been pressed. Returns `nil` otherwise. ---
---@param categoryId string
---@param bindId string
---@return Maybe<unsigned>
function input.bindDown(categoryId, bindId) end

--- Returns `true` if this bind is currently held. ---
---@param categoryId string
---@param bindId string
---@return boolean
function input.bindHeld(categoryId, bindId) end

--- If this bind was released this frame, returns how many times it has been released. Returns `nil` otherwise. ---
---@param categoryId string
---@param bindId string
---@return Maybe<unsigned>
function input.bindDown(categoryId, bindId) end

--- If this key was pressed this frame, returns how many times it has been pressed. Returns `nil` otherwise or if the specified modifier keys are not held. ---
---@param keyName string
---@param modNames StringList
---@return Maybe<unsigned>
function input.keyDown(keyName, modNames) end

--- Returns true if the specified key and all specified modifier keys (like Ctrl and Shift) are held. ---
---@param keyName string
---@param modNames StringList
---@return boolean
function input.keyHeld(keyName, modNames) end

--- Same as `input.keyHeld`. ---
---@param keyName string
---@param modNames StringList
---@return boolean
function input.key(keyName, modNames) end

--- If this key was released this frame, returns how many times it has been released. Returns `nil` otherwise or if the specified modifier keys are not held. ---
---@param keyName string
---@param modNames StringList
---@return Maybe<unsigned>
function input.keyUp(keyName, modNames) end

--- If this mouse button was pressed this frame, returns how many times it has been pressed. Returns `nil` otherwise. ---
---@param buttonName string
---@return Maybe<unsigned>
function input.mouseDown(buttonName) end

--- Returns true if the specified mouse button is held. ---
---@param buttonName string
---@return boolean
function input.mouseHeld(buttonName) end

--- Same as `input.mouseHeld`. ---
---@param buttonName string
---@return boolean
function input.mouse(buttonName) end

--- If this mouse button was released this frame, returns how many times it has been released. Returns `nil` otherwise. ---
---@param buttonName string
---@return Maybe<unsigned>
function input.mouseUp(buttonName) end

--- Resets this bind to its default keys. ---
---@param categoryId string
---@param bindId string
---@return void
function input.resetBinds(categoryId, bindId) end

--- Sets this bind to the specified keys. ---
---@param categoryId string
---@param bindId string
---@param binds Json
---@return void
function input.setBinds(categoryId, bindId, binds) end

--- Returns the default keys for this bind. ---
---@param categoryId string
---@param bindId string
---@return Json
function input.getDefaultBinds(categoryId, bindId) end

--- Returns the keys for this bind. ---
---@param categoryId string
---@param bindId string
---@return Json
function input.getBinds(categoryId, bindId) end

--- Returns all input events for this frame. ---
---@return Json
function input.events() end

--- Returns the mouse position in pixels relative to the bottom left of the screen. ---
---@return Vec2F
function input.mousePosition() end

--- Returns the amount of binds currently held with the given tag.
---@param tagName string
---@return unsigned
function input.getTag(tagName) end
