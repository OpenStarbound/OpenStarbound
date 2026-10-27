---@meta

--- clipboard API
---@class clipboard
clipboard = {}

--- Returns `true` if clipboard access is currently allowed. Clipboard operations require the game window to be focused and, unless overridden, for the player to have enabled clipboard integration. ---
---@return boolean
function clipboard.available() end

--- Returns `true` if textual data is available on the clipboard and the script may read it. ---
---@return boolean
function clipboard.hasText() end

--- Returns the clipboard text when access is available, or `nil` otherwise. ---
---@return Maybe<String>
function clipboard.getText() end

--- Replaces the clipboard contents with the supplied UTF-8 text. Returns `false` if the game window is unfocused or the operation is blocked. ---
---@param text string
---@return boolean
function clipboard.setText(text) end

--- Replaces the clipboard contents with arbitrary data payloads. Keys are MIME type strings (for example `"text/plain"` or `"image/png"`) and values are raw byte strings containing the payload for that type. Returns `false` if the game window is unfocused or the operation is blocked. ---
---@return boolean
function clipboard.setData() end

--- Copies an image to the clipboard. When an `Image` userdata is provided it is exported as-is; when a string path is supplied the image asset is loaded before exporting. The image is written as PNG data and `false` is returned if the clipboard cannot be modified.
---@return boolean
function clipboard.setImage() end
