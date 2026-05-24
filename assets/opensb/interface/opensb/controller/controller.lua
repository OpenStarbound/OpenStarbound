-- Controller Settings Pane

local modeDescriptions = {
  auto = "Switches between gamepad and mouse based on last input device used.",
  gamepad = "Right stick aims. Virtual cursor (L3) for menus. Mouse disabled.",
  hybrid = "Right stick aims. Mouse/trackpad active for UI. Best for Steam Deck."
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
  -- Update button appearance via caption markup
  local modes = {
    { widget = "modeAuto",    id = "auto",    label = "Auto" },
    { widget = "modeGamepad", id = "gamepad", label = "Gamepad" },
    { widget = "modeHybrid",  id = "hybrid",  label = "Hybrid" }
  }
  for _, m in ipairs(modes) do
    if m.id == mode then
      widget.setButtonCaption(m.widget, "^#ebd74a;" .. m.label)
    else
      widget.setButtonCaption(m.widget, m.label)
    end
  end
  widget.setText("modeDescription", modeDescriptions[mode] or "")
end

local function updateSliders()
  -- Deadzone: range [1,20] maps to [0.05, 1.00] (step 0.05)
  local deadzone = getConfig("controllerAimDeadzone", 0.15)
  local deadzoneStep = math.max(1, math.min(20, math.floor(deadzone / 0.05 + 0.5)))
  widget.setSliderValue("deadzoneSlider", deadzoneStep)
  widget.setText("deadzoneValue", string.format("%.2f", deadzoneStep * 0.05))

  -- Aim radius: range [2,16] direct
  local aimRadius = getConfig("controllerAimRadius", 8.0)
  local aimStep = math.max(2, math.min(16, math.floor(aimRadius + 0.5)))
  widget.setSliderValue("aimRadiusSlider", aimStep)
  widget.setText("aimRadiusValue", tostring(aimStep))

  -- Cursor speed: range [1,20] maps to [100, 2000] (step 100)
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
