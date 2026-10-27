---@meta

--- CommandProcessor API
---@class CommandProcessor
CommandProcessor = {}

--- Checks whether the specified connection id is authorized to perform admin actions and returns `nil` if authorization is succesful. If unauthorized, returns a `String` error message to display to the client requesting the action, which may include the specified action description, such as "Insufficient privileges to do the time warp again."
---@param connectionId ConnectionId
---@param actionDescription string
---@return string
function CommandProcessor.adminCheck(connectionId, actionDescription) end
