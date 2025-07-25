-- identity is the NPC or player's identity
-- humanoidConfig is the merged data of the defined humanoidConfig for that species merged with its humanoidOverrides
-- npcHumanoidConfig is only passed when an NPC has its own humanoidConfig defined in its npcType
-- the returned value is the humanoidConfig that will be effectively used for the humanoidEntity
-- this means you can have things like movement parameters, death particles, or the numerous offsets change based on identity values
-- and if you're using animation configs for the species, you can change things a whole lot more

local emoteStates = { "idle", "blabber", "shout", "happy", "sad", "neutral", "laugh", "annoyed", "oh", "oooh", "blink",
	"wink", "eat", "sleep" }
local bodyStates = { "idle", "walk", "run", "jump", "fall", "swim", "swimIdle", "duck", "sit", "lay" }
local portraitModes = { "head", "bust", "full", "fullNeutral", "fullNude", "fullNeutralNude" }
local tilePixels = 8
-- this build script is meant to interpret retail's humanoidConfig and apply all the relevant values to a base animation that should then
-- be basically identical to how the retail humanoids look, now with the ability to add extra parts into the animation
function build(identity, humanoidParameters, humanoidConfig, npcHumanoidConfig)
	humanoidConfig = sb.jsonMerge(npcHumanoidConfig or humanoidConfig, humanoidParameters)
	if not humanoidConfig.useAnimation then return humanoidConfig end

	if not humanoidConfig.animation then
		humanoidConfig.animation = {
			version = 1,
			includes = {
				"/humanoid/opensb/humanoid.animation"
			},
			globalTagDefaults = {},
			animatedParts = {
				stateTypes = {},
				parts = {}
			},
			transformationGroups = {}
		}
	elseif type(humanoidConfig.animation) == "string" then
		local animation = humanoidConfig.animation
		humanoidConfig.animation = {
			version = 1,
			includes = {
				animation
			},
			globalTagDefaults = {},
			animatedParts = {
				stateTypes = {},
				parts = {}
			},
			transformationGroups = {}
		}
	end
	for k, v in pairs(identity) do
		if type(v) == "string" then
			humanoidConfig.animation.globalTagDefaults[k] = v
		end
	end
	local stateTypes = humanoidConfig.animation.animatedParts.stateTypes
	local parts = humanoidConfig.animation.animatedParts.parts

	for i, emoteState in ipairs(emoteStates) do
		setPath(stateTypes, { "emote", "states", emoteState, "cycle" }, humanoidConfig.humanoidTiming.emoteCycle[i])
		stateTypes.emote.states[emoteState].frames = humanoidConfig.humanoidTiming.emoteFrames[i]
		setPath(humanoidConfig, { "emoteAnimations", emoteState, "emote" }, { emoteState, false, false })
	end
	for i, bodyState in ipairs(bodyStates) do
		setPath(stateTypes, { "body", "states", bodyState, "cycle" }, humanoidConfig.humanoidTiming.stateCycle[i])
		stateTypes.body.states[bodyState].frames = humanoidConfig.humanoidTiming.stateFrames[i]
		stateTypes.body.states[bodyState].properties = {}
		setPath(humanoidConfig, { "stateAnimations", bodyState, "body" }, { bodyState, false, false })
	end
	for i, portraitMode in ipairs(portraitModes) do
		setPath(humanoidConfig, { "portraitAnimations", portraitMode, "body" }, { "idle", true, false })
	end
	humanoidConfig.portraitAnimations.bust.portraitMode = { "bust", true, false }
	humanoidConfig.portraitAnimations.head.portraitMode = { "head", true, false }

	for i, v in ipairs(humanoidConfig.armWalkSeq) do
		setPath(stateTypes.body.states.walk, { "frameProperties", "animationTags", i, "armSequenceFrame" }, tostring(v))
	end
	for i, v in ipairs(humanoidConfig.armRunSeq) do
		setPath(stateTypes.body.states.run, { "frameProperties", "animationTags", i, "armSequenceFrame" }, tostring(v))
	end

	for i, v in ipairs(humanoidConfig.walkBob) do
		setPath(stateTypes.body.states.walk, { "frameProperties", "movementOffset", i },
			{ { "reset" }, { "translate", { 0, v / tilePixels } } })
	end
	for i, v in ipairs(humanoidConfig.runBob) do
		setPath(stateTypes.body.states.run, { "frameProperties", "movementOffset", i },
			{ { "reset" }, { "translate", { 0, (v + humanoidConfig.runFallOffset) / tilePixels } } })
	end
	for i, v in ipairs(humanoidConfig.swimBob) do
		setPath(stateTypes.body.states.swim, { "frameProperties", "movementOffset", i },
			{ { "reset" }, { "translate", { 0, v / tilePixels } } })
	end

	stateTypes.body.states.run.properties.headOffset = {
		{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headRunOffset) },
	}
	stateTypes.body.states.swim.properties.headOffset = {
		{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headSwimOffset) },
	}
	stateTypes.body.states.swimIdle.properties.headOffset = {
		{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headSwimOffset) },
	}
	stateTypes.body.states.duck.properties.headOffset = {
		{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headDuckOffset) },
	}
	stateTypes.body.states.duck.properties.movementOffset = {
		{ "reset" }, { "translate", { 0, humanoidConfig.duckOffset / tilePixels } },
	}
	stateTypes.body.states.sit.properties.headOffset = {
		{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headSitOffset) },
	}
	stateTypes.body.states.sit.properties.movementOffset = {
		{ "reset" }, { "translate", { 0, humanoidConfig.sitOffset / tilePixels } },
	}
	stateTypes.body.states.lay.properties.headOffset = {
		{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headLayOffset) },
	}
	stateTypes.body.states.lay.properties.movementOffset = {
		{ "reset" }, { "translate", { 0, humanoidConfig.layOffset / tilePixels } },
	}
	stateTypes.body.states.jump.properties.movementOffset = {
		{ "reset" }, { "translate", { 0, humanoidConfig.jumpBob / tilePixels } },
	}
	stateTypes.body.states.fall.properties.movementOffset = {
		{ "reset" }, { "translate", { 0, humanoidConfig.runFallOffset / tilePixels } }
	}
	if humanoidConfig.bodyFullbright then
		setPath(parts, { "body", "properties", "fullbright" }, true)
		setPath(parts, { "frontArm", "properties", "fullbright" }, true)
		setPath(parts, { "backArm", "properties", "fullbright" }, true)
		setPath(parts, { "facialHair", "properties", "fullbright" }, true)
		setPath(parts, { "facialMask", "properties", "fullbright" }, true)
		setPath(parts, { "hair", "properties", "fullbright" }, true)
		setPath(parts, { "head", "properties", "fullbright" }, true)
		setPath(parts, { "emote", "properties", "fullbright" }, true)
	end
	setPath(parts, { "anchor", "properties", "offset" }, vecTilePixels(humanoidConfig.globalOffset))
	setPath(parts, { "head", "properties", "mouthOffset" }, vecTilePixels(humanoidConfig.mouthOffset))
	setPath(parts, { "body", "properties", "feetOffset" }, vecTilePixels(humanoidConfig.feetOffset))

	setPath(parts, { "frontArm", "properties", "rotationCenter" }, vecTilePixels(humanoidConfig.frontArmRotationCenter))
	setPath(parts, { "backArm", "properties", "rotationCenter" }, vecTilePixels({
		humanoidConfig.backArmRotationCenter[1] + humanoidConfig.backArmOffset[1],
		humanoidConfig.backArmRotationCenter[2] + humanoidConfig.backArmOffset[2],
	}))
	setPath(parts, { "frontHandItem", "properties", "offset" }, vecTilePixels(humanoidConfig.frontHandPosition))
	setPath(parts, { "backHandItem", "properties", "offset" }, vecTilePixels({
		humanoidConfig.frontHandPosition[1] + humanoidConfig.backArmOffset[1],
		humanoidConfig.frontHandPosition[2] + humanoidConfig.backArmOffset[2],
	}))
	setPath(parts, { "head", "properties", "rotationCenter" },
		vecTilePixels(humanoidConfig.headRotationCenter or { 0, 0 }))

	-- these states play in reverse for moving backwards, run has a duplicate state for this because back items have a different frameset for it
	stateTypes.body.states.runbackwards = stateTypes.body.states.run
	setPath(humanoidConfig, { "stateAnimationsBackwards", "walk", "body" }, { "walk", false, true })
	setPath(humanoidConfig, { "stateAnimationsBackwards", "run", "body" }, { "runbackwards", false, true })


	-- there are 20 cosmetic slots and we need to support any cosmetic being in any slot, and these should all be the same aside from zlevel, so just generate that
	for i = 1, 20 do
		local cosmeticParts = sb.parseJson(sb.printJson(root.assetJson(humanoidConfig.cosmeticParts or
			"/humanoid/opensb/humanoidCosmetics.config")):gsub("%<slot%>", tostring(i)))
		for partName, part in pairs(cosmeticParts) do
			fixSlotProperties(part.properties, i)
			fixSlotFrameProperties(part.frameProperties, i)
			for stateTypeName, stateType in pairs(part.partStates or {}) do
				for stateName, state in pairs(stateType) do
					if type(state) == "table" then
						fixSlotProperties(state.properties, i)
						fixSlotFrameProperties(state.frameProperties, i)
					end
				end
			end
			parts[partName] = part
		end
		humanoidConfig.animation.globalTagDefaults["headCosmetic" .. tostring(i) .. "Frameset"] = ""
		humanoidConfig.animation.globalTagDefaults["headCosmetic" .. tostring(i) .. "Directives"] = ""
		humanoidConfig.animation.globalTagDefaults["chestCosmetic" .. tostring(i) .. "Frameset"] = ""
		humanoidConfig.animation.globalTagDefaults["chestCosmetic" .. tostring(i) .. "Directives"] = ""
		humanoidConfig.animation.globalTagDefaults["legsCosmetic" .. tostring(i) .. "Frameset"] = ""
		humanoidConfig.animation.globalTagDefaults["legsCosmetic" .. tostring(i) .. "Directives"] = ""
		humanoidConfig.animation.globalTagDefaults["backCosmetic" .. tostring(i) .. "Frameset"] = ""
		humanoidConfig.animation.globalTagDefaults["backCosmetic" .. tostring(i) .. "Directives"] = ""

		humanoidConfig.animation.globalTagDefaults["frontSleeve" .. tostring(i) .. "Frameset"] = ""
		humanoidConfig.animation.globalTagDefaults["backSleeve" .. tostring(i) .. "Frameset"] = ""

		humanoidConfig.animation.transformationGroups["backCosmetic" .. tostring(i) .. "Rotation"] = { interpolated = true }
	end

	-- remove parts that aren't used so the game isn't fussing about incomplete image paths in the log
	if identity.facialHairGroup == "" then
		parts.facialHair = false
	end
	if identity.facialMaskGroup == "" then
		parts.facialMask = false
	end
	if identity.hairGroup == "" then
		parts.hair = false
	end

	return humanoidConfig
