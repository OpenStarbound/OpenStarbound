-- Controller Settings Pane
-- Reads/writes controller configuration via root.getConfiguration/setConfiguration

local SLIDER_BG = {40, 40, 40, 255}
local SLIDER_FG = {80, 200, 255, 255}
local SLIDER_KNOB = {255, 255, 255, 255}

local MODE_DESCRIPTIONS = {
  auto = "Switches between mouse and gamepad based on last input",
  gamepad = "Right stick aims, virtual cursor for menus",
  hybrid = "Right stick aims, mouse still active for UI"
}

local currentMode = "auto"

-- Slider definitions: {configKey, min, max, format, widgetName, labelWidget}
local sliders = {
  deadzone = {key = "controllerAimDeadzone", min = 0.05, max = 0.50, fmt = "%.2f", canvas = "deadzoneSlider", label = "deadzoneValue"},
  aimRadius = {key = "controllerAimRadius", min = 2.0, max = 16.0, fmt = "%.1f", canvas = "aimRadiusSlider", label = "aimRadiusValue"},
  cursorSpeed = {key = "controllerVirtualCursorSpeed", min = 200, max = 2000, fmt = "%d", canvas = "cursorSpeedSlider", label = "cursorSpeedValue"}
}

local function clampVal(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

local function drawSlider(canvasName, fraction)
  local canvas = widget.bindCanvas(canvasName)
  local size = canvas:size()
  canvas:clear()

  -- Background
  canvas:drawRect({0, 0, size[1], size[2]}, SLIDER_BG)

  -- Filled portion
  local fillWidth = math.floor(fraction * size[1])
  canvas:drawRect({0, 0, fillWidth, size[2]}, SLIDER_FG)

  -- Knob
  local knobX = math.floor(fraction * (size[1] - 4)) + 2
  canvas:drawRect({knobX - 2, 0, knobX + 2, size[2]}, SLIDER_KNOB)
end

local function getSliderFraction(s)
  local val = root.getConfiguration(s.key) or s.min
  return clampVal((val - s.min) / (s.max - s.min), 0, 1)
end

local function setSliderFromClick(s, position)
  local canvas = widget.bindCanvas(s.canvas)
  local size = canvas:size()
  local fraction = clampVal(position[1] / size[1], 0, 1)
  local val = s.min + fraction * (s.max - s.min)

  -- Snap to nice values
  if s.max <= 1.0 then
    val = math.floor(val * 100 + 0.5) / 100
  elseif s.max <= 20 then
    val = math.floor(val * 10 + 0.5) / 10
  else
    val = math.floor(val + 0.5)
  end

  root.setConfiguration(s.key, val)
end

local function updateModeButtons()
  widget.setText("modeDescription", MODE_DESCRIPTIONS[currentMode] or "")
end

local function loadSettings()
  currentMode = root.getConfiguration("controllerMode") or "auto"
  updateModeButtons()
end

function init()
  loadSettings()
end

function update(dt)
  -- Redraw sliders each frame (simple approach)
  for _, s in pairs(sliders) do
    local frac = getSliderFraction(s)
    drawSlider(s.canvas, frac)
    local val = root.getConfiguration(s.key) or s.min
    widget.setText(s.label, string.format(s.fmt, val))
  end
end

-- Mode selection callbacks
function selectModeAuto()
  currentMode = "auto"
  root.setConfiguration("controllerMode", "auto")
  updateModeButtons()
end

function selectModeGamepad()
  currentMode = "gamepad"
  root.setConfiguration("controllerMode", "gamepad")
  updateModeButtons()
end

function selectModeHybrid()
  currentMode = "hybrid"
  root.setConfiguration("controllerMode", "hybrid")
  updateModeButtons()
end

-- Slider click callbacks
function deadzoneSlider(position, button, isDown)
  if isDown then
    setSliderFromClick(sliders.deadzone, position)
  end
end

function aimRadiusSlider(position, button, isDown)
  if isDown then
    setSliderFromClick(sliders.aimRadius, position)
  end
end

function cursorSpeedSlider(position, button, isDown)
  if isDown then
    setSliderFromClick(sliders.cursorSpeed, position)
  end
end
