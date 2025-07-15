local module = {}
modules.identity = module
local commands = modules.commands

commands.register("identity", function(args)
  local method, path, value = chat.parseArguments(args)
  if not method or method == "get" or method == "" then
    local identity = player.humanoidIdentity()
    if path then
      value = identity[path]
      if value or path == "imagePath" then
        return type(value) == "string" and value or sb.printJson(value, 1)
      else
        return string.format("%s is not a part of the identity", path)
      end
    end
    return sb.printJson(player.humanoidIdentity(), 1)
  elseif method == "set" and path then
    local identity = player.humanoidIdentity()
    local prev = identity[path]
    if path ~= "imagePath" then
      if not prev then
        return string.format("%s is not a part of the identity", path)
      elseif type(prev) ~= type(value) then
        return string.format("%s must be a %s", path, type(prev))
      end
    elseif type(value) ~= "nil" and type(value) ~= "string" then
      return "imagePath must be null or a string"
    end
    identity[path] = value
    player.setHumanoidIdentity(identity)
    return string.format("Set %s to %s", path, type(value) == "string" and value or sb.printJson(value, 1))
  else 
    return "Usage: /identity <get|set> [path] [value]"
  end
end)

commands.register("description", function(args)
  if not args or args == "" then
    return player.description()
  else
    player.setDescription(args)
    return "Description set to " .. args .. ". Warp or rejoin for it to take effect."
  end
end)