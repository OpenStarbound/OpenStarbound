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
	if humanoidConfig.useBodyMask then
		table.insert(humanoidConfig.animation.includes, "/humanoid/opensb/bodyMask.animation")
	end
	if humanoidConfig.useBodyHeadMask then
		table.insert(humanoidConfig.animation.includes, "/humanoid/opensb/bodyHeadMask.animation")
	end
	-- there are 20 cosmetic slots and we need to support any cosmetic being in any slot
	-- in postload we generated 20 duplicates of each cosmetic's animation but with the <slot> tag changed and z levels changed to respect the slot
	for _, path in ipairs(humanoidConfig.cosmeticAnimations or root.assetJson("/humanoid.config:cosmeticAnimations")) do
		for i = 1, 20 do
			table.insert(humanoidConfig.animation.includes, path .. "." .. i)
		end
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
		setPath(stateTypes, { "bodyHighPriority", "states", bodyState, "cycle" }, humanoidConfig.humanoidTiming.stateCycle[i])
		stateTypes.body.states[bodyState].frames = humanoidConfig.humanoidTiming.stateFrames[i]
		stateTypes.body.states[bodyState].properties = {}
		stateTypes.bodyHighPriority.states[bodyState].frames = humanoidConfig.humanoidTiming.stateFrames[i]
		stateTypes.bodyHighPriority.states[bodyState].properties = {}
		setPath(humanoidConfig, { "stateAnimations", bodyState, "body" }, { bodyState, false, false })
		setPath(humanoidConfig, { "stateAnimations", bodyState, "bodyHighPriority" }, { bodyState, false, false })
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

	if not humanoidConfig.movementBobOffsets then
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
	end
	if not humanoidConfig.animationHeadOffsets then
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
		stateTypes.body.states.sit.properties.headOffset = {
			{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headSitOffset) },
		}
		stateTypes.body.states.lay.properties.headOffset = {
			{ "reset" }, { "translate", vecTilePixels(humanoidConfig.headLayOffset) },
		}
	end
	if not humanoidConfig.animationMovementOffsets then
		stateTypes.body.states.duck.properties.movementOffset = {
			{ "reset" }, { "translate", { 0, humanoidConfig.duckOffset / tilePixels } },
		}
		stateTypes.body.states.sit.properties.movementOffset = {
			{ "reset" }, { "translate", { 0, humanoidConfig.sitOffset / tilePixels } },
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
	end
	if humanoidConfig.bodyFullbright then
		for _, v in ipairs(humanoidConfig.bodyFullbrightParts or root.assetJson("/humanoid.config:bodyFullbrightParts")) do
			setPath(parts, { v, "properties", "fullbright" }, true)
		end
	end
	humanoidConfig.identityFramesetTags = humanoidConfig.identityFramesetTags or root.assetJson("/humanoid.config:identityFramesetTags")

	setPath(parts, { "anchor", "properties", "offset" }, vecTilePixels(humanoidConfig.globalOffset))
	setPath(parts, { humanoidConfig.mouthOffsetPart or "head", "properties", humanoidConfig.mouthOffsetPartPoint or "mouthOffset" }, vecTilePixels(humanoidConfig.mouthOffset))
	setPath(parts, { humanoidConfig.feetOffsetPart or "body", "properties", humanoidConfig.feetOffsetPartPoint or "feetOffset" }, vecTilePixels(humanoidConfig.feetOffset))

	setPath(parts, { humanoidConfig.headArmorOffsetPart or "headCosmetic", "properties", humanoidConfig.headArmorOffsetPartPoint or "armorOffset" }, vecTilePixels(humanoidConfig.headArmorOffset))
	setPath(parts, { humanoidConfig.chestArmorOffsetPart or "chestCosmetic", "properties", humanoidConfig.chestArmorOffsetPartPoint or "armorOffset" }, vecTilePixels(humanoidConfig.chestArmorOffset))
	setPath(parts, { humanoidConfig.legsArmorOffsetPart or "legsCosmetic", "properties", humanoidConfig.legsArmorOffsetPartPoint or "armorOffset" }, vecTilePixels(humanoidConfig.legsArmorOffset))
	setPath(parts, { humanoidConfig.backArmorOffsetPart or "backCosmetic", "properties", humanoidConfig.backArmorOffsetPartPoint or "armorOffset" }, vecTilePixels(humanoidConfig.backArmorOffset))

	setPath(parts, { humanoidConfig.frontArmRotationPart or "frontArm", "properties",  humanoidConfig.frontArmRotationPartPoint or "rotationCenter" }, vecTilePixels(humanoidConfig.frontArmRotationCenter))
	setPath(parts, { humanoidConfig.backArmRotationPart or "backArm", "properties", humanoidConfig.backArmRotationPartPoint or "rotationCenter" }, vecTilePixels({
		humanoidConfig.backArmRotationCenter[1] + humanoidConfig.backArmOffset[1],
		humanoidConfig.backArmRotationCenter[2] + humanoidConfig.backArmOffset[2],
	}))
	setPath(parts, { humanoidConfig.frontHandItemPart or "frontHandItem", "properties", "offset" }, vecTilePixels(humanoidConfig.frontHandPosition))
	setPath(parts, { humanoidConfig.backHandItemPart or "backHandItem", "properties", "offset" }, vecTilePixels({
		humanoidConfig.frontHandPosition[1] + humanoidConfig.backArmOffset[1],
		humanoidConfig.frontHandPosition[2] + humanoidConfig.backArmOffset[2],
	}))
	setPath(parts, { humanoidConfig.headRotationPart or "head", "properties",  humanoidConfig.headRotationPartPoint or "rotationCenter" },
		vecTilePixels(humanoidConfig.headRotationCenter or { 0, -2 }))

	-- these states play in reverse for moving backwards, run has a duplicate state for this because back items have a different frameset for it
	stateTypes.body.states.runbackwards = stateTypes.body.states.run
	stateTypes.bodyHighPriority.states.runbackwards = stateTypes.bodyHighPriority.states.run
	setPath(humanoidConfig, { "stateAnimationsBackwards", "walk", "body" }, { "walk", false, true })
	setPath(humanoidConfig, { "stateAnimationsBackwards", "walk", "bodyHighPriority" }, { "walk", false, true })
	setPath(humanoidConfig, { "stateAnimationsBackwards", "run", "body" }, { "runbackwards", false, true })
	setPath(humanoidConfig, { "stateAnimationsBackwards", "run", "bodyHighPriority" }, { "runbackwards", false, true })

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
