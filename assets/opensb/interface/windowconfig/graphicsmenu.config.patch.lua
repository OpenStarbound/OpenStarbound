-- gadgets
local function jcopy(base) return sb.jsonMerge(base, {}) end

local function clone(base, a, b)
  local copy = jcopy(base[a])
  base[b] = copy
  return copy
end

local function shift(thing, x, y)
  thing.position[1] = thing.position[1] + (tonumber(x) or 0)
  thing.position[2] = thing.position[2] + (tonumber(y) or 0)
  return thing
end

-- patch function, called by the game
function patch(config)
  local layout = config.paneLayout
  layout.panefeature.positionLocked = false
  layout.panefeature.anchor = "center"
  for i = 1, 32 do config.zoomList[i] = i end
  -- Create the camera pan speed widgets
  shift(clone(layout, "zoomLabel", "cameraSpeedLabel"), 100).value = "CAMERA PAN SPEED"
  shift(clone(layout, "zoomSlider", "cameraSpeedSlider"), 100)
  shift(clone(layout, "zoomValueLabel", "cameraSpeedValueLabel"), 100)
  config.cameraSpeedList = jarray()
  for i = 1, 50 do config.cameraSpeedList[i] = i / 10 end

  -- Create the interface scale widgets
  shift(clone(layout, "zoomLabel", "interfaceScaleLabel"), 0, 28).value = "INTERFACE SCALE"
  shift(clone(layout, "zoomSlider", "interfaceScaleSlider"), 0, 28)
  shift(clone(layout, "zoomValueLabel", "interfaceScaleValueLabel"), 0, 28)
  config.interfaceScaleList = {0, 1, 2, 3, 4, 5, 6} -- 0 = AUTO!

  -- Create anti-aliasing toggle
  shift(clone(layout, "multiTextureLabel", "antiAliasingLabel"), 98).value = "SUPER-SAMPLED AA"
  shift(clone(layout, "multiTextureCheckbox", "antiAliasingCheckbox"), 99)
  -- Create object lighting toggle
  shift(clone(layout, "multiTextureLabel", "objectLightingLabel"), 0, -11).value = "NEW OBJECT LIGHTS"
  shift(clone(layout, "multiTextureCheckbox", "objectLightingCheckbox"), 0, -11)
  -- Create hardware cursor toggle
  shift(clone(layout, "multiTextureLabel", "hardwareCursorLabel"), 98, -11).value = "HARDWARE CURSOR"
  shift(clone(layout, "multiTextureCheckbox", "hardwareCursorCheckbox"), 99, -11)

  shift(layout.title, 0, 24)
  shift(layout.resLabel, 0, 28)
  shift(layout.resSlider, 0, 28)
  shift(layout.resValueLabel, 0, 28)
  return config
end