local module = {}
modules.identity = module
local commands = modules.commands

commands.register("identity", function(args)
  local method, arg1, arg2 = chat.parseArguments(args)
  if not method or method == "get" or method == "" then
    local identity = player.humanoidIdentity()
    if arg1 then
      arg2 = identity[arg1]
      if arg2 or arg1 == "imagePath" then
        return type(arg2) == "string" and arg2 or sb.printJson(arg2, 1)
      else
        return string.format("%s is not a part of the identity", arg1)
      end
    end
    return sb.printJson(player.humanoidIdentity(), 1)
  elseif method == "set" and arg1 then
    local identity = player.humanoidIdentity()
    local prev = identity[arg1]
    if arg1 ~= "imagePath" then
      if not prev then
        return string.format("%s is not a part of the identity", arg1)
      elseif type(prev) ~= type(arg2) then
        return string.format("%s must be a %s", arg1, type(prev))
      end
    elseif type(arg2) ~= "nil" and type(arg2) ~= "string" then
      return "imagePath must be null or a string"
    end
    identity[arg1] = arg2
    player.setHumanoidIdentity(identity)
    return string.format("Set %s to %s", arg1, type(arg2) == "string" and arg2 or sb.printJson(arg2, 1))
  elseif method == "randomize" then
    local newIdentity = root.generateHumanoidIdentity(arg1 or player.species(), nil, player.gender())
    player.setHumanoidIdentity(newIdentity)
    return "Randomized identity"
  elseif method == "save" then
    if type(arg1) == "string" then
      if arg1:find("%.") then
        return "Name cannot contain '.'"
      end
            root.setConfigurationPath("savedHumanoidIdentities." .. arg1, player.humanoidIdentity())
      return "Saved identity to starbound.config:savedHumanoidIdentities."..arg1
    else
      return "You must provide a name for this identity"
    end
  elseif method == "load" then
    if type(arg1) == "string" then
      local identity = root.getConfigurationPath("savedHumanoidIdentities." .. arg1)
      if identity then
        player.setHumanoidIdentity(identity)
        return "Loaded identity from starbound.config:savedHumanoidIdentities."..arg1
      else
        return "No saved identity named "..arg1
      end
    else
      return "You must provide a name to load from"
    end
  else
    return "Usage: /identity <get|set> [path] [value]\n/identity randomize [species]\n/identity <save|load> [name]"
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
