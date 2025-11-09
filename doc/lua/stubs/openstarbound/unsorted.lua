---@meta

--- Global functions

--- Compiles the provided **source** and returns it as a callable function. If there are any syntax errors, returns `nil` and the error as a string instead. - **name** is used for error messages, the default is the name of the script that called `loadstring`. - **env** is used as the environment of the returned function, the default is `_ENV`.
---@param source string
---@param name string
---@param env LuaValue
---@return Maybe<LuaFunction>, Maybe<String>
function loadstring(source, name, env) end
