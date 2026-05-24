require "/scripts/util.lua"
require "/scripts/vec2.lua"
require "/scripts/rect.lua"
require "/scripts/poly.lua"
require "/scripts/interp.lua"
require "/interface/cockpit/cockpitview.lua"
require "/interface/cockpit/cockpitutil.lua"

-- Pane callbacks

function init()
  View:init()

  self.clickEvents = {}
  self.input = {
    up = false,
    left = false,
    down = false,
    right = false
  }
  self.zoomOut = false
  self.selection = {}
  self.focus = {}
  self.viewCoordinate = nil
  self.travel = {}
  self.viewSelection = false
  self.cursorOverride = nil
  self.tooltipOverride = nil

  self.sounds = config.getParameter("sounds")
  self.playTyping = true

  self.state = FSM:new()
  if not contains(player.shipUpgrades().capabilities, "planetTravel") then
    self.state:set(disabledState)
  elseif celestial.skyInHyperspace() and celestial.currentSystem() then
    self.state:set(transitState)
  else
    self.state:set(systemScreenState, celestial.currentSystem())
  end

  widget.registerMemberCallback("bookmarksFrame.bookmarkList.bookmarkItemList", "editBookmark", editBookmark)
  widget.registerMemberCallback("bookmarksFrame.bookmarkList.bookmarkItemList", "selectBookmark", selectBookmark)

  pane.playSound(self.sounds.open)

  player.lounge(pane.sourceEntity())
end

function dismissed()
  for _,sound in pairs(self.sounds) do
    pane.stopAllSounds(sound)
  end
end

function canvasClickEvent(position, button, isButtonDown)
  if widget.hasFocus("consoleScreenCanvas") and not widget.active("jumpDialog") then
    showTopLeftFrame(nil)

    table.insert(self.clickEvents, {position, button, isButtonDown})
  end
end

function canvasKeyEvent(key, isDown)
  if key == 65 then
    self.input.up = isDown
  elseif key == 43 then
    self.input.left = isDown
  elseif key == 61 then
    self.input.down = isDown
  elseif key == 46 then
    self.input.right = isDown
  end
end

function takeInputEvents()
  local clicks = self.clickEvents
  self.clickEvents = {}
  return clicks
end

function createTooltip(screenPosition)
  local topLeftButtons = config.getParameter("topLeftButtons")
  for _,name in pairs(topLeftButtons) do
    if widget.inMember(name, screenPosition) then
      return config.getParameter(string.format("topLeftButtonTooltips.%s", name))
    end
  end
  if self.tooltipOverride then
    return self.tooltipOverride
  end
  return View:tooltip(screenPosition)
end

function cursorOverride(screenPosition)
  if widget.getChildAt(screenPosition) == ".consoleScreenCanvas" then
    return self.cursorOverride
  end
end

function update(dt)
  self.selection = {}
  self.cursorOverride = nil
  self.tooltipOverride = nil

  self.state:update(dt)

  View.questLocations = {}
  for _, p in pairs(player.questLocations()) do
    table.insert(View.questLocations, {p[1], p[2] and "tracked" or "untracked"})
  end

  View:render(dt)

  self.zoomOut = false
  self.viewSelection = false
  self.clickEvents = {}

  local questWorldId = player.currentQuestWorld()
  local coordinate = questWorldId and worldIdCoordinate(questWorldId)
  local questLocation = player.currentQuestLocation()
  if (questWorldId and coordinate) or questLocation then
    widget.setButtonEnabled("goToQuest", true)
  else
    widget.setButtonEnabled("goToQuest", false)
  end

  local bountyStation = player.getProperty("bountyStation") or {}
  bountyStation = bountyStation[player.serverUuid()]
  if bountyStation then
    widget.setButtonEnabled("goToPeacekeeper", true)
  else
    widget.setButtonEnabled("goToPeacekeeper", false)
  end

  -- OpenStarbound: show/hide fly button based on selected star
  local showFly = self.flySelection
    and canFlyShip(self.flySelection)
    and not compare(self.flySelection, celestial.currentSystem())
  widget.setVisible("flyToSelection", showFly and true or false)

  if not widget.active("coordinatesFrame") then
    setCoordinateTextbox(View.universeCamera.position, true)
  end
end

-- GUI Callbacks

function viewSelection()
  self.viewSelection = true
end

function toggleBookmarks()
  if widget.active("bookmarksFrame") then
    showTopLeftFrame(nil)
  else
    showTopLeftFrame("bookmarksFrame")
  end
  updateBookmarks()
end

function updateBookmarks()
  local list = "bookmarksFrame.bookmarkList.bookmarkItemList"
  widget.clearListItems(list)
  local orbitBookmarks = player.orbitBookmarks()
  table.sort(orbitBookmarks, function(a, b) return string.lower(a[2].bookmarkName) < string.lower(b[2].bookmarkName) end)
  for _,p in pairs(orbitBookmarks) do
    local system, bookmark = locationCoordinate(p[1]), p[2]
    local newItem = string.format("%s.%s", list, widget.addListItem(list))
    widget.setText(string.format("%s.name", newItem), bookmark.bookmarkName)
    widget.setText(string.format("%s.planetName", newItem), bookmark.targetName)

    local icon = string.format(config.getParameter("bookmarks.icon"), bookmark.icon)
    widget.setImage(string.format("%s.icon", newItem), icon)

    -- Set data to be passed to editBookmark and selectBookmark callbacks
    widget.setData(string.format("%s.editButton", newItem), p)
    widget.setData(string.format("%s.itemButton", newItem), {system, bookmark.target})
  end
