--constants
local PATH = "/interface/opensb/bindings/"
local CATEGORY_LIST_WIDGET = "categories.list"
local BINDS_WIDGET       = "binds"

local fmt = string.format
local log = function() end

--SNARE

local snared = false

function snare(key) end

local mods = {}
for i, mod in ipairs{"LShift", "RShift", "LCtrl", "RCtrl", "LAlt", "RAlt", "LGui", "RGui", "AltGr", "Scroll"} do
  local data = {name = mod, active = false}
  mods[i], mods[mod] = data, data
end


local function getMods(key)
  local bindMods = jarray()
  for i, mod in ipairs(mods) do
    if mod.active and mod.name ~= key then
      bindMods[#bindMods + 1] = mod.name
    end
  end
  if bindMods[1] then return bindMods end
end

local function finishBind(type, value)
  widget.blur("snare")
  snared = false
  snareFinished{ type = type, value = value, mods = getMods(value) }
  for i, mod in ipairs(mods) do
    mod.active = false
  end
end

local function scanInputEvents()
  local events = input.events()
  for i, event in pairs(events) do
    local type, data = event.type, event.data
    if type == "KeyDown" then
      local key = data.key
      local mod = mods[key]
      if mod then mod.active = true end
    elseif type == "KeyUp" then
      return finishBind("key", data.key)
    elseif type == "MouseButtonDown" then
      return finishBind("mouse", data.mouseButton)
    end
  end
end

function update()
  if snared then
    scanInputEvents()
    if snared and not widget.hasFocus("snare") then
      snared = false
      snareFinished()
      for i, mod in ipairs(mods) do
        mod.active = false
      end
    end
  end
end

local function beginSnare()
  snared = true
  widget.focus("snare")
end

-- BINDING


local function alphabeticalNameSortGreater(a, b) return a.name > b.name end
local function alphabeticalNameSortLesser(a, b)  return a.name < b.name end
local sortedCategories = {}
local categories = {}

local widgetsToCategories = {}
local allBinds = {}

local function addCategoryToList(data)
  local name = widget.addListItem(CATEGORY_LIST_WIDGET)
  widget.setText(fmt("%s.%s.button", CATEGORY_LIST_WIDGET, name), data.name)
  log("Added category ^cyan;%s^reset; to list", data.name)
  return name
end

local function parseBinds()
  for i, path in pairs(root.assetsByExtension("binds")) do
    local data = root.assetJson(path)
    for categoryId, data in pairs(data) do
      if not data.name then data.name = categoryId end
      data.categoryId = categoryId
      categories[categoryId] = data
    end
  end

  for categoryId, data in pairs(categories) do
    sortedCategories[#sortedCategories + 1] = data
  end
  table.sort(sortedCategories, alphabeticalNameSortLesser)
  for i = 1, #sortedCategories do
    local data = sortedCategories[i]
    data.index = i
    local name = addCategoryToList(data)
    data.widget = name
    widgetsToCategories[name] = data
  end
  if sortedCategories[1] then
    local first = sortedCategories[1].widget
    widget.setListSelected(CATEGORY_LIST_WIDGET, first)
    selectCategory(first)
  end
end

function bindsToString(binds)
  local t = {}
  for i, bind in pairs(binds) do
    local str = ""
    if bind.mods then
      for i, v in pairs(bind.mods) do
        str = str .. v .. " + "
      end
    end
    if bind.type == "key" then
      str = str .. bind.value
    elseif bind.type == "mouse" then
      str = str .. bind.value
    end
    local _i = (i - 1) * 2
    if _i ~= 0 then
      t[_i] = ", "
    end
    t[_i + 1] = str
  end
  return table.concat(t)
end

function setButtonsEnabled(state)
  for i, bind in pairs(allBinds) do
    widget.setButtonEnabled(fmt("%s.apply_%s", BINDS_WIDGET, bind.bindId), state)
    widget.setButtonEnabled(fmt("%s.erase_%s", BINDS_WIDGET, bind.bindId), state)
    widget.setButtonEnabled(fmt("%s.reset_%s", BINDS_WIDGET, bind.bindId), state)
  end
  for widgetId in pairs(widgetsToCategories) do
    widget.setButtonEnabled(fmt("%s.%s.button", CATEGORY_LIST_WIDGET, widgetId), state)
  end
  widget.setButtonEnabled(CATEGORY_LIST_WIDGET, state)
end

local activeBind
local activeCategory

function snareFinished(newBind)
  setButtonsEnabled(true)
  if not newBind or not activeBind or not activeCategory then return end
  local currentBinds = input.getBinds(activeCategory, activeBind)
  local replace = false
  for i, v in pairs(currentBinds) do
    if sb.printJson(v) == sb.printJson(newBind) then
      --equal, replace all binds
      currentBinds = {newBind}
      replace = true
      break
    end
  end
  if replace then
    currentBinds = {newBind}
    log("Replaced %s with %s", activeBind, sb.printJson(newBind))
  else
    currentBinds[#currentBinds + 1] = newBind
    log("Added %s to %s", sb.printJson(newBind), activeBind)
  end
  input.setBinds(activeCategory, activeBind, currentBinds)
  widget.setText(fmt("%s.apply_%s", BINDS_WIDGET, activeBind), bindsToString(currentBinds))
end

function eraseBind(bind)
  bind = bind:sub(7)
  local binds = jarray()
  binds[1] = nil
  input.setBinds(activeCategory, bind, binds)
  widget.setText(fmt("%s.apply_%s", BINDS_WIDGET, bind), "")
end

function resetBind(bind)
  bind = bind:sub(7)
  local defaultBinds = input.getDefaultBinds(activeCategory, bind)
  if #defaultBinds == 0 then
    defaultBinds = jarray()
    defaultBinds[1] = nil
  end
  input.setBinds(activeCategory, bind, defaultBinds)
  widget.setText(fmt("%s.apply_%s", BINDS_WIDGET, bind), bindsToString(defaultBinds))
end

function applyBind(bind)
  bind = bind:sub(7)
  log("Modifying bind %s", bind)
  setButtonsEnabled(false)
  activeBind = bind
  beginSnare()
end

local function addBindGroup(data, i, added)
  local y = (i - 1) * -14
  local bg = {
    type = "image",
    file = PATH .. "groupname.png",
    position = {-12, y}
  }
  local name = "group_" .. i
  widget.addChild(BINDS_WIDGET, bg, name)
  added[#added + 1] = name
  local label = {
    type = "label",
    value = data.name,
    wrapWidth = 120,
    fontSize = 8,
    hAnchor = "mid",
    vAnchor = "mid",
    position = {120, 6}
  }
  widget.addChild(fmt("%s.%s", BINDS_WIDGET, name), label, "text")
end

local function addBindSet(data, i, added)
  local y = (i - 1) * -14
  local bg = {
    type = "image",
    file = PATH .. "bindname.png",
    position = {-12, y}
  }
  local name = "label_" .. i
  widget.addChild(BINDS_WIDGET, bg, name)
  added[#added + 1] = name
  local label = {
    type = "label",
    value = data.name,
    wrapWidth = 120,
    fontSize = 8,
    hAnchor = "mid",
    vAnchor = "mid",
    position = {62, 6}
  }
  widget.addChild(fmt("%s.%s", BINDS_WIDGET, name), label, "text")
  local button = {
    type = "button",
    callback = "applyBind",
    position = {112, y + 2},
    pressedOffset = {0, 0},
    base = PATH .. "bind.png",
    hover = PATH .. "bind.png?fade=fff;0.025",
    pressed = PATH .. "bind.png?fade=fff;0.05?multiply=0f0",
    caption = bindsToString(input.getBinds(data.categoryId, data.bindId))
  }
  name = "apply_" .. data.bindId
  added[#added + 1] = name
  widget.addChild(BINDS_WIDGET, button, name)
  local erase = {
    type = "button",
    callback = "eraseBind",
    position = {209, y + 2},
    pressedOffset = {0, -1},
    base = PATH .. "garbage.png",
    hover = PATH .. "garbage.png?multiply=faa",
    pressed = PATH .. "garbage.png?multiply=f00",
  }
  name = "erase_" .. data.bindId
  added[#added + 1] = name
  widget.addChild(BINDS_WIDGET, erase, name)
  local reset = {
    type = "button",
    callback = "resetBind",
    position = {218, y + 2},
    pressedOffset = {0, -1},
    base = PATH .. "reset.png",
    hover = PATH .. "reset.png?multiply=faa",
    pressed = PATH .. "reset.png?multiply=f00",
  }
  name = "reset_" .. data.bindId
  added[#added + 1] = name
  widget.addChild(BINDS_WIDGET, reset, name)
end

function selectCategory()
  local selected = widget.getListSelected(CATEGORY_LIST_WIDGET)
  local category = widgetsToCategories[selected]
  local dataFromPrev = widget.getData(BINDS_WIDGET)
  if dataFromPrev then
    for i, v in pairs(dataFromPrev) do
      widget.removeChild(BINDS_WIDGET, v)
    end
  end
  widgetsToBinds = {}

  local groups = category.groups or {}
  if not groups.unsorted then
    groups.unsorted = {name = "Unsorted"}
  end

  local sortedGroups = {}
  for groupId, data in pairs(groups) do
    data.name = tostring(data.name or groupId)
    data.sortedBinds = {}
    sortedGroups[#sortedGroups + 1] = data
  end

  allBinds = {}
  for bindId, data in pairs(category.binds) do
    if not data.name then data.name = bindId end
    data.bindId = bindId
    data.categoryId = category.categoryId
    local group = groups[data.group or "unsorted"] or groups.unsorted
    group.sortedBinds[#group.sortedBinds + 1] = data
    allBinds[#allBinds + 1] = data
  end

  activeCategory = category.categoryId
  table.sort(sortedGroups, alphabeticalNameSortLesser)

  for groupId, data in pairs(groups) do
    table.sort(data.sortedBinds, alphabeticalNameSortLesser)
  end

  local bannerBinds = widget.bindCanvas("banner")
  bannerBinds:clear()

  local bannerName = tostring(category.bannerName or category.name or category.categoryId)
  bannerBinds:drawText(bannerName, {position = {127, 13}, horizontalAnchor = "mid", verticalAnchor = "mid"}, 16)

  local onlyUnsorted = not sortedGroups[2] and sortedGroups[1] == groups.unsorted

  local added = {}
  local index = 0
  for iA = 1, #sortedGroups do
    local group = sortedGroups[iA]
    local bindsCount = #group.sortedBinds
    if bindsCount > 0 then
      if not onlyUnsorted then
        index = index + 1
        addBindGroup(group, index, added)
      end
      for iB = 1, bindsCount do
        index = index + 1
        addBindSet(group.sortedBinds[iB], index, added)
      end
    end
  end

  widget.setData(BINDS_WIDGET, added)
end


local function initCallbacks()
  widget.registerMemberCallback(CATEGORY_LIST_WIDGET, "selectCategory", selectCategory)
end

function init()
  --log = sb.logInfo

  widget.clearListItems(CATEGORY_LIST_WIDGET)
  initCallbacks()
  parseBinds()

  script.setUpdateDelta(1)
end