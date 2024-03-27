-- Manages the voice HUD indicators and click-to-mute/unmute.

local fmt = string.format
local sqrt, min, max = math.sqrt, math.min, math.max

local module = {}
modules.voice_manager = module

--constants
local INDICATOR_PATH = "/interface/opensb/voicechat/indicator/"
local BACK_INDICATOR_IMAGE = INDICATOR_PATH .. "back.png"
local FRONT_INDICATOR_IMAGE = INDICATOR_PATH .. "front.png"
local FRONT_MUTED_INDICATOR_IMAGE = INDICATOR_PATH .. "front_muted.png"
local INDICATOR_SIZE = {300, 48}
local LINE_PADDING = 12
local LINE_WIDTH = 296
local LINE_WIDTH_PADDED = LINE_WIDTH - LINE_PADDING
local LINE_COLOR = {50, 210, 255, 255}
local FONT_DIRECTIVES = "?border=1;333;3337?border=1;333;3330"
local NAME_PREFIX = "^noshadow,white,set;"

local function dbToLoudness(db) return math.exp(db * 0.05) end

local canvas

local linePaddingDrawable
do
  local drawable = { image = BACK_INDICATOR_IMAGE, position = {0, 0}, color = LINE_COLOR, centered = false }
	function linePaddingDrawable(a, b)
		drawable.image = BACK_INDICATOR_IMAGE .. fmt("?crop=%i;%i;%i;%i?fade=fff;1", a, 0, b, INDICATOR_SIZE[2])
		drawable.position[1] = a
		return drawable;
	end
end

local lineDrawable = { line = {{LINE_PADDING, 24}, {10, 24}}, width = 48, color = LINE_COLOR }
local function line(pos, value)
	local width = math.floor((LINE_WIDTH * value) + 0.5)
	LINE_COLOR[4] = 255 * math.min(1, sqrt(width / 350))
	if width > 0 then
		canvas:drawDrawable(linePaddingDrawable(0, math.min(12, width)), pos)
		if width > 12 then
			lineDrawable.line[2][1] = math.min(width, LINE_WIDTH_PADDED)
			canvas:drawDrawable(lineDrawable, pos)
			if width > LINE_WIDTH_PADDED then
				canvas:drawDrawable(linePaddingDrawable(LINE_WIDTH_PADDED, width), pos)
			end
		end
	end
end

local drawable = { image = BACK_INDICATOR_IMAGE, centered = false }
local textPositioning = { position = {0, 0}, horizontalAnchor = "left", verticalAnchor = "mid" }

local hoveredSpeaker = nil
local hoveredSpeakerIndex = 1
local hoveredSpeakerPosition = {0, 0}
local function mouseOverSpeaker(mouse, pos, expand)
	expand = tonumber(expand) or 0
	return (mouse[1] > pos[1] - expand and mouse[1] < pos[1] + 300 + expand)
	   and (mouse[2] > pos[2] - expand and mouse[2] < pos[2] +  48 + expand)
end

local function drawSpeakerBar(mouse, pos, speaker, i)
	drawable.image = BACK_INDICATOR_IMAGE
	canvas:drawDrawable(drawable, pos)
	line(pos, dbToLoudness(speaker.smoothDecibels))
	local hovering = not speaker.isLocal and mouseOverSpeaker(mouse, pos)

	textPositioning.position = {pos[1] + 49, pos[2] + 24}
	textPositioning.horizontalAnchor = "left"
	local text = NAME_PREFIX ..
	(hovering and (speaker.muted and "^#31d2f7;Unmute^reset; " or "^#f43030;Mute^reset; ") or "")
	.. speaker.name
	canvas:drawText(text, textPositioning, 16, nil, nil, nil, FONT_DIRECTIVES)
	drawable.image = speaker.muted and FRONT_MUTED_INDICATOR_IMAGE or FRONT_INDICATOR_IMAGE
	canvas:drawDrawable(drawable, pos)

	if hovering	then
		hoveredSpeaker = speaker
		hoveredSpeakerIndex = i
		hoveredSpeakerPosition = pos
		if input.keyHeld("LShift") then
			textPositioning.position = {pos[1] + 288, pos[2] + 24}
			textPositioning.horizontalAnchor = "right"
			canvas:drawText("^#fff7,font=iosevka-semibold;" .. tostring(speaker.speakerId), textPositioning, 16, nil, nil, nil, FONT_DIRECTIVES)
		end

		if input.mouseDown("MouseLeft") then
			local muted = not voice.speakerMuted(speaker.speakerId)
			interface.queueMessage((muted and "^#f43030;Muted^reset; " or "^#31d2f7;Unmuted^reset; ") .. speaker.name, 4, 0.5)
			voice.setSpeakerMuted(speaker.speakerId, muted)
			speaker.muted = muted
		end
	end
end

local function simulateSpeakers()
	local speakers = {}
	for i = 2, 5 + math.floor((math.sin(os.clock()) * 4) + .5) do
		local dB = -48 + 48 * math.sin(os.clock() + (i * 0.5))
		speakers[i] = {
			speakerId = i,
			entityId = -65536 * i,
			name = "Player " .. i .. " (" .. (math.floor((dB * 100) + 0.5) / 100) .. " dB)",
			decibels = dB,
			smoothDecibels = dB,
			muted = false
		}
	end
	return speakers
end

local function drawIndicators()
	canvas:clear()
	local screenSize = canvas:size()
	local mousePosition = canvas:mousePosition()
	local basePos = {screenSize[1] - 350, 50}

	local speakersRemaining, speakers = {}, {}
	local hoveredSpeakerId = nil
	if hoveredSpeaker then
		if not mouseOverSpeaker(mousePosition, hoveredSpeakerPosition, 16) then
			hoveredSpeaker = nil
		else
			hoveredSpeakerId = hoveredSpeaker.speakerId
		end
	end

	local speakerCount = 0
	for i, speaker in pairs(voice.speakers()) do
		local speakerId = speaker.speakerId
		speakersRemaining[speakerId] = true
		if speakerId == hoveredSpeakerId then
			hoveredSpeaker = speaker
		else
			speakerCount = speakerCount + 1
			speakers[speakerCount] = speaker
		end
	end

	if hoveredSpeaker then
		local len = #speakers
		if hoveredSpeakerIndex > len then
			for i = len + 1, hoveredSpeakerIndex - 1 do
				speakers[i] = false
			end
			speakers[hoveredSpeakerIndex] = hoveredSpeaker
		else
			table.insert(speakers, hoveredSpeakerIndex, hoveredSpeaker)
		end
	end

	for i, v in pairs(speakers) do
		if v then
			local pos = {basePos[1], basePos[2] + (i - 1) * 52}
			drawSpeakerBar(mousePosition, pos, v, i)
		end
	end
end

function module.init()
	canvas = interface.bindCanvas("voice", true)
end

function module.update()
	drawIndicators()
end