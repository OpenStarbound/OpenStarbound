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
  -- Create the camera pan speed widgets
  shift(clone(layout, "zoomLabel", "cameraSpeedLabel"), 100).value = "CAMERA PAN SPEED"
  shift(clone(layout, "zoomSlider", "cameraSpeedSlider"), 100)
  shift(clone(layout, "zoomValueLabel", "cameraSpeedValueLabel"), 100)
  -- Populate camera speed list
  config.cameraSpeedList = jarray()
  for i = 1, 50 do config.cameraSpeedList[i] = i / 10 end
  for i = 1, 32 do config.zoomList[i] = i end
  -- Create anti-aliasing toggle
  shift(clone(layout, "multiTextureLabel", "antiAliasingLabel"), 98).value = "SUPER-SAMPLED AA"
  shift(clone(layout, "multiTextureCheckbox", "antiAliasingCheckbox"), 99)
  -- Create object lighting toggle
  shift(clone(layout, "multiTextureLabel", "objectLightingLabel"), 0, -11).value = "NEW OBJECT LIGHTS"
  shift(clone(layout, "multiTextureCheckbox", "objectLightingCheckbox"), 0, -11)
  -- Create hardware cursor toggle
  shift(clone(layout, "multiTextureLabel", "hardwareCursorLabel"), 98, -11).value = "HARDWARE CURSOR"
  shift(clone(layout, "multiTextureCheckbox", "hardwareCursorCheckbox"), 99, -11)
  return config
end