end

function setPath(input, path, value)
	local i = input
	for j, v in ipairs(path) do
		if type(i[v]) == "nil" then
			if j == #path then
				i[v] = value
				return true
			else
				i[v] = {}
			end
		elseif type(i[v]) ~= "table" then
			return false
		end
		i = i[v]
	end
end

function vecTilePixels(vec)
	return { vec[1] / tilePixels, vec[2] / tilePixels }
end

function fixSlotProperties(properties, slot)
	if not properties then return end
	if properties.zLevel then
		properties.zLevel = properties.zLevel + slot / 100
	end
	if properties.flippedZLevel then
		properties.flippedZLevel = properties.flippedZLevel + slot / 100
	end
	if properties.zLevelSlot then
		properties.zLevel = properties.zLevelSlot[slot]
	end
	if properties.flippedZLevelSlot then
		properties.flippedZLevel = properties.flippedZLevelSlot[slot]
	end
end

function fixSlotFrameProperties(frameProperties, slot)
	if not frameProperties then return end
	if frameProperties.zLevel then
		for i, v in ipairs(frameProperties.zLevel) do
			frameProperties.zLevel[i] = v + slot / 100
		end
	end
	if frameProperties.flippedZLevel then
		for i, v in ipairs(frameProperties.flippedZLevel) do
			frameProperties.flippedZLevel[i] = v + slot / 100
		end
	end
	if frameProperties.zLevelSlot then
		for i, v in ipairs(frameProperties.zLevelSlot) do
			frameProperties.zLevel[i] = v[slot]
		end
	end
	if frameProperties.flippedZLevelSlot then
		for i, v in ipairs(frameProperties.flippedZLevelSlot) do
			frameProperties.flippedZLevel[i] = v[slot]
		end
	end
end