end

function editBookmark(_, p)
  local system, bookmark = p[1], p[2]
  showBookmarkDialog(locationCoordinate(system), bookmark)
end

function selectBookmark(_, p)
  local system, target = p[1], p[2]
  if type(target) == "string" then
    target = {"object", target}
  else
    target = {"coordinate", target}
  end
  self.focus = {system = system, target = target}
end

function goToShip()
  local shipLocation = celestial.shipLocation()
  if shipLocation and shipLocation[1] == "orbit" then
    shipLocation = shipLocation[2].target
  end
  self.focus = {system = celestial.currentSystem(), target = shipLocation}
end

-- OpenStarbound: Fly button for controller/trackpad users without right-click
function flyToSelection()
  if self.flySelection then
    if canFlyShip(self.flySelection) and not compare(self.flySelection, celestial.currentSystem()) then
      pane.playSound(self.sounds.startTravel)
      self.travel = { system = self.flySelection.location }
    end
  end
end

function goToQuest()
  local questWorldId = player.currentQuestWorld()
  if questWorldId then
    local coordinate = worldIdCoordinate(questWorldId)
    if coordinate then
      if player.isMapped(coordinate) then
        self.focus = {system = coordinateSystem(coordinate), target = {"coordinate", coordinate}}
      else
        self.focus = {system = coordinateSystem(coordinate)}
      end
      return
    end
  end
  local questLocation = player.currentQuestLocation()
  if questLocation then
    self.focus = {system = locationCoordinate(questLocation.system), target = questLocation.location}
  end
end

function goToPeacekeeper()
  local bountyStation = player.getProperty("bountyStation")
  bountyStation = bountyStation[player.serverUuid()]
  local system, target
  if bountyStation.system then
    system = bountyStation.system

    if bountyStation.uuid then
      target = {"object", bountyStation.uuid}
    end
  end
  self.focus = {system = system, target = target}
end

function zoomOut()
  self.zoomOut = true
end

function addBookmark()
  local bookmark
  if self.selection.planet then
    bookmark = newPlanetBookmark(self.selection.planet)
  elseif self.selection.object then
    bookmark = newObjectBookmark(self.selection.object.uuid, self.selection.object.typeName)
  end
  if not bookmark then return end
  bookmark = existingBookmark(self.selection.system, bookmark) or bookmark

  showBookmarkDialog(self.selection.system, bookmark)
end

function showTopLeftFrame(showFrame)
  for frame in pairs(config.getParameter("topLeftFrameButtons")) do
    widget.setVisible(frame, false)
  end

  local buttonIndex = 1
  local frameOffset = {0, 0}
  local spacing = config.getParameter("topLeftSpacing")
  local buttons = config.getParameter("topLeftButtons")
  if showFrame then
    widget.setVisible(showFrame, true)
    frameOffset = vec2.add(widget.getSize(showFrame), spacing)
    local button = config.getParameter(string.format("topLeftFrameButtons.%s", showFrame))
    _,buttonIndex = util.find(buttons, function(s) return s == button end)
  end

  local height = config.getParameter("topLeftButtonHeight")
  local anchor = widget.getPosition(buttons[1])
  for i = 1, #buttons do
    local position = {anchor[1], anchor[2] - ((height + spacing) * (i - 1))}
    if i > buttonIndex then
      position[2] = position[2] - frameOffset[2]
    end
    widget.setPosition(buttons[i], position)
  end
end

function showBookmarkDialog(system, bookmark)
  local existing = existingBookmark(system, bookmark)
  bookmark = existing or bookmark

  widget.setVisible("editBookmarkFrame", true)
  if existing then
    widget.setText("editBookmarkFrame.title", config.getParameter("bookmarks.editTitle"))
    widget.setButtonEnabled("editBookmarkFrame.remove", true)
  else
    widget.setText("editBookmarkFrame.title", config.getParameter("bookmarks.newTitle"))
    widget.setButtonEnabled("editBookmarkFrame.remove", false)
  end
  widget.setImage("editBookmarkFrame.icon", string.format(config.getParameter("bookmarks.icon"), bookmark.icon))
  widget.setText("editBookmarkFrame.planetName", bookmark.targetName)
  widget.setText("editBookmarkFrame.name", bookmark.bookmarkName)
  widget.focus("editBookmarkFrame.name")

  -- Set data to be passed to the confirmBookmark and deleteBookmark callbacks
  widget.setData("editBookmarkFrame.ok", {system, bookmark})
  widget.setData("editBookmarkFrame.name", {system, bookmark})
  widget.setData("editBookmarkFrame.remove", {system, bookmark})
end

function hideBookmarkDialog()
  widget.setVisible("editBookmarkFrame", false)
  updateBookmarks()
end

function confirmBookmark(_, p)
  local system, bookmark = p[1], p[2]
  bookmark.bookmarkName = widget.getText("editBookmarkFrame.name")
  if existingBookmark(system, bookmark) then
    player.removeOrbitBookmark(system, bookmark)
  end
  player.addOrbitBookmark(system, bookmark)
  hideBookmarkDialog()
