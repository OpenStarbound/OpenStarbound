--constants
local PATH = "/interface/opensb/shaders/"
local GROUP_LIST_WIDGET = "groups.list"
local OPTIONS_WIDGET       = "options"

local fmt = string.format
local log = function() end

function update()
end


local function alphabeticalNameSortGreater(a, b) return a.name > b.name end
local function alphabeticalNameSortLesser(a, b)  return a.name < b.name end
local sortedGroups = {}
local groups = {}

local widgetsToGroups = {}
local allSettings = {}

local function addGroupToList(data)
  local name = widget.addListItem(GROUP_LIST_WIDGET)
  widget.setText(fmt("%s.%s.button", GROUP_LIST_WIDGET, name), data.friendlyName)
  log("Added group ^cyan;%s^reset; to list", data.friendlyName)
  return name
end

local function parseGroups()
  for name, data in next, renderer.postProcessGroups() do
    if not data.hidden then
      if not data.friendlyName then data.friendlyName = name end
      data.groupId = name
      data.name = data.friendlyName
      groups[data.groupId] = data
    end
  end

  for groupId, data in pairs(groups) do
    sortedGroups[#sortedGroups + 1] = data
  end
  table.sort(sortedGroups, alphabeticalNameSortLesser)
  for i = 1, #sortedGroups do
    local data = sortedGroups[i]
    data.index = i
    local name = addGroupToList(data)
    data.widget = name
    widgetsToGroups[name] = data
  end
  if sortedGroups[1] then
    local first = sortedGroups[1].widget
    widget.setListSelected(GROUP_LIST_WIDGET, first)
    selectGroup(first)
  end
end

local activeGroup

local function addOptionGroup(data, i, added)
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

local function addOption(data, i, added)
  local y = (i - 1) * -14
  local bg = {
    type = "image",
    file = PATH .. "optionname.png",
    position = {-12, y}
  }
  local name = "label_" .. i
  widget.addChild(OPTIONS_WIDGET, bg, name)
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
  widget.addChild(fmt("%s.%s", OPTIONS_WIDGET, name), label, "text")
  -- todo: finish this
end

local function addEnabled(groupname, i, added)
  local y = (i - 1) * -14
  local bg = {
    type = "image",
    file = PATH .. "optionname.png",
    position = {-12, y}
  }
  local name = "label_" .. i
  widget.addChild(OPTIONS_WIDGET, bg, name)
  added[#added + 1] = name
  local label = {
    type = "label",
    value = "Enabled",
    wrapWidth = 120,
    fontSize = 8,
    hAnchor = "mid",
    vAnchor = "mid",
    position = {62, 6}
  }
  widget.addChild(fmt("%s.%s", OPTIONS_WIDGET, name), label, "text")
  local checkbox = {
    type = "button",
    callback = "toggleGroupEnabled",
    position = {165, y + 2},
    pressedOffset = {0, 0},
    base = "/interface/optionsmenu/checkboxnocheck.png",
    hover = "/interface/optionsmenu/checkboxnocheckhover.png",
    baseImageChecked = "/interface/optionsmenu/checkboxcheck.png",
    hoverImageChecked = "/interface/optionsmenu/checkboxcheckhover.png",
    checkable = true,
    checked = renderer.postProcessGroupEnabled(groupname)
  }
  name = "check_"..groupname
  added[#added + 1] = name
  widget.addChild(OPTIONS_WIDGET, checkbox, name)
end

function toggleGroupEnabled(checkbox, cdata)
  renderer.setPostProcessGroupEnabled(activeGroup, widget.getChecked(fmt("%s.%s",OPTIONS_WIDGET,checkbox)), true)
end

function selectGroup()
  local selected = widget.getListSelected(GROUP_LIST_WIDGET)
  local group = widgetsToGroups[selected]
  local dataFromPrev = widget.getData(OPTIONS_WIDGET)
  if dataFromPrev then
    for i, v in pairs(dataFromPrev) do
      widget.removeChild(OPTIONS_WIDGET, v)
    end
  end
  widgetsToOptions = {}
  
  activeGroup = group.groupId
  
  local bannerOptions = widget.bindCanvas("banner")
  bannerOptions:clear()

  local bannerName = tostring(group.bannerName or group.name or group.groupId)
  bannerOptions:drawText(bannerName, {position = {127, 13}, horizontalAnchor = "mid", verticalAnchor = "mid"}, 16)
  
  local added = {}
  local index = 0
  addEnabled(group.groupId,index,added)

  --[[
  local categories = group.categories or {}
  if not categories.unsorted then
    categories.unsorted = {name = "Unsorted"}
  end

  local sortedCategories = {}
  for categoryId, data in pairs(categories) do
    data.name = tostring(data.name or categoryId)
    data.sortedOptions = {}
    sortedCategories[#sortedCategories + 1] = data
  end

  table.sort(sortedCategories, alphabeticalNameSortLesser)

  for categoryId, data in pairs(categories) do
    table.sort(data.sortedOptions, alphabeticalNameSortLesser)
  end

  local onlyUnsorted = not sortedGroups[2] and sortedGroups[1] == categories.unsorted

  for iA = 1, #sortedCategories do
    local category = sortedCategories[iA]
    local optionsCount = #category.sortedOptions
    if optionsCount > 0 then
      if not onlyUnsorted then
        index = index + 1
        addOptionCategory(category, index, added)
      end
      for iB = 1, optionsCount do
        index = index + 1
        addOption(category.sortedOptions[iB], index, added)
      end
    end
  end]]
  
  widget.setData(OPTIONS_WIDGET, added)
end


local function initCallbacks()
  widget.registerMemberCallback(GROUP_LIST_WIDGET, "selectGroup", selectGroup)
end

function init()
  --log = chat and chat.addMessage or sb.logInfo

  widget.clearListItems(GROUP_LIST_WIDGET)
  initCallbacks()
  parseGroups()

  script.setUpdateDelta(1)
end
