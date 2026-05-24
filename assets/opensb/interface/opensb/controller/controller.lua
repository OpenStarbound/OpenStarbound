-- Controller Settings Pane

local modeDescriptions = {
  auto = "Auto-switches between gamepad and mouse based on last input.",
  gamepad = "Right stick aims. L3 toggles virtual cursor for menus.",
  hybrid = "Right stick aims. Mouse/trackpad handles UI directly."
}

local function getConfig(key, default)
  local ok, val = pcall(root.getConfiguration, key)
  if ok and val ~= nil then return val end
  return default
end

local function setConfig(key, value)
  root.setConfiguration(key, value)
end

local function updateModeButtons()
  local mode = getConfig("controllerMode", "auto")
  widget.setChecked("modeAuto", mode == "auto")
  widget.setChecked("modeGamepad", mode == "gamepad")
  widget.setChecked("modeHybrid", mode == "hybrid")
  widget.setText("modeDescLabel", modeDescriptions[mode] or "")
end

local function updateSliders()
  local deadzone = getConfig("controllerAimDeadzone", 0.15)
  local deadzoneStep = math.max(1, math.min(20, math.floor(deadzone / 0.05 + 0.5)))
  widget.setSliderValue("deadzoneSlider", deadzoneStep)
  widget.setText("deadzoneValue", string.format("%.2f", deadzoneStep * 0.05))

  local aimRadius = getConfig("controllerAimRadius", 8.0)
  local aimStep = math.max(2, math.min(16, math.floor(aimRadius + 0.5)))
  widget.setSliderValue("aimRadiusSlider", aimStep)
  widget.setText("aimRadiusValue", tostring(aimStep))

  local cursorSpeed = getConfig("controllerVirtualCursorSpeed", 800.0)
  local speedStep = math.max(1, math.min(20, math.floor(cursorSpeed / 100 + 0.5)))
  widget.setSliderValue("cursorSpeedSlider", speedStep)
  widget.setText("cursorSpeedValue", tostring(speedStep * 100))
end

function init()
  updateModeButtons()
  updateSliders()
end

function selectModeAuto()
  setConfig("controllerMode", "auto")
  updateModeButtons()
end

function selectModeGamepad()
  setConfig("controllerMode", "gamepad")
  updateModeButtons()
end

function selectModeHybrid()
  setConfig("controllerMode", "hybrid")
  updateModeButtons()
end

function deadzoneChanged()
  local val = widget.getSliderValue("deadzoneSlider")
  local deadzone = val * 0.05
  setConfig("controllerAimDeadzone", deadzone)
  widget.setText("deadzoneValue", string.format("%.2f", deadzone))
end

function aimRadiusChanged()
  local val = widget.getSliderValue("aimRadiusSlider")
  setConfig("controllerAimRadius", val)
  widget.setText("aimRadiusValue", tostring(val))
end

function cursorSpeedChanged()
  local val = widget.getSliderValue("cursorSpeedSlider")
  local speed = val * 100
  setConfig("controllerVirtualCursorSpeed", speed)
  widget.setText("cursorSpeedValue", tostring(speed))
end
