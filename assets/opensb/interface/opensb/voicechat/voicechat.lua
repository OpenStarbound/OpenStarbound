--constants
local PATH = "/interface/opensb/voicechat/"
local DEVICE_LIST_WIDGET = "devices.list"
local DEFAULT_DEVICE_NAME = "Use System Default"
local NULL_DEVICE_NAME = "No Audio Device"
local COLD_COLOR = {25, 255, 255, 225}
local HOT_COLOR = {255, 96, 96, 225}
local MINIMUM_DB = -80
local VOICE_MAX, INPUT_MAX = 1.75, 1.0
local MID = 7.5

local fmt = string.format

local debugging = false
local function log(...)
	if not debugging then return end
	sb.logInfo(...)
end

local function mapToRange(x, min, max)
	return math.min(1, math.max(0, (x - min)) / max)
end

local function linear(a, b, c)
	return a + (b - a) * c
end

local settings = {}

local function set(k, v)
  settings[k] = v
  local newSettings = jobject()
  newSettings[k] = v
  voice.mergeSettings(newSettings)
  return v
end

local devicesToWidgets = {}
local widgetsToDevices = {}
local nullWidget
local function addDeviceToList(deviceName)
	local name = widget.addListItem(DEVICE_LIST_WIDGET)
	widget.setText(fmt("%s.%s.button", DEVICE_LIST_WIDGET, name), deviceName)
	widgetsToDevices[name] = deviceName
	devicesToWidgets[deviceName] = name
	log("Added audio device '%s' to list", deviceName)
	return name
end

function selectDevice()
	local selected = widget.getListSelected(DEVICE_LIST_WIDGET)
	if selected == nullWidget then
		set("inputEnabled", false)
		widget.setListSelected(DEVICE_LIST_WIDGET, nullWidget)
	end
	local deviceName = widgetsToDevices[selected]
	if deviceName == DEFAULT_DEVICE_NAME then deviceName = nil end

	if settings.deviceName == deviceName then
		local inputEnabled = set("inputEnabled", not settings.inputEnabled)
		widget.setListSelected(DEVICE_LIST_WIDGET, inputEnabled and selected or nullWidget)
	else
		set("deviceName", deviceName)
		set("inputEnabled", true)
	end
end

local function initCallbacks()
	widget.registerMemberCallback(DEVICE_LIST_WIDGET, "selectDevice", selectDevice)
end

local function updateVoiceButton()
	local enabled = settings.enabled
	widget.setText("enableVoiceToggle", enabled and "^#0f0;disable voice chat" or "^#f00;enable voice chat")
	widget.setImage("enableVoiceToggleBack", PATH .. "bigbuttonback.png?multiply=" .. (enabled and "0f0" or "f00"))
end

local function updateVoiceModeButtons()
	local pushToTalk = settings.inputMode:lower() == "pushtotalk"
	widget.setImage("pushToTalkBack",    PATH .. "pushtotalkback.png?multiply=" .. (pushToTalk and "0f0" or "f00"))
	widget.setImage("voiceActivityBack", PATH .. "activityback.png?multiply="   .. (pushToTalk and "f00" or "0f0"))
	widget.setText("pushToTalk",    pushToTalk and "^#0f0;PUSH TO TALK" or "^#f00;PUSH TO TALK")
	widget.setText("voiceActivity", pushToTalk and "^#f00;ACTIVITY"     or "^#0f0;ACTIVITY")
end

local voiceCanvas, inputCanvas = nil, nil
local function updateVolumeCanvas(canvas, volume, multiplier)
	canvas:clear()
	local lineEnd = 1 + volume * 195
	local lineColor = {95, 110, 255, 225}
	local multiplied = volume * multiplier
	if multiplied > 1 then
		local level = (multiplied - 1) / (multiplier - 1)
		for i = 1, 4 do
			lineColor[i] = linear(lineColor[i], HOT_COLOR[i], level)
		end
	else
		for i = 1, 4 do
			lineColor[i] = linear(lineColor[i], COLD_COLOR[i], 1 - multiplied)
		end
	end

	canvas:drawLine({1, MID}, {lineEnd, MID}, lineColor, 60)
	canvas:drawLine({lineEnd - 0.5, MID}, {lineEnd + 0.5, MID}, {255, 255, 255, 200}, 60)
	local str = volume == 0 and "^#f00,shadow;MUTED" or fmt("^shadow;%s%%", math.floor(volume * multiplier * 1000) * 0.1)
	canvas:drawText(str, {position = {92.5, MID}, horizontalAnchor = "mid", verticalAnchor = "mid"}, 16, {255, 255, 255, 255})
end