end

function deleteBookmark(_, p)
  local system, bookmark = p[1], p[2]
  player.removeOrbitBookmark(system, bookmark)
  hideBookmarkDialog()
end

function cancelBookmark()
  hideBookmarkDialog()
end

function goToCoordinate()
  local x = tonumber(widget.getText("coordinatesFrame.x")) or 0
  local y = tonumber(widget.getText("coordinatesFrame.y")) or 0
  if x == "" or y == "" then
    return
  end
  self.viewCoordinate = {x, y}
end

function setCoordinateTextbox(position, force)
  local clamped = vec2.floor(View:clampUniversePosition(position))
  if not compare(clamped, position) or force then
    self.playTyping = false
    widget.setText("coordinatesFrame.x", tostring(clamped[1]))
    self.playTyping = false
    widget.setText("coordinatesFrame.y", tostring(clamped[2]))
  end
end

function coordinateChanged(widgetName)
  if self.playTyping then
    pane.playSound(self.sounds.typing)
  end
  self.playTyping = not self.playTyping
  local position = {
    tonumber(widget.getText("coordinatesFrame.x")) or 0,
    tonumber(widget.getText("coordinatesFrame.y")) or 0
  }
  setCoordinateTextbox(position)
end

function toggleCoordinates()
  if widget.active("coordinatesFrame") then
    showTopLeftFrame(nil)
  else
    showTopLeftFrame("coordinatesFrame")
  end
end

function showJumpDialog(system, target)
  local fuelAmount = world.getProperty("ship.fuel")
  if fuelAmount then
    widget.setVisible("jumpDialog", true)

    local cost = fuelCost()
    if player.isAdmin() or fuelAmount >= cost then
      widget.setText("jumpDialog.text", string.format(config.getParameter("jumpDialog.valid"), cost))
      widget.setButtonEnabled("jumpDialog.jump", true)
      widget.setData("jumpDialog.jump", {system = system, target = target})

      self.travel.confirmed = false
    else
      widget.setText("jumpDialog.text", string.format(config.getParameter("jumpDialog.invalid"), cost))
      widget.setButtonEnabled("jumpDialog.jump", false)
      widget.setData("jumpDialog.jump", nil)
    end
  end
end

function hideJumpDialog()
  if widget.active("jumpDialog") then
    widget.setData("jumpDialog.jump", nil)
    widget.setVisible("jumpDialog", false)
    if not self.travel.confirmed then
      self.travel = {}
    end
  end
end

function confirmJump(_, jump)
  if jump and compare(self.travel.system, jump.system) and compare(self.travel.target, jump.target) then
    self.travel.confirmed = true
    pane.playSound(self.sounds.jump)
  end
  hideJumpDialog()
end

function cancelJump()
  hideJumpDialog()
end

-- Helpers

function fuelCost()
  local cost = config.getParameter("jumpFuelCost")
  return util.round(cost - cost * (world.getProperty("ship.fuelEfficiency") or 0.0))
end

function canFlyShip(system)
  if not compare(system, celestial.currentSystem()) then
    return not celestial.flying() and not celestial.skyFlying()
  else
    return celestial.skyFlyingType() ~= "arriving"
  end
end

-- GUI states

function disabledState()
  View:reset()
  widget.setVisible("disabledLabel", true)

  local flickerTime = config.getParameter("disabledFlickerTime")
  local flicker = config.getParameter("disabledFlicker")
  local color = config.getParameter("disabledColor")

  local timer = 0
  while not contains(player.shipUpgrades().capabilities, "planetTravel") do
    if timer <= 0 then
      color[4] = (flicker[1] + (math.random() * (flicker[2] - flicker[1]))) * 255
      timer = flickerTime
      widget.setFontColor("disabledLabel", color)
    end
    timer = timer - script.updateDt()
    coroutine.yield()
  end

  widget.setVisible("disabledLabel", false)
  return self.state:set(systemScreenState, celestial.currentSystem())
end

function transitState()
  View:reset()
  View:setCamera("universe", systemPosition(celestial.currentSystem()), "system")
  widget.setVisible("transitLabel", true)

  local color = config.getParameter("transitColor")
  local pulse = config.getParameter("transitPulse")

  local timer = 0
  while celestial.skyFlyingType() == "warp" do
    timer = timer + script.updateDt()
    local ratio = (math.sin((timer % 1) / 1 * math.pi * 2) + 1.0) / 2.0
    color[4] = 255 * (pulse[1] + (ratio * (pulse[2] - pulse[1])))
    widget.setFontColor("transitLabel", color)
    coroutine.yield()
  end

  widget.setVisible("transitLabel", false)
  return self.state:set(systemScreenState, celestial.currentSystem())
end

