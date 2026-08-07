--Silver Sokolova

--[[
CHANGES FROM VANILLA
mode radioGroup has a `mode` value in its data, for the name of the mode
gender radioGroup has a `data` value, set to the name of the gender

TODO: use root.assetsByExtension to gather all species files and put
them in a table, as people can put species files in any folder
{
  "species": {
    "human": "/species/human.species"
  }
}
]]

function init()
  debug = false

  spinners = config.getParameter("spinners")
  spinnerIndices = config.getParameter("spinnerIndices")
  disabledSpinners = {}
  personalities = root.assetJson("/humanoid.config:personalities")
  tooltip = root.assetJson("/interface/tooltips/species.tooltip")
  
  --If world exists, we're not on the title screen
  if world then
    --Pre-select some options based on the player's current identity
    z_genderOptions = #config.getParameter("gui.gender.buttons") - 1
    local playerGender = player.gender()
    for i = 0, z_genderOptions do
      if playerGender == widget.getData("gender."..i) then
        widget.setSelectedOption("gender", i)
        genderIndex = i + 1
        genderChoice = i
      end
    end

    local z_modes = #config.getParameter("gui.mode.buttons") - 1
    local playerMode = player.mode()
    for i = 0, z_modes do
      local data = widget.getData("mode."..i)
      if playerMode == data.mode then
        widget.setSelectedOption("mode", i)
        widget.setText("labelMode", data.description)
      end
    end

    widget.setVisible("saveChar", false) --TODO: change after title screen merge?
    widget.setText("name", player.name())
  end

  speciesOrdering = config.getParameter("speciesOrdering")
  z_speciesSlots = 0
  speciesIndex = 1
  z_speciesSlots = #config.getParameter("gui").species.buttons
  speciesOffset = 0
  speciesInPage = 0
  populateSpecies(_, 0)
  
  --Grab species data
  updateSpeciesData(player.species())
  speciesChoice = player.species()

  --TODO: find what number they are
  updateChoices(player.getHumanoidParameter("choices"))

  --TODO: Update spinner labels
  updateCharacterImages()
  script.setUpdateDelta(debug and 1 or 0)
end

function updateChoices(choices)
  choices = choices or {}
  genderChoice      = choices[1] or 1
  bodyColorChoice   = choices[2] or 1
  altyChoice        = choices[3] or 1
  hairChoice        = choices[4] or 1
  shirtChoice       = choices[5] or 1
  pantsChoice       = choices[6] or 1
  headyChoice       = choices[7] or 1
  shirtColorChoice  = choices[8] or 1
  pantsColorChoice  = choices[9] or 1
  personalityChoice = choices[10] or 1
end

function saveChar() end
function cancel() end
function toggleClothing() end

function updateHumanoidIdentity()
  player.setHumanoidParameter("choices", {genderChoice, bodyColorChoice, altyChoice, hairChoice, shirtChoice, pantsChoice, headyChoice, shirtColorChoice, pantsColorChoice, personalityChoice})

  local newIdentity, newParameters, newEquipment = root.createHumanoid(
    widget.getText("name"),
    speciesChoice or "human",
    genderChoice,
    bodyColorChoice,
    altyChoice,
    hairChoice,
    headyChoice,
    shirtChoice,
    shirtColorChoice,
    pantsChoice,
    pantsColorChoice,
    personalityChoice,
    ext
  )
  player.setHumanoidIdentity(newIdentity)
end