local thresholdCanvas = nil
local function updateThresholdCanvas(canvas, dB)
	canvas:clear()
  local scale = pane.scale()
	local lineEnd = 1 + (1 - (dB / MINIMUM_DB)) * 195
	local lineColor = {255, 255, 0, 127}
	canvas:drawLine({1,  2}, {lineEnd,  2}, lineColor, scale)
	canvas:drawLine({1, 13}, {lineEnd, 13}, lineColor, scale)
	lineColor[4] = 64
	canvas:drawLine({lineEnd,  2}, {196,  2}, lineColor, scale)
	canvas:drawLine({lineEnd, 13}, {196, 13}, lineColor, scale)
	canvas:drawLine({lineEnd - 0.5, MID}, {lineEnd + 0.5, MID}, {255, 255, 255, 200}, 60)

	local loudness = 1 - (voice.speaker().smoothDecibels / MINIMUM_DB)
	local loudnessEnd = math.min(1 + loudness * 195, 196)
	if loudnessEnd > 0 then
		lineColor[4] = 200
		canvas:drawLine({1, MID}, {loudnessEnd, MID}, lineColor, 4 * scale)
	end

	local str = fmt("^shadow;%sdB", math.floor(dB * 10) * 0.1)
	canvas:drawText(str, {position = {92.5, MID}, horizontalAnchor = "mid", verticalAnchor = "mid"}, 16, {255, 255, 255, 255})
end

function init()
	settings = voice.getSettings()
	voiceCanvas = widget.bindCanvas("voiceVolume")
	inputCanvas = widget.bindCanvas("inputVolume")
	thresholdCanvas = widget.bindCanvas("threshold")
end

function displayed()
	devicesToWidgets = {}
	widgetsToDevices = {}
	widget.clearListItems(DEVICE_LIST_WIDGET)
	initCallbacks()

	addDeviceToList(DEFAULT_DEVICE_NAME)

	for i, v in pairs(voice.devices()) do
		addDeviceToList(v)
	end

	nullWidget = widget.addListItem(DEVICE_LIST_WIDGET)
	local nullWidgetPath = fmt("%s.%s", DEVICE_LIST_WIDGET, nullWidget)
	widget.setPosition(nullWidgetPath, {0, 10000})
	widget.setVisible(nullWidgetPath, false)

	local preferredDeviceWidget = devicesToWidgets[settings.deviceName or DEFAULT_DEVICE_NAME]
	if preferredDeviceWidget and settings.inputEnabled then
		widget.setListSelected(DEVICE_LIST_WIDGET, preferredDeviceWidget)
	end

	updateVoiceButton()
	updateVoiceModeButtons()
	updateVolumeCanvas(voiceCanvas, settings.outputVolume / VOICE_MAX, VOICE_MAX)
	updateVolumeCanvas(inputCanvas, settings.inputVolume  / INPUT_MAX, INPUT_MAX)
	updateThresholdCanvas(thresholdCanvas, settings.threshold)
end

function update()
	updateThresholdCanvas(thresholdCanvas, settings.threshold)
end

local function sliderToValue(x)
	return mapToRange(x, 5, 187)
end

local function mouseInSlider(mouse)
	return mouse[1] > 0 and mouse[1] < 197
	   and mouse[2] > 0 and mouse[2] < 16
end

local function handleVolume(canvas, mouse, multiplier, setter)
	if not mouseInSlider(mouse) then return end

	local volumePreMul = sliderToValue(mouse[1])
	local volume = volumePreMul * multiplier;
	if math.abs(volume - 1) < 0.01 then
		volumePreMul = 1 / multiplier
		volume = 1
	end

	updateVolumeCanvas(canvas, volumePreMul, multiplier)
	setter(volume)
end

function voiceVolume(mouse, button)
	if button ~= 0 then return end
	handleVolume(voiceCanvas, mouse, VOICE_MAX, function(v) set("outputVolume", v) end)
end

function inputVolume(mouse, button)
	if button ~= 0 then return end
	handleVolume(inputCanvas, mouse, INPUT_MAX, function(v) set("inputVolume", v) end)
end

function threshold(mouse, button)
	if button ~= 0 then return end
	if not mouseInSlider(mouse) then return end
	local dB = (1 - sliderToValue(mouse[1])) * MINIMUM_DB
	set("threshold", dB)

	updateThresholdCanvas(thresholdCanvas, dB)
end

function switchVoiceMode(mode)
	log("switching voice mode to %s", tostring(mode))
	local success, err = pcall(function()
		set("inputMode", mode)
		updateVoiceModeButtons()
	end)
	if not success then log("%s", err) end
end

function voiceToggle()
  set("enabled", not settings.enabled)
	updateVoiceButton()
end