function systemUniverseTransition(fromSystem)
  widget.setVisible("zoomOut", false)
  widget.setVisible("remoteTitle", false)
  widget.setVisible("remoteDescription", false)
  widget.setVisible("systemName", false)

  View:reset()
  pane.playSound(self.sounds.zoom, -1)
  View.drawSystem = true
  View.system.system = fromSystem
  View.system.planets = celestial.children(fromSystem)
  View.system.scaleObjects = true

  if compare(fromSystem, celestial.currentSystem()) then
    View.drawShip = true
  end
  View.drawStars = true
  View.stars.currentSystem = fromSystem

  local timer = 0
  local transitionTime = 1.0
  local position = systemPosition(fromSystem)
  local systemScale = View:systemScale(fromSystem)
  while timer < transitionTime do
    local ratio = 1 - ((1 - timer / transitionTime) ^ 4)
    View:transitionCamera("universe", ratio, {position, position}, {systemScale, "universe"})
    View:transitionCamera("system", ratio, {{0, 0}, {0, 0}}, {systemScale, "universe"})
    View.backgroundScale = interp.linear(ratio, View:bgScale("system"), View:bgScale("universe"))

    View.stars.systems = celestial.scanSystems(View:systemScanRegion())
    View.stars.lines = celestial.scanConstellationLines(View:systemScanRegion())
    View.stars.currentOpacity = math.max(0.0, (timer / transitionTime - 0.7) / 0.3)
    View.stars.lineOpacity = timer / transitionTime
    View.ship.scale = View.systemCamera.scale / systemScale

    coroutine.yield()
    timer = timer + script.updateDt()
  end

  pane.stopAllSounds(self.sounds.zoom)
  self.state:set(universeScreenState, fromSystem)
end

function universeScreenState(startSystem)
  widget.setVisible("zoomOut", false)

  View:reset()

  if startSystem then
    View:setCamera("universe", systemPosition(startSystem), "universe")
  end
  View.backgroundScale = View:bgScale("universe")
  View.drawStars = true
  View.stars.currentSystem = celestial.currentSystem()
  View.stars.displayMarker = true

  local scrollSpeed = config.getParameter("universeScrollSpeed")

  local systems = {}
  local lines = {}
  local selection = nil
  local startDrag, drag
  local hover
  while true do
    systems = celestial.scanSystems(View:systemScanRegion())
    lines = celestial.scanConstellationLines(View:systemScanRegion())
    View.stars.systems = systems
    View.stars.lines = lines

    local newHover = closestSystemInRange(View:toUniverse(View:mousePosition()), systems, 2.5)
    if newHover and not compare(newHover, selection) then
      local tooltip = config.getParameter("systemTooltip")
      local tooltipGui = config.getParameter("systemTooltipConfig")

      local name = celestial.planetName(newHover)
      if name then
        tooltipGui.name.value = name

        local starType = celestial.planetParameters(newHover).typeName or "default"
        tooltipGui.type.value = config.getParameter(string.format("starTypeNames.%s", starType))
        tooltipGui.type.color = config.getParameter(string.format("starTypeColors.%s", starType))

        local explored = player.isMapped(newHover)
        if explored then
          tooltipGui.explored.value, tooltipGui.explored.color = tooltip.exploredLabel.explored, tooltip.exploredLabelColor.explored
        else
          tooltipGui.explored.value, tooltipGui.explored.color = tooltip.exploredLabel.unexplored, tooltip.exploredLabelColor.unexplored
        end

        self.tooltipOverride = tooltipGui;
      end

      if not compare(newHover, hover) then
        pane.playSound(self.sounds.hover)
      end
    end
    hover = newHover

    if self.viewCoordinate then
      local position = self.viewCoordinate
      self.viewCoordinate = nil
      return self.state:set(universeMoveState, View.universeCamera.position, systems, position, false)
    end

    if self.focus.system then
      if compare(View.universeCamera.position, systemPosition(self.focus.system)) then
        local viewSystem = player.isMapped(self.focus.system) and self.focus.target ~= nil
        if viewSystem and self.focus.target[1] == "object" then
          local mappedObjects = player.mappedObjects(self.focus.system)
          viewSystem = mappedObjects[self.focus.target[2]] ~= nil
        end
        if viewSystem then
          return self.state:set(universeSystemTransition, systems, lines, self.focus.system, false)
        else
          selection = self.focus.system
          View:select(selection, {"coordinate", selection})
          View:showSystemInfo(selection)
          self.focus = {}
        end
      else
        return self.state:set(universeMoveState, View.universeCamera.position, systems, systemPosition(self.focus.system), false)
      end
    end

    -- dragging starts if the mouse is moved some threshold distance away from the start position
    if startDrag and (drag or vec2.mag(vec2.sub(View:mousePosition(), startDrag.screen)) > 2) then
      drag = vec2.sub(startDrag.screen, View:mousePosition())
      local position = vec2.add(startDrag.universe, vec2.div(drag, View:scale("universe", "universe")))
      View:setCamera("universe", View:clampUniversePosition(position), "universe")
    else
      -- if not dragging allow keyboard scrolling
      local dir = {0, 0}
      if self.input.up then
        dir[2] = dir[2] + 1
      end
      if self.input.down then
        dir[2] = dir[2] - 1
      end
      if self.input.left then
        dir[1] = dir[1] - 1
      end
      if self.input.right then
        dir[1] = dir[1] + 1
      end
      local position = vec2.add(View.universeCamera.position, vec2.mul(vec2.norm(dir), scrollSpeed * script.updateDt()))
      View:setCamera("universe", position, "universe")
    end

    for _,click in pairs(takeInputEvents()) do
      local clickPosition, button, down = click[1], click[2], click[3]
      if button == 0 then
        if down then
          if hover then
            if compare(selection, hover) then
              -- zoom into system if it's explored
              self.viewSelection = true
            else
              selection = hover
              self.flySelection = hover  -- OpenStarbound: track for fly button
              -- wait until system is loaded
              while #celestial.children(selection) == 0 do
                coroutine.yield()
              end

              View:select(selection, {"coordinate", selection})
              View:showSystemInfo(selection)
            end
          else
            -- start dragging from the current camera position
            startDrag = {screen = clickPosition, universe = View.universeCamera.position}
          end
        else
          -- clear selection only if the player has clicked in empty space without dragging
          if startDrag and not drag then
            selection = nil
            self.flySelection = nil  -- OpenStarbound: clear fly button
            View:select(nil)
            View:showSystemInfo(false)
          end

          startDrag = nil
          drag = nil
        end
      end

      if button == 2 and down and hover then
        if canFlyShip(hover) and not compare(hover, celestial.currentSystem()) then
          pane.playSound(self.sounds.startTravel)
          self.travel = { system = hover.location }
        else
          pane.playSound(self.sounds.failTravel)
        end
      end
    end

    if selection and self.viewSelection then
      if player.isMapped(selection) then
        return self.state:set(universeSystemTransition, systems, lines, selection, false)
      end
    end

    if self.travel.system then
      if self.travel.confirmed then
        local fuelAmount = world.getProperty("ship.fuel")
        local cost = fuelCost()
        if player.isAdmin() or (fuelAmount and fuelAmount >= cost) then
          local currentSystem = celestial.currentSystem()
          local travelSystem = {location = self.travel.system, planet = 0, satellite = 0}
          View.stars.hover = nil

          while not celestial.scanRegionFullyLoaded(View:systemScanRegion())
             or not celestial.scanRegionFullyLoaded(View:systemScanRegion(systemPosition(currentSystem))) do
            celestial.scanSystems(View:systemScanRegion())
            celestial.scanSystems(View:systemScanRegion(systemPosition(currentSystem)))
            coroutine.yield()
          end

          local startSystems = celestial.scanSystems(View:systemScanRegion())
          local endSystems = celestial.scanSystems(View:systemScanRegion(systemPosition(currentSystem)))
          local queuedTravel = {systemPosition(currentSystem), endSystems, systemPosition(travelSystem), {system = self.travel.system, target = self.travel.target}}
          self.travel = {}
          return self.state:set(universeMoveState, View.universeCamera.position, startSystems, systemPosition(currentSystem), false, queuedTravel)
        else
          self.travel = {}
        end
      elseif self.travel.confirmed == nil then
        showJumpDialog(self.travel.system, self.travel.target)
      end
    end

    if compare(hover, selection) then
      hover = nil
    end
    View.stars.hoverSystem = hover

    coroutine.yield()
  end
