---@meta

--- chat API
---@class chat
chat = {}

--- Sends a new message in the chat. Optionally specifies the modeName: - `"Broadcast"`: Global chat. Default value. - `"Local"`: Chat within the current planet. - `"Party"`: Chat within the current party. If `speak` is false, the chat bubble will not appear above the player. If `data` is provided, it will be sent as a JSON object with the message. This can be used to send custom data with the message. ---
---@param message string
---@param modeName string
---@param speak boolean
---@param data Json
---@return number
function chat.send(message, modeName, speak, data) end

--- Executes the specified command and returns a list of strings with the result. ---
---@param command string
---@return string[]
function chat.command(command) end

--- Adds the specified message to the chat log. The following keys are available in the `config` JSON object: - `String` __mode__ - The mode of the message. Can be one of the followgin: - `"Broadcast"` - `"Local"` - `"Party"` - `"Whisper"` - `"CommandResult"` - `"RadioMessage"` - `"World"` - `String` __channelName__ - The name of the channel to send the message to. - `String` __fromNick__ - The name of the sender of the message. - `String` __portrait__ - message portrait. - `bool` __showPane__ - If false, the chat pane will not be triggered. ---
---@param text string
---@param config Json
---@return void
function chat.addMessage(text, config) end

--- Returns the current chat input text. ---
---@return string
function chat.input() end

--- Sets the current chat input text. If `moveCursor` is true, the cursor will be moved to the end of the text. Returns true if the input was set successfully, false otherwise. ---
---@param text string
---@param moveCursor boolean
---@return boolean
function chat.setInput(text, moveCursor) end

--- Clears the chat input text. If `count` is provided, it will clear the last `count` messages, all otherwise.
---@return void
function chat.clear() end

---@return LuaVariadic<Json>
function chat.parseArguments() end
