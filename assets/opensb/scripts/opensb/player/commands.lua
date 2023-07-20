local module = {}
modules.commands = module

local commands = {}
local function command(name, func)
  commands[name] = func
end

function module.init()
  for name, func in pairs(commands) do
    message.setHandler("/" .. name, function(isLocal, _, ...)
      if not isLocal then
        return
      else
        return func(...)
      end
    end)
  end
end


command("run", function(src)
	local success, result = pcall(loadstring, src, "/run")
  if not success then
    return "^#f00;compile error: " .. result
  else
    local success, result = pcall(result)
    return not success and "^#f00;error: " .. result or sb.printJson(result)
  end
end)