end

function universeMoveState(startPosition, systems, toPosition, travel, queued)
  View:reset()
  if not travel and not queued then
    pane.playSound(self.sounds.pan, -1)
  end

  View.drawTravel = true
  local finalRegion = View:systemScanRegion(toPosition)
  View.travel.finalRegion = finalRegion
  View.travel.stars = util.map(systems, function(s)
      return {s, systemPosition(s)}
    end)
  View.travel.lines = celestial.scanConstellationLines(View:systemScanRegion())
  View.travel.currentSystem = celestial.currentSystem()
  View.travel.displayMarker = true

  if travel and not celestial.skyFlying() then
    if not player.isAdmin() then
      world.setProperty("ship.fuel", world.getProperty("ship.fuel") - fuelCost())
    end
    celestial.flyShip(travel.system, travel.target)
    while not celestial.skyFlying() do
      coroutine.yield()
    end
  end

  while not celestial.scanRegionFullyLoaded(finalRegion) do
    celestial.scanSystems(finalRegion)
    coroutine.yield()
  end
  local finalLines = celestial.scanConstellationLines(finalRegion)
  util.appendLists(View.travel.lines, finalLines)
  local finalStars = celestial.scanSystems(finalRegion)
  View.travel.finalStars = finalStars

  View:setCamera("universe", startPosition, "universe")
  if not compare(startPosition, toPosition) then
    -- wait to disembark
    if travel then
      View.travel.travelLine = {startPosition, toPosition}
      while celestial.skyFlyingType() == "disembarking" do
        coroutine.yield()
      end
    end
    View.travel.currentSystem = nil

    local timer = 0
    local transitionTime = 1.0
    local start = View.universeCamera.position
    local delta = vec2.sub(toPosition, start)

    local exp = math.max(2, math.log(vec2.mag(delta)) / math.log(10))
    while true do
      local ratio
      if travel then
        -- if traveling, track sky flying progress
        if celestial.skyFlyingType() ~= "warp" then
          break
        end
        ratio = celestial.skyWarpProgress()
      else
        -- if moving the camera just use a timer
        if timer >= transitionTime then
          break
        end
        ratio = timer / transitionTime
      end
      View.universeCamera.position = {
        util.easeInOutExp(ratio, start[1], delta[1], exp),
        util.easeInOutExp(ratio, start[2], delta[2], exp)
      }
      coroutine.yield()
      timer = timer + script.updateDt()
    end

    View.travel.travelLine = nil

    View:setCamera("universe", toPosition, "universe")
  end

  if not travel and not queued then
    pane.stopAllSounds(self.sounds.pan)
  end
  if queued then
    self.state:set(universeMoveState, table.unpack(queued))
  elseif travel then
    self.state:set(universeSystemTransition, finalStars, finalLines, locationCoordinate(travel.system), true)
  else
    self.state:set(universeScreenState)
  end
