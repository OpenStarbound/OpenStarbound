local commands = {}
local logHelp = "Available OpenStarbound server commands:\n"
local userHelp = logHelp .. "^cyan;"
local adminHelp = userHelp

local function cmd(meta, func)
  local first = next(commands) == nil
  local name        = meta.name
  local description = meta.description
  local permission  = meta.permission
  if not meta.hidden then
    logHelp   =   logHelp .. (first and name or ", "       .. name)
    userHelp  =  userHelp .. (first and name or ", ^cyan;" .. name)
    adminHelp = adminHelp .. (first and name or ", ^cyan;" .. name)
  end

  local keyName = name:lower()
  if permission == "tell" then
    commands[keyName] = {meta = meta, func = function(connectionId, ...)
      return func(universe.isAdmin(connectionId), connectionId, ...)
    end}
  elseif permission == "admin" then
    commands[keyName] = {meta = meta, func = function(connectionId, ...)
      local error = CommandProcessor.adminCheck(connectionId, description:sub(1, 1):lower() .. description:sub(2))
      if error then
        return error
      else
        return func(connectionId, ...)
      end
    end}
  elseif permission == "user" then
    commands[keyName] = {meta = meta, func = func}
  else
    error(string.format("Command '%s' has invalid permission", name))
  end
end

cmd({
  name = "openhelp",
  description = "Get help",
  permission = "tell"
},
function(isAdmin, connectionId)
  return isAdmin and adminHelp or userHelp
end)

do

local objects = nil
cmd({
  name = "packetTest",
  description = "Do science",
  permission = "admin",
  hidden = true
},
function(connectionId)
  if not objects then
    objects = {}
    local paths = root.assetsByExtension("object")
    for i, v in pairs(paths) do
      local json = root.assetJson(v)
      objects[i] = {json.objectName, json.shortdescription or json.objectName}
    end
  end
  local item = objects[math.random(#objects)]
  universe.sendPacket(connectionId, "GiveItem", {
    item = {name = item[1], count = 1}
  })

  return "Can I interest you in a ^cyan;" .. item[2] .. "^reset;?"
end)

end

local _init = type(init) == "function" and init
function init(...)
  sb.logInfo("%s", logHelp)
  if _init then return _init(...) end
end

local _command = type(command) == "function" and command
function command(commandName, connectionId, args)
  local ret = _command and _command(commandName, connectionId, args)
  if ret then return ret end

  local command = commands[commandName:lower()]
  if command then
    local success, ret = pcall(command.func, connectionId, table.unpack(args))
    if not success then
      sb.logError("Error in OpenStarbound server command /%s: %s", commandName, ret)
      return "command error: " .. ret
    else
      return ret
    end
  end
end