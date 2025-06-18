local module = {}
modules.commands = module

local commands = {}
local function register(name, func)
  commands[name] = func
end

function module.init()
  for name, func in pairs(commands) do
    message.setHandler({name = "/" .. name, localOnly = true}, func)
  end
end

register("run", function(src)
	local success, result = pcall(loadstring, src, "/run")
  if not success then
    return "^#f00;compile error: " .. result
  else
    local success, result = pcall(result)
    if not success then
      return "^#f00;error: " .. result
    elseif result == nil then
      return nil
    elseif type(result) == "string" then
      return result
    else
      local success, printed = pcall(sb.printJson, result)
      if not success then
        success, printed = pcall(sb.print, result)
      end
      if not success then
        return "^#f00;could not print return value: " .. printed
      else
        return printed
      end
    end
  end
end)

register("headrotation", function(arg)
  local key = "disableHeadRotation"
  if status.statusProperty(key) == true then
    status.setStatusProperty(key, nil)
    return "Head rotation is now enabled for this character"
  else
    status.setStatusProperty(key, true)
    return "Head rotation is now disabled for this character"
  end
end)

module.register = register