local module = {}
modules.copy_paste = module

local commands = modules.commands
local function getItemName(item)
  return item.parameters.shortdescription
      or root.itemConfig(item.name).config.shortdescription
      or item.name
end

local function popupError(prefix, msg)
  sb.logError("%s: %s", prefix, msg)
  msg = #msg > 80 and msg:sub(1, 80) .. "..." or msg
  local findNewLine = msg:find("\n", 1, true)
  interface.queueMessage("^#f00;error:^reset; " .. (findNewLine and msg:sub(1, findNewLine - 1) or msg), 7)
end

local function getClipboardText()
  local text = clipboard.hasText() and clipboard.getText()
  return text and text:sub(1, 1) == "{" and text or nil
end

local function copyItem()
  local item = player.swapSlotItem() or player.primaryHandItem() or player.altHandItem()
  if not item then return end

  local message = "^#f00;Failed to write to clipboard^reset;"
  if clipboard.setText(sb.printJson(item, 2)) then
    message = string.format("Copied ^cyan;%s^reset; to clipboard", getItemName(item))
  end
  interface.queueMessage(message, 4, 0.5)
end

local function pasteItem()
  if not clipboard.available() then
    return interface.queueMessage("^#f00;Clipboard unavailable^reset;", 4, 0.5)
  end
  local swap = player.swapSlotItem()
  local data = getClipboardText()
  if not data then return end

  local success, result = pcall(sb.parseJson, data)
  if not success then
    popupError("Error parsing clipboard item", result)
  else
    if swap then
      if swap.name == result.name and sb.jsonEqual(swap.parameters, result.parameters) then
        result.count = (tonumber(result.count) or 1) + (tonumber(swap.count) or 1)
      else
        return interface.queueMessage("^#f00;Cursor is occupied^reset;", 4, 0.5)
      end
    end
    local success, err = pcall(player.setSwapSlotItem, result)
    if not success then popupError("Error loading clipboard item", err)
    else
      local message = string.format("Pasted ^cyan;%s^reset; from clipboard", getItemName(result))
      interface.queueMessage(message, 4, 0.5)
    end
  end
end

function module.update()
  if input.bindDown("opensb", "editingCopyItemJson") then
    copyItem()
  end

  if input.bindDown("opensb", "editingPasteItemJson") then
    pasteItem()
  end
end

commands.register("exportplayer", function()
  if not clipboard.available() then
    return "Clipboard unavailable"
  end
  local success, text = pcall(sb.printJson, player.save(), 2)
  if not success then return text end
  clipboard.setText(text)
  return "Exported player to clipboard"
end)

commands.register("importplayer", function()
  local data = getClipboardText()
  if not data then return "Clipboard does not contain JSON" end

  local success, result = pcall(sb.parseJson, data)
  if not success then
    return "Error parsing player: " .. result
  else
    success, result = pcall(player.load, result)
    return success and "Loaded player from clipboard" or "Error loading player: " .. result
  end
end)