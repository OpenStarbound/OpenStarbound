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

local function moveto(thing, otherthing)
  thing.position[1] = otherthing.position[1]
  thing.position[2] = otherthing.position[2]
  return thing
end

-- patch function, called by the game
function patch(config)
  local layout = config.paneLayout
  layout.panefeature.positionLocked = false
  layout.panefeature.anchor = "center"
  if layout.bgShine then
    layout.bgShine.zlevel = -10
  end
  
  -- upshift from bottom, so we can add new checkbox rows
  local upshift = 11
  
  shift(layout.zoomLabel, 0, upshift)
  shift(layout.zoomSlider, 0, upshift)
  shift(layout.zoomValueLabel, 0, upshift)
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
  config.interfaceScaleList = {0} -- 0 = AUTO!
  for i = 1, 17 do config.interfaceScaleList[i + 1] = 0.75 + i / 4 end
  
  -- shift up all checkboxes
  for _,k in next, {"fullscreen","borderless","monochrome","interactiveHighlight","speechBubble","textureLimit","multiTexture"} do
    shift(layout[k.."Label"],0,upshift)
    shift(layout[k.."Checkbox"],0,upshift)
  end

  -- Create anti-aliasing toggle
  shift(clone(layout, "multiTextureLabel", "antiAliasingLabel"), 98).value = "SUPER-SAMPLED AA"
  shift(clone(layout, "multiTextureCheckbox", "antiAliasingCheckbox"), 99)
  -- Create new lighting toggle
  shift(clone(layout, "multiTextureLabel", "newLightingLabel"), 0, -11).value = "NEW LIGHTING"
  shift(clone(layout, "multiTextureCheckbox", "newLightingCheckbox"), 0, -11)
  -- Create hardware cursor toggle
  shift(clone(layout, "multiTextureLabel", "hardwareCursorLabel"), 98, -11).value = "HARDWARE CURSOR"
  shift(clone(layout, "multiTextureCheckbox", "hardwareCursorCheckbox"), 99, -11)
  -- Create HDR toggle
  shift(clone(layout, "multiTextureLabel", "hdrLabel"), 0, -22).value = "HDR"
  shift(clone(layout, "multiTextureCheckbox", "hdrCheckbox"), 0, -22)
  -- Create vsync toggle
  shift(clone(layout, "multiTextureLabel", "vsyncLabel"), 98, -22).value = "VSYNC"
  shift(clone(layout, "multiTextureCheckbox", "vsyncCheckbox"), 99, -22)
  
  -- Create shader menu button
  shift(moveto(clone(layout, "accept", "showShadersMenu"), layout.interfaceScaleSlider), 112, -2).caption = "Shaders"
  

  shift(layout.title, 0, 24+upshift)
  shift(layout.resLabel, 0, 28+upshift)
  shift(layout.resSlider, 0, 28+upshift)
  shift(layout.resValueLabel, 0, 28+upshift)
  return config
end