function populateSpecies(_, direction)
  speciesIndex = speciesIndex + (z_speciesSlots * direction)

  for i = 0, z_speciesSlots do
    local radioButton = "species." .. i
    local currentSpecies = speciesOrdering[speciesIndex + i]

    if currentSpecies then
      local speciesConfig = root.assetJson("/species/"..currentSpecies..".species")

      local characterImages = {}
      for j = 0, z_genderOptions do
        characterImages[j + 1] = speciesConfig.genders[j + 1].characterImage
      end

      widget.setData(radioButton, {
        race = currentSpecies,
        --charCreationTooltip = {title="",subTitle="",description=("speciesOffset: "..speciesOffset)},
        tooltipValues = speciesConfig.charCreationTooltip,
        characterImages = characterImages
      })
      widget.setButtonOverlayImage("species."..i, speciesConfig.genders[genderIndex].characterImage)
    else
      widget.setButtonOverlayImage("species."..i, "/assetmissing.png?crop=1;1;1;1")
      widget.setData(radioButton, nil)
    end
  end
  widget.setButtonEnabled("btnSpeciesUp", (speciesIndex + z_speciesSlots) < #speciesOrdering)
  widget.setButtonEnabled("btnSpeciesDown", (speciesIndex + z_speciesSlots) ~= (z_speciesSlots + 1))
end

function updateCharacterImages()
  for i = 0, z_speciesSlots do
    local data = widget.getData("species."..i)
    if data then
      widget.setButtonOverlayImage("species."..i, data.characterImages[genderIndex])
    end
  end
end

--[[
function updateSpinners()
  updateBodyDirectives(0, 0)
  spinnerPersonality(0)
--spinnerStyle("hair", "hair", "hairType", 0)
--spinnerStyle("heady", "facialHair", "facialHairType", 0)
  heady.main(0)
  updateSpinnerLabel("shirt", getClothingForLabel("shirt", "chest"))
  updateSpinnerLabel("pants", getClothingForLabel("pants", "legs"))
  updateSpinnerLabel("shirtColor", getClothingColorForLabel(player.equippedItem("chest")))
  updateSpinnerLabel("pantsColor", getClothingColorForLabel(player.equippedItem("legs")))
end
]]

function gender(index, gender)
  if player.gender() ~= gender then
    genderIndex = index + 1
    genderChoice = index
    updateHumanoidIdentity()
    updateBodyTypeGroups()
    updateCharacterImages()
  end
end

function mode(index, data)
  if player.mode() ~= data.mode then
    player.setMode(data.mode)
    widget.setText("labelMode", data.description)
  end
end

function name()
  player.setName(widget.getText("name"))
end

function randomName()
  widget.setText("name", root.generateName(speciesData.nameGen[genderIndex]))
end

function randomize()
  identity, choices = root.generateHumanoidIdentity(player.species(), nil, player.gender())

  player.setHumanoidParameter("choices", choices.choices)
  updateChoices(choices.choices)
  updateHumanoidIdentity()
end

function species(index, data)
  if data and player.species() ~= data.race then
    speciesChoice = data.race
    updateSpeciesData(data.race)
    randomize()
    updateHumanoidIdentity()
  --updateSpinners()
  end
end

function updateSpeciesData(species)
  speciesData = root.assetJson("/species/"..species..".species") --TODO: Gather a list of species files and their locations via root.assetsByExtension and use that instead of assuming the path. I'm pretty sure people can plop species files wherever they want
  for i = 1, #speciesData.bodyColor do speciesData.bodyColor[i] = paletteSwapDirective(speciesData.bodyColor[i]) end
  for i = 1, #speciesData.undyColor do speciesData.undyColor[i] = paletteSwapDirective(speciesData.undyColor[i]) end
  for i = 1, #speciesData.hairColor do speciesData.hairColor[i] = paletteSwapDirective(speciesData.hairColor[i]) end
  
  for i = 1, #spinners do
    widget.setText(spinners[i], speciesData.charGenTextLabels[i])
    disabledSpinners[spinners[i]] = speciesData.charGenTextLabels[i] == ""
  end

  updateBodyTypeGroups()
  headOptionAsHairColor = speciesData["headOptionAsHairColor"]
  headOptionAsFacialhair = speciesData["headOptionAsFacialhair"]
  altOptionAsUndyColor = speciesData["altOptionAsUndyColor"]
  altOptionAsHairColor = speciesData["altOptionAsHairColor"]

  for i = 0, z_genderOptions do
    widget.setButtonOverlayImage("gender."..i, speciesData.genders[i + 1].image)
  end
end

--Called from `gender` and `updateSpeciesData`
function updateBodyTypeGroups()
  --Check if there's actually a change as to not spam servers with new data
  local newGroup = speciesData.genders[genderIndex].hairGroup or "hair"; if newGroup ~= player.hairGroup() then player.setHairGroup(newGroup) end
        newGroup = speciesData.genders[genderIndex].facialHairGroup or "facialHair"; if newGroup ~= player.facialHairGroup() then player.setFacialHairGroup(newGroup) end
        newGroup = speciesData.genders[genderIndex].facialMaskGroup or "facialMask"; if newGroup ~= player.facialMask() then player.setFacialMask(newGroup) end
end

function paletteSwapDirective(color) --modified version from vanilla dye script
  if color == "" then return color end
  local directive = "?replace"
  for key, val in pairs(color) do
    directive = string.format("%s;%s=%s", directive, key, val)
  end
  return directive:lower()
end

--generic spinners
function updateSpinnerLabel(spinnerIndex, i) --TODO fix this, make it work for everything. works good for clothes colors right now
  local index = spinnerIndices[spinnerIndex]
  if index then
    widget.setText(spinners[index], speciesData.charGenTextLabels[index].." "..i)
  --if index < 7 or index == 10 then --dont save clothing stuff
  --  editorChoices[spinners[index]] = i
  --end
  end
end

--for this, we WANT to override existing items because this won't be called on existing characters... IN THE FUTURE!!!
function spinnerClothing(itemType, slotName, direction)
  local ejectCurrentItem = true
  local currentItem = (player.equippedItem(slotName) or {name = ""})
  local clothingOptions = speciesData.genders[genderIndex][itemType]

  if currentItem.name ~= "" then
    for i = 1, #clothingOptions do
      if currentItem.name == clothingOptions[i] then
        ejectCurrentItem = false
        break
      end
    end

    if ejectCurrentItem then
      player.giveItem(currentItem)
    end
  end

  currentItem = (currentItem or {name = ""}).name

  for i = 1, #clothingOptions do
    if currentItem == clothingOptions[i] then
      local styleNumber = i + direction
      styleNumber = (styleNumber > 0 and styleNumber <= #clothingOptions) and styleNumber or (direction > 0 and 1 or #clothingOptions)
      player.setEquippedItem(slotName, clothingOptions[styleNumber])
      updateSpinnerLabel(itemType, styleNumber)
      break
    elseif direction ~= 0 then
      player.setEquippedItem(slotName, clothingOptions[1])
      updateSpinnerLabel(itemType, 1)
    end
  end
end

--TODO: make it work nicely with items that have a non-standard amount of dye options
function spinnerClothingColor(itemType, slotName, direction)
  local currentItem = (player.equippedItem(slotName) or {name = "", parameters = {}})
  currentColorIndex = (currentItem.parameters.colorIndex or 0) + direction
  currentItem = currentItem.name
  if currentItem ~= "" then
    local colorOptions = getNumColorOptions(currentItem)
    local newColorIndex = math.min((currentColorIndex >= 0 and currentColorIndex <= colorOptions) and currentColorIndex or (direction >= 0 and 0 or colorOptions), colorOptions)
    player.setEquippedItem(slotName, {currentItem, 1, {colorIndex = newColorIndex}})
    updateSpinnerLabel(itemType.."Color", newColorIndex + 1)
  end
end

function getNumColorOptions(itemName)
  local colorOptions = root.itemConfig(itemName).config.colorOptions
  local res = -1
  for _, _ in pairs(colorOptions) do
    res = res + 1
  end
  return res
end

function getClothingForLabel(itemType, slot)
  local clothingOptions = speciesData.genders[genderIndex][itemType]
  local item = player.equippedItem(slot)
  if item then
    item = item.name
    for i = 1, #clothingOptions do
      if item == clothingOptions[i] then
        return i
      end
    end
  end
  return ""
end

function getClothingColorForLabel(item)
  if item then
    local colorOptions = getNumColorOptions(item.name)
    return math.min(colorOptions - 1, item.parameters and item.parameters.colorIndex or 0) + 1
  end
  return ""
end

--PERSONALITY
function spinnerPersonality(direction)
  local currentPersonality = player.personality()

  --I don't think the player will somehow have their personality fattened but check just in case
  if #currentPersonality == 0 then
    currentPersonality = {currentPersonality.idle, currentPersonality.armIdle, currentPersonality.headOffset, currentPersonality.armOffset}
  end

  --Compare current personality to all other personalities
  for i = 1, #personalities do
    if comparePersonality(currentPersonality, personalities[i]) then
      local personalityNumber = i + direction
      personalityNumber = wrap(personalityNumber, #personalities)
      player.setPersonality(fattenPersonality(personalities[personalityNumber]))
      updateSpinnerLabel("personality", personalityNumber)
      break
    elseif direction ~= 0 then
      player.setPersonality(fattenPersonality(personalities[1]))
      updateSpinnerLabel("personality", 1)
    end
  end

  personalityChoice = wrap(personalityChoice, #personalities)
end

function comparePersonality(p1, p2)
  if #p1 ~= #p2 then return false end
  for i = 1, #p1 do
    if type(p1[i]) == "table" then
      if p1[i][1] ~= p2[i][1] and p1[i][2] ~= p2[i][2] then
        return false
      end
    elseif p1[i] ~= p2[i] then
      return false
    end
  end
  return true
end

--Fatten the personality because god forbid anything in this game be consistent
function fattenPersonality(personality)
  return {
    idle = personality[1],
    armIdle = personality[2],
    headOffset = personality[3],
    armOffset = personality[4]
  }
end

function wrap(input, max)
  if input + 1 > max then
    input = input % max
  end

  return (input < 1 and max) or (input > max and 1) or input
end

--SPINNERS

--This sucks but we can't pass data from an up/down spinner so this will have to do
mainSkinColor, hairStyle, shirt, pants, alty, heady, shirtColor, pantsColor, personality = {}, {}, {}, {}, {}, {}, {}, {}, {}

--mainSkinColor
mainSkinColor.up   = function() mainSkinColor.main(1) end
mainSkinColor.down = function() mainSkinColor.main(-1) end
mainSkinColor.main = function(n)
  bodyColorChoice = wrap(bodyColorChoice + n, #speciesData["bodyColor"])
  updateHumanoidIdentity()
end

--hairStyle
hairStyle.up     = function() hairStyle.main(1) end
hairStyle.down   = function() hairStyle.main(-1) end
hairStyle.main   = function(n)
  hairChoice = wrap(hairChoice + n, #speciesData.genders[genderIndex]["hair"])
  updateHumanoidIdentity()
end

alty.up   = function() alty.main(1) end
alty.down = function() alty.main(-1) end
alty.main = function(n)
  altyChoice = altyChoice + n
  updateHumanoidIdentity()
end

--heady
heady.up   = function() heady.main(1) end
heady.down = function() heady.main(-1) end
heady.main = function(n)
  headyChoice = headyChoice + n
  updateHumanoidIdentity()
end

--personality
personality.up   = function() personality.main(1) end
personality.down = function() personality.main(-1) end
personality.main = function(n)
  personalityChoice = wrap(personalityChoice + n, #personalities)
  updateHumanoidIdentity()
end

--shirt
shirt.up   = function() shirt.main(1) end
shirt.down = function() shirt.main(-1) end
shirt.main = function(n) spinnerClothing("shirt", "chest", n) end

--pants
pants.up   = function() pants.main(1) end
pants.down = function() pants.main(-1) end
pants.main = function(n) spinnerClothing("pants", "legs", n) end

--shirtColor
shirtColor.up   = function() shirtColor.main(1) end
shirtColor.down = function() shirtColor.main(-1) end
shirtColor.main = function(n) spinnerClothingColor("shirt", "chest", n) end

--pantsColor
pantsColor.up   = function() pantsColor.main(1) end
pantsColor.down = function() pantsColor.main(-1) end
pantsColor.main = function(n) spinnerClothingColor("pants", "legs", n) end


--DEBUG FUNCTIONS
--[[
function createTooltip(p)
  local childAt = widget.getChildAt(p)
  if childAt then
    if childAt:find(".species.") then
      local tooltipValues = widget.getData(childAt:sub(2))
      if tooltipValues then
        tooltipValues = tooltipValues.tooltipValues
        if tooltip.descriptionLabel then tooltip.descriptionLabel.value = tooltipValues.description end
        if tooltip.title then tooltip.title.value = tooltipValues.title end
        if tooltip.subtitle then tooltip.subtitle.value = tooltipValues.subTitle end
        return tooltip
      end
    elseif player.isAdmin() then
      local name = widget.getChildAt(p)
      if name then
        return name
      end
    end
  end
end

function update()
  player.say(string.format("z_speciesSlots: %s\nspeciesOffset: %s\n#speciesOrdering: %s", z_speciesSlots, speciesOffset, #speciesOrdering))
end
--]]
