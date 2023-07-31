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
    if not success then
      return "^#f00;error: " .. result
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