end

function universeSystemTransition(systems, lines, toSystem, warpIn)
  widget.setVisible("zoomOut", false)

  View:reset()
  pane.playSound(self.sounds.zoom, -1)
  View.drawStars = true
  View.stars.systems = systems
  View.stars.lines = lines
  View.stars.currentSystem = toSystem

  while #celestial.children(toSystem) == 0 do
    coroutine.yield()
  end

  View.drawSystem = true
  View.system.system = toSystem
  View.system.planets = celestial.children(toSystem)
  View.system.scaleObjects = true
  if not compare(celestial.currentSystem(), toSystem) then
    View.system.mappedObjects = player.mappedObjects(toSystem)
  end

  local isCurrent = compare(toSystem, celestial.currentSystem())
  if isCurrent then
    if not warpIn then
      View.drawShip = true
    end
  else
    widget.setVisible("remoteTitle", true)
    widget.setVisible("remoteDescription", true)
  end

  local universeStart = View.universeCamera.position
  local systemStart = vec2.mul(vec2.sub(universeStart, systemPosition(toSystem)), View.settings.universeSystemRatio)

  local timer = 0
  local transitionTime = 1.0
  local systemScale = View:systemScale(toSystem)
  while timer < transitionTime do
    local ratio = (timer / transitionTime) ^ 4
    View:transitionCamera("universe", ratio, {universeStart, systemPosition(toSystem)}, {"universe", systemScale})
    View:transitionCamera("system", ratio, {systemStart, {0, 0}}, {"universe", systemScale})
    View.backgroundScale = interp.linear(ratio, View:bgScale("universe"), View:bgScale("system"))

    View.stars.currentOpacity = math.max(0.0, (0.3 - timer / transitionTime) / 0.3)
    View.stars.lineOpacity = 1 - (timer / transitionTime)
    View.ship.scale = View.systemCamera.scale / systemScale

    coroutine.yield()
    timer = timer + script.updateDt()
  end

  pane.stopAllSounds(self.sounds.zoom)
  self.state:set(systemScreenState, toSystem, warpIn)
end

