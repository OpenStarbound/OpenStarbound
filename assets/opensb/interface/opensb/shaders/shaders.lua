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
local shaderConfig

function displayed()
  shaderConfig = root.getConfiguration("postProcessGroups")
end
function dismissed()
  shaderConfig = nil
end

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
    value = data.label,
    wrapWidth = 120,
    fontSize = 8,
    hAnchor = "mid",
    vAnchor = "mid",
    position = {120, 6}
  }
  widget.addChild(fmt("%s.%s", BINDS_WIDGET, name), label, "text")
end

local function digitRegex(n)
  local i = 0
  while n ~= math.floor(n) do
    n = n * 10
    i = i + 1
  end
  return fmt("%%.%df",i) -- create format string %.nf, where n = %d (i with no decimal points)
end
local optionPrefix = "option_"
local optionOffset = #optionPrefix+1
local function addOption(data, i, added)
  local y = (i - 1) * -14
  local bg = {
    type = "image",
    file = PATH .. "optionname.png",
    position = {-12, y}
  }
  local name = "label_" .. data.name
  widget.addChild(OPTIONS_WIDGET, bg, name)
  added[#added + 1] = name
  local label = {
    type = "label",
    value = data.label,
    wrapWidth = 120,
    fontSize = 8,
    hAnchor = "mid",
    vAnchor = "mid",
    position = {62, 6}
  }
  widget.addChild(fmt("%s.%s", OPTIONS_WIDGET, name), label, "text")
  local value = data.default or 0
  if not shaderConfig[activeGroup] then
    shaderConfig[activeGroup] = {parameters={}}
  end
  if not shaderConfig[activeGroup].parameters then
    shaderConfig[activeGroup].parameters = {}
  end
  if shaderConfig[activeGroup].parameters[data.name] ~= nil then
    value = shaderConfig[activeGroup].parameters[data.name]
  end
  
  -- todo: finish this
  if data.type == "slider" then
    local range = data.range or {0,1}
    local delta = data.delta or 0.01
    -- for some reason ranges require ints so
    local r = {range[1]/delta, range[2]/delta, 1}
    
    local slider = {
      type = "slider",
      callback = "sliderOptionModified",
      position = {110, y + 2},
      gridImage = "/interface/optionsmenu/smallselection.png",
      range = r
    }
    name = optionPrefix..data.name
    added[#added + 1] = name
    widget.addChild(OPTIONS_WIDGET, slider, name)
    
    widget.setSliderValue(fmt("%s.%s",OPTIONS_WIDGET,name), value/delta)
    local valLabel = {
      type = "label",
      value = fmt(digitRegex(delta),value),
      position = {186, y + 2}
    }
    added[#added + 1] = name.."_value"
    widget.addChild(OPTIONS_WIDGET, valLabel, name.."_value")
  elseif data.type == "select" then
    local n = #data.values
    local width = math.floor(118/n)
    local by = y
    local buttons = {}
    for i=0,n-1 do
      local img = fmt("%sselect.png?crop=0;0;%.0f;13",PATH,width)
      local button = {
        type = "button",
        callback="selectOptionPressed",
        caption = data.values[i+1],
        base = img,
        hover = img.."?brightness=-25",
        baseImageChecked = img.."?multiply=00ff00",
        hoverImageChecked = img.."?multiply=00ff00?brightness=-25",
        position = {110+width*i, by},
        pressedOffset = {0, 0},
        checkable = true,
        checked = (value == i)
      }
      name = "select_"..data.name.."_"..i
      added[#added + 1] = name
      widget.addChild(OPTIONS_WIDGET, button, name)
      table.insert(buttons,{widget=fmt("%s.%s",OPTIONS_WIDGET,name),index=i})
    end
    for k,v in next, buttons do
      widget.setData(v.widget,{option=data,index=v.index,buttons=buttons})
    end
    --[[local bge = {
      type = "image",
      file = PATH.."selectend.png",
      position = {110+width*3, by}
    }
    name = "bgend_"..data.name
    added[#added + 1] = name
    widget.addChild(OPTIONS_WIDGET, bge, name)]]
  end
end

function sliderOptionModified(option, odata)
  local oname = string.sub(option, optionOffset)
  local parameter = groups[activeGroup].parameters[oname]
  local value = widget.getSliderValue(fmt("%s.%s",OPTIONS_WIDGET,option))*parameter.delta
  
  widget.setText(fmt("%s.%s",OPTIONS_WIDGET,option.."_value"), fmt(digitRegex(parameter.delta),value))
  
  for k,v in next, parameter.effects do
    renderer.setEffectParameter(v, oname, value)
  end
  shaderConfig[activeGroup].parameters[oname] = value
  root.setConfiguration("postProcessGroups",shaderConfig)
end

function selectOptionPressed(button,bdata)
  sb.logInfo(sb.print(bdata))
  
  for k,v in next, bdata.buttons do
    if v.index ~= bdata.index then
      widget.setChecked(v.widget, false)
    end
  end
  
  value = bdata.index
  
  local oname = bdata.option.name
  local parameter = groups[activeGroup].parameters[oname]
  
  for k,v in next, parameter.effects do
    renderer.setEffectParameter(v, oname, value)
  end
  shaderConfig[activeGroup].parameters[oname] = value
  root.setConfiguration("postProcessGroups",shaderConfig)
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
  shaderConfig = root.getConfiguration("postProcessGroups")
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
  if group.categories then
  
  elseif group.parameters then
    local sortedParams = {}
    for k,v in next, group.parameters do
      v.name = k
      sortedParams[#sortedParams+1] = v
    end
    table.sort(sortedParams, alphabeticalNameSortLesser)
    
    for k,v in next, sortedParams do
      index = index + 1
      addOption(v, index, added)
    end
  end
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
  end
  ]]
  
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
