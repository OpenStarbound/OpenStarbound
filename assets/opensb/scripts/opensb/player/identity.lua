local module = {}
modules.copy_paste = module

local commands = modules.commands


commands.register("identity", function(args)
  method, path, value = chat.parseArguments(args)
  if not method or method == "get" or method == "" then
    local identity = player.humanoidIdentity()

	if path then
	  if identity[path] then
	    return string.format(identity[path])
      else
	    return string.format("%s is not a part of the identity", path)
	  end
	end

	return sb.printJson(player.humanoidIdentity(), 1)
  elseif method == "set" and path and value then
	local identity = player.humanoidIdentity()
	if not identity[path] then
	  return string.format("%s is not a part of the identity", path)
	end
	identity[path] = value
	player.setHumanoidIdentity(identity)
	return string.format("Set %s to %s", path, value)
  else 
    return "Usage: /identity <get|set> [path] [value]"
  end
end)