function systemScreenState(system, warpIn)
  widget.setVisible("zoomOut", true)

  View:reset()
  local planets = util.untilNotEmpty(function() return celestial.children(system) end)

  widget.setVisible("systemName", true)
  widget.setText("systemName", celestial.planetName(system))

  local scale = View:systemScale(system)
  View:setCamera("system", {0, 0}, scale)
  View:setCamera("universe", systemPosition(system), "system")
  View.backgroundScale = View:bgScale("system")

  local isCurrent = compare(system, celestial.currentSystem())

  View.drawSystem = true
  View.system.system = system
  View.system.planets = planets
  local mappedObjects = player.mappedObjects(system)
  if not isCurrent then
    View.system.mappedObjects = mappedObjects
  end

  if isCurrent then
    View.drawShip = true
    View.ship.scale = 1.0
    if warpIn then
      View.playerWarpIn = 0
    end
  end

  local hover = nil
  local selection = nil
  local destination = nil
  local flyState = nil
  while true do
    local newHover = closestLocationInRange(View:toSystem(View:mousePosition()), system, 10)
    if newHover and not compare(newHover, hover) and not compare(newHover, selection) then
      pane.playSound(self.sounds.hover)
    end
    hover = closestLocationInRange(View:toSystem(View:mousePosition()), system, 10)

    if self.viewCoordinate then
      return self.state:set(systemUniverseTransition, system)
    end

    if self.focus.system then
      if compare(self.focus.system, system) then
        if self.focus.target then
          if self.focus.target[1] == "coordinate" then
            for _,planet in pairs(planets) do
              if compare(planet, coordinatePlanet(self.focus.target[2])) then
                return self.state:set(systemPlanetTransition, planets, planet)
              end
            end
          elseif self.focus.target[1] == "object" then
            selection = self.focus.target
            View:select(system, selection)
            if isCurrent then
              View:showObjectInfo(system, {
                uuid = self.focus.target[2],
                type = celestial.objectType(self.focus.target[2]),
                parameters = celestial.objectParameters(self.focus.target[2])
              })
            elseif mappedObjects[self.focus.target[2]] ~= nil then
              View:showObjectInfo(system, {
                uuid = self.focus.target[2],
                type = mappedObjects[self.focus.target[2]].typeName,
                parameters = mappedObjects[self.focus.target[2]].parameters
              })
            end
          end
        end
        self.focus = {}
      else
        return self.state:set(systemUniverseTransition, system)
      end
    end

    if self.zoomOut then
      return self.state:set(systemUniverseTransition, system)
    end

    local zoomOut = vec2.mag(View:toSystem(View:mousePosition())) - 20 > (View.settings.viewRadius) / View.systemCamera.scale
    if zoomOut then
      self.cursorOverride = config.getParameter("zoomOutCursor")
    end

    for _,click in pairs(takeInputEvents()) do
      local position, button, down = click[1], click[2], click[3]
      if down then
        if button == 0 then
          if zoomOut then
            return self.state:set(systemUniverseTransition, system)
          else
            if compare(selection, hover) then
              self.viewSelection = true
            else
              selection = hover
              View:select(system, selection)

              -- clicking currently orbited planet or moons of the planet zooms in
              if selection and selection[1] == "coordinate" then
                View:showClusterInfo(selection[2])
              else
                View:showClusterInfo(false)
              end

              if selection and selection[1] == "object" then
                if isCurrent then
                  View:showObjectInfo(system, {
                    uuid = selection[2],
                    type = celestial.objectType(selection[2]),
                    parameters = celestial.objectParameters(selection[2])
                  })
                else
                  View:showObjectInfo(system, {
                    uuid = selection[2],
                    type = mappedObjects[selection[2]].typeName,
                    parameters = mappedObjects[selection[2]].parameters
                  })
                end
              else
                View:showObjectInfo(false)
              end
            end
          end
        end
        if button == 2 then
          if canFlyShip(system) then
            pane.playSound(self.sounds.startTravel)
            local to = hover or View:toSystem(position)
            if to[1] == "coordinate" and celestial.planetParameters(to[2]).worldType ~= "FloatingDungeon" then
              local planetPosition = celestial.planetPosition(to[2])
              local orbitRadius = math.max(10, (celestial.clusterSize(to[2]) / 2.0) + 2.0)
              local orbitPosition = vec2.mul(vec2.norm(vec2.sub(planetPosition, celestial.shipSystemPosition())), orbitRadius)
              self.travel = {
                system = system.location,
                target = {"orbit", {
                  target = to[2],
                  enterPosition = orbitPosition,
                  enterTime = world.time(),
                  direction = util.randomDirection()
                }}
              }
            else
              self.travel = { system = system.location, target = to }
            end
          else
            pane.playSound(self.sounds.failTravel)
          end
        end
      end
    end

    if selection and selection[1] == "coordinate" and self.viewSelection and player.isMapped(selection[2]) then
      return self.state:set(systemPlanetTransition, planets, selection[2])
    end

    if self.travel.system then
      if isCurrent and compare(self.travel.system, system.location) then
        if self.travel.target then
          if self.travel.target[1] ~= "coordinate" or celestial.visitableParameters(self.travel.target[2]) then
            celestial.flyShip(self.travel.system, self.travel.target)
          end
        end
        self.travel = {}
      else
        return self.state:set(systemUniverseTransition, system)
      end
    end

    local destination = celestial.shipDestination()
    if destination then
      local shipLocation = celestial.shipLocation()
      if shipLocation then
        if compare(shipLocation, destination)
           or (shipLocation[1] == "orbit" and destination[1] == "orbit" and compare(shipLocation[2].target, destination[2].target)) then
          local planet
          if destination[1] == "orbit" then planet = destination[2].target end
          if destination[1] == "coordinate" then planet = destination[2] end

          destination = nil
          if planet then
            return self.state:set(systemPlanetTransition, planets, coordinatePlanet(planet))
          end
        end
      end
    end

    if compare(hover, selection) then
      hover = nil
    end
    View.system.destination = destination
    View.system.hover = hover

    if selection and selection[1] == "object" and (isCurrent or mappedObjects[uuid] ~= nil) then
      local uuid = selection[2]
      local typeName = isCurrent and celestial.objectType(uuid) or mappedObjects[uuid].typeName
      local parameters = isCurrent and celestial.objectParameters(uuid) or mappedObjects[uuid].parameters
      if celestial.objectTypeConfig(typeName).permanent then
        self.selection = {system = system, object = {uuid = uuid, typeName = typeName}}
      end
    end

    coroutine.yield()
  end

  self.state:set(systemUniverseTransition, system)
end

function systemPlanetTransition(planets, toPlanet)
  widget.setVisible("zoomOut", false)
  widget.setVisible("systemName", false)

  View:reset()
  pane.playSound(self.sounds.zoom, -1)
  local system = coordinateSystem(toPlanet)
  View.drawPlanet = true
  View.planet.planet = toPlanet
  View.planet.drawObjects = true

  View.drawSystem = true
  View.system.system = coordinateSystem(toPlanet)
  View.system.planets = planets
  View.system.fadePlanet = toPlanet
  View.system.drawObjects = false

  if compare(system, celestial.currentSystem()) then
    View.planet.mappedObjects = player.mappedObjects(system)
    View.drawShip = true
    View.ship.scale = 1.0
  end

  local timer = 0
  local transitionTime = 1.0
  local planetScale = View:planetScale(toPlanet)
  local systemScale = View:systemScale(coordinateSystem(toPlanet))
  while timer < transitionTime do
    local ratio = timer / transitionTime
    local planetPosition = celestial.planetPosition(toPlanet)
    View:transitionCamera("system", ratio ^ 3, {{0, 0}, planetPosition}, {systemScale, planetScale})
    View.backgroundScale = interp.linear(ratio, View:bgScale("system"), View:bgScale("planet"))

    View.planet.orbitOpacity = ratio
    View.system.orbitOpacity = math.max(0, (0.1 - math.min(0.1, (ratio ^ 3))) * 10)
    View.system.fadeOpacity = View.system.orbitOpacity

    timer = timer + script.updateDt()
    coroutine.yield()
  end

  pane.stopAllSounds(self.sounds.zoom)
  self.state:set(planetScreenState, toPlanet)
end

