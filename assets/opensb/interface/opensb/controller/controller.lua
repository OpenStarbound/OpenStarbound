-- Controller Settings Pane

local modeDescriptions = {
  off = "Controller input completely disabled. Mouse and keyboard only.",
  auto = "Auto-switches between gamepad and mouse based on last input.",
  gamepad = "Right stick aims. L3 toggles virtual cursor for menus.",
  hybrid = "Right stick aims. Mouse/trackpad handles UI directly."
}

local modeButtons = {
  { widget = "modeOff",     id = "off" },
  { widget = "modeAuto",    id = "auto" },
  { widget = "modeGamepad", id = "gamepad" },
  { widget = "modeHybrid",  id = "hybrid" }
}

local function updateModeButtons()
  local mode = root.getConfiguration("controllerMode") or "auto"
  for _, m in ipairs(modeButtons) do
    widget.setChecked(m.widget, m.id == mode)
  end
  widget.setText("modeDescLabel", modeDescriptions[mode] or "")
end

local function updateSliders()
  local deadzone = root.getConfiguration("controllerAimDeadzone") or 0.15
  local deadzoneStep = math.max(1, math.min(20, math.floor(deadzone / 0.05 + 0.5)))
  widget.setSliderValue("deadzoneSlider", deadzoneStep)
  widget.setText("deadzoneValue", string.format("%.2f", deadzoneStep * 0.05))

  local aimRadius = root.getConfiguration("controllerAimRadius") or 8.0
  local aimStep = math.max(2, math.min(16, math.floor(aimRadius + 0.5)))
  widget.setSliderValue("aimRadiusSlider", aimStep)
  widget.setText("aimRadiusValue", tostring(aimStep))

  local cursorSpeed = root.getConfiguration("controllerVirtualCursorSpeed") or 800.0
  local speedStep = math.max(1, math.min(20, math.floor(cursorSpeed / 100 + 0.5)))
  widget.setSliderValue("cursorSpeedSlider", speedStep)
  widget.setText("cursorSpeedValue", tostring(speedStep * 100))

  local vertThreshold = root.getConfiguration("controllerVerticalThreshold") or 0.5
  local vertStep = math.max(1, math.min(19, math.floor(vertThreshold / 0.05 + 0.5)))
  widget.setSliderValue("verticalThresholdSlider", vertStep)
  widget.setText("verticalThresholdValue", string.format("%.2f", vertStep * 0.05))

  local rumbleIntensity = root.getConfiguration("controllerRumbleIntensity") or 1.0
  local rumbleStep = math.max(0, math.min(20, math.floor(rumbleIntensity * 20 + 0.5)))
  widget.setSliderValue("rumbleIntensitySlider", rumbleStep)
  widget.setText("rumbleIntensityValue", string.format("%.2f", rumbleStep / 20))
end

function init()
  updateModeButtons()
  updateSliders()
end

local function selectMode(mode)
  root.setConfiguration("controllerMode", mode)
  -- Uncheck siblings; the clicked button's auto-toggle will check it
  for _, m in ipairs(modeButtons) do
    if m.id ~= mode then
      widget.setChecked(m.widget, false)
    end
  end
  widget.setText("modeDescLabel", modeDescriptions[mode] or "")
end

function selectModeOff() selectMode("off") end
function selectModeAuto() selectMode("auto") end
function selectModeGamepad() selectMode("gamepad") end
function selectModeHybrid() selectMode("hybrid") end

function deadzoneChanged()
  local val = widget.getSliderValue("deadzoneSlider")
  local deadzone = val * 0.05
  root.setConfiguration("controllerAimDeadzone", deadzone)
  widget.setText("deadzoneValue", string.format("%.2f", deadzone))
end

function aimRadiusChanged()
  local val = widget.getSliderValue("aimRadiusSlider")
  root.setConfiguration("controllerAimRadius", val)
  widget.setText("aimRadiusValue", tostring(val))
end

function cursorSpeedChanged()
  local val = widget.getSliderValue("cursorSpeedSlider")
  local speed = val * 100
  root.setConfiguration("controllerVirtualCursorSpeed", speed)
  widget.setText("cursorSpeedValue", tostring(speed))
end

function verticalThresholdChanged()
  local val = widget.getSliderValue("verticalThresholdSlider")
  local threshold = val * 0.05
  root.setConfiguration("controllerVerticalThreshold", threshold)
  widget.setText("verticalThresholdValue", string.format("%.2f", threshold))
end

function rumbleIntensityChanged()
  local val = widget.getSliderValue("rumbleIntensitySlider")
  local intensity = val / 20
  root.setConfiguration("controllerRumbleIntensity", intensity)
  widget.setText("rumbleIntensityValue", string.format("%.2f", intensity))
end