function planetScreenState(planet)
  widget.setVisible("zoomOut", true)

  View:reset()

  View.backgroundScale = View:bgScale("planet")

  View.drawPlanet = true
  View.planet.planet = planet

  local system = coordinateSystem(planet)
  local isCurrent = compare(system, celestial.currentSystem())
  if isCurrent then
    View.planet.drawObjects = true
    View.drawShip = true
    View.ship.scale = 1.0
  else
    View.planet.mappedObjects = player.mappedObjects(system)
  end

  local hover = nil
  local selection = nil
  local planetScale = View:planetScale(planet)
  while true do
    View:setCamera("system", celestial.planetPosition(planet), planetScale)

    if self.viewCoordinate then
      return self.state:set(planetSystemTransition, planet)
    end

    if self.focus.system then
      if self.focus.target and self.focus.target[1] == "coordinate" then
        local planets = {planet}
        util.appendLists(planets, celestial.children(planet))
        for _,planet in pairs(planets) do
          if compare(planet, self.focus.target[2]) then
            selection = {"coordinate", planet}
            View:showPlanetInfo(planet)
            self.focus = {}
            break
          end
        end
      else
        return self.state:set(planetSystemTransition, planet)
      end
    end

    if self.zoomOut then
      return self.state:set(planetSystemTransition, planet)
    end

    local newHover = closestLocationInRange(View:toSystem(View:mousePosition()), planet, 3)
    if newHover and not compare(hover, newHover) and not compare(newHover, selection) then
      pane.playSound(self.sounds.hover)
    end
    hover = newHover

    local zoomOut = planetDistance(planet, View:toSystem(View:mousePosition())) - 2 > (View.settings.viewRadius) / View.systemCamera.scale
    if zoomOut then
      self.cursorOverride = config.getParameter("zoomOutCursor")
    end

    for _,click in pairs(takeInputEvents()) do
      local position, button, down = click[1], click[2], click[3]
      if down then
        if button == 0 then
          if zoomOut then
            return self.state:set(planetSystemTransition, planet)
          end

          if not compare(hover, select) then
            selection = hover
            if selection and selection[1] == "coordinate" then
              View:showPlanetInfo(selection[2])
            else
              View:showPlanetInfo(false)
            end

            if selection and selection[1] == "object" and isCurrent then
                View:showObjectInfo(system, {
                  uuid = selection[2],
                  type = celestial.objectType(selection[2]),
                  parameters = celestial.objectParameters(selection[2])
                })
            else
              View:showObjectInfo(false)
            end
          end
        end
        if button == 2 and hover then
          if canFlyShip(system) and locationVisitable(hover) then
            pane.playSound(self.sounds.startTravel)
            self.travel = {system = system.location, target = hover}
          else
            pane.playSound(self.sounds.failTravel)
          end
        end
      end
    end

    if self.travel.system then
      if self.travel.target then
        local shipLocation = celestial.shipLocation()
        if shipLocation and self.travel.target[1] == "coordinate" then
          local orbiting
          if shipLocation[1] == "orbit" then
            orbiting = shipLocation[2].target
          elseif shipLocation[1] == "coordinate" then
            orbiting = shipLocation[2]
          end
          if orbiting and compare(coordinatePlanet(orbiting), coordinatePlanet(self.travel.target[2])) then
            celestial.flyShip(self.travel.system, self.travel.target)
            self.travel = {}
          end
        end
      end

      if self.travel.system then
        return self.state:set(planetSystemTransition, planet)
      end
    end

    if compare(hover, select) then
      hover = nil
    end
    View.planet.hover = hover
    if selection then
      View:select(system, selection)
    else
      View:select(nil)
    end

    if selection and selection[1] == "coordinate" then
      self.selection = {system = coordinateSystem(selection[2]), planet = selection[2]}
    end

    coroutine.yield()
  end
end

function planetSystemTransition(fromPlanet)
  widget.setVisible("zoomOut", false)

  View:reset()
  pane.playSound(self.sounds.zoom, -1)
  local system = coordinateSystem(fromPlanet)
  local planets = util.untilNotEmpty(function() return celestial.children(system) end)

  View.drawPlanet = true
  View.planet.planet = fromPlanet
  View.planet.drawObjects = true

  View.drawSystem = true
  View.system.system = system
  View.system.planets = planets
  View.system.objects = celestial.systemObjects()
  View.system.fadePlanet = fromPlanet
  View.system.fadeOpacity = 0.0
  View.system.drawObjects = false

  if compare(system, celestial.currentSystem()) then
    View.planet.mappedObjects = player.mappedObjects(system)
    View.drawShip = true
    View.ship.scale = 1.0
  end

  local timer = 0
  local transitionTime = 1.0
  local systemScale = View:systemScale(system)
  local planetScale = View:planetScale(fromPlanet)
  while timer < transitionTime do
    local ratio = 1 - ((1 - (timer / transitionTime)) ^ 3)
    local planetPosition = celestial.planetPosition(fromPlanet)
    View:transitionCamera("system", ratio, {planetPosition, {0, 0}}, {planetScale, systemScale})

    View.planet.orbitOpacity = 1.0 - ratio
    View.system.orbitOpacity = math.max(0, (ratio - 0.9) * 10)
    View.system.fadeOpacity = View.system.orbitOpacity

    timer = timer + script.updateDt()
    coroutine.yield()
  end

  pane.stopAllSounds(self.sounds.zoom)
  self.state:set(systemScreenState, system, false)
end
