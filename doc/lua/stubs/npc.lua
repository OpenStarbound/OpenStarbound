---@meta

--- npc API
---@class npc
npc = {}

--- Returns the specified local position in world space. ---
---@param offset Vec2F
---@return Vec2F
function npc.toAbsolutePosition(offset) end

--- Returns the species of the npc. ---
---@return string
function npc.species() end

--- Returns the gender of the npc ---
---@return string
function npc.gender() end

--- Returns the specific humanoid identity of the npc, containing information such as hair style and idle pose. ---
---@return Json
function npc.humanoidIdentity() end

--- Returns the npc type of the npc. ---
---@return string
function npc.npcType() end

--- Returns the seed used to generate this npc. ---
---@return uint64_t
function npc.seed() end

--- Returns the level of the npc. ---
---@return number
function npc.level() end

--- Returns the list of treasure pools that will spawn when the npc dies. ---
---@return List<String>
function npc.dropPools() end

--- Sets the list of treasure pools that will spawn when the npc dies. ---
---@param pools List<String>
---@return void
function npc.setDropPools(pools) end

--- Returns the current energy of the npc. Same as `status.resource("energy")` ---
---@return number
function npc.energy() end

--- Returns the current energy of the npc. Same as `status.maxResource("energy")` ---
---@return number
function npc.maxEnergy() end

--- Makes the npc say a string. Optionally pass in tags to replace text tags. Optionally give config options for the chat message. Returns whether the chat message was successfully added. Available options: ``` { drawBorder = true, fontSize = 8, color = {255, 255, 255}, sound = "/sfx/humanoid/avian_chatter_male1.ogg" } ``` ---
---@param line string
---@param tags Map<String,String>
---@param config Json
---@return boolean
function npc.say(line, tags, config) end

--- Makes the npc say a line, with a portrait chat bubble. Optionally pass in tags to replace text tags. Optionally give config options for the chat message. Returns whether the chat message was successfully added. Available options: ``` { drawMoreIndicator = true, sound = "/sfx/humanoid/avian_chatter_male1.ogg" } ``` ---
---@param line string
---@param portrait string
---@param tags Map<String,String>
---@param config Json
---@return boolean
function npc.sayPortrait(line, portrait, tags, config) end

--- Makes the npc show a facial emote. ---
---@param emote string
---@return void
function npc.emote(emote) end

--- Sets the current dance for the npc. Dances are defined in .dance files. ---
---@param dance string
---@return void
function npc.dance(dance) end

--- Sets whether the npc should be interactive. ---
---@param interactive boolean
---@return void
function npc.setInteractive(interactive) end

--- Sets the npc to lounge in a loungeable. Optionally specify which anchor (seat) to use. Returns whether the npc successfully lounged. ---
---@param loungeable EntityId
---@param anchorIndex size_t
---@return boolean
function npc.setLounging(loungeable, anchorIndex) end

--- Makes the npc stop lounging. ---
---@return void
function npc.resetLounging() end

--- Returns whether the npc is currently lounging. ---
---@return boolean
function npc.isLounging() end

--- Returns the EntityId of the loungeable the NPC is currently lounging in. Returns nil if not lounging. ---
---@return Maybe<EntityId>
function npc.loungingIn() end

--- Sets the list of quests the NPC will offer. ---
---@param quests JsonArray
---@return void
function npc.setOfferedQuests(quests) end

--- Sets the list of quests the played can turn in at this npc. ---
---@param quests JsonArray
---@return void
function npc.setTurnInQuests(quests) end

--- Sets the specified item slot to contain the specified item. Possible equipment items slots: * head * headCosmetic * chest * chestCosmetic * legs * legsCosmetic * back * backCosmetic * primary * alt ---
---@param slot string
---@param item ItemDescriptor
---@return boolean
function npc.setItemSlot(slot, item) end

--- Returns the item currently in the specified item slot. ---
---@param slot string
---@return ItemDescriptor
function npc.getItemSlot(slot) end

--- Set whether the npc should not gain status effects from the equipped armor. Armor will still be visually equipped. ---
---@param disable boolean
---@return void
function npc.disableWornArmor(disable) end

--- Toggles `on` firing the item equipped in the `primary` item slot. ---
---@return void
function npc.beginPrimaryFire() end

--- Toggles `on` firing the item equipped in the `alt` item slot. ---
---@return void
function npc.beginAltFire() end

--- Toggles `off` firing the item equipped in the `primary` item slot. ---
---@return void
function npc.endPrimaryFire() end

--- Toggles `off` firing the item equipped in the `alt` item slot. ---
---@return void
function npc.endAltFire() end

--- Sets whether tools should be used as though shift is held. ---
---@param shifting boolean
---@return void
function npc.setShifting(shifting) end

--- Sets whether damage on touch should be enabled. ---
---@param enabled boolean
---@return void
function npc.setDamageOnTouch(enabled) end

--- Returns the current aim position in world space. ---
---@return Vec2F
function npc.aimPosition() end

--- Sets the aim position in world space. ---
---@param position Vec2F
---@return void
function npc.setAimPosition(position) end

--- Sets a particle emitter to burst when the npc dies. ---
---@param emitter string
---@return void
function npc.setDeathParticleBurst(emitter) end

--- Sets the text to appear above the npc when it first appears on screen. ---
---@param status string
---@return void
function npc.setStatusText(status) end

--- Sets whether the nametag should be displayed above the NPC. ---
---@param display boolean
---@return void
function npc.setDisplayNametag(display) end

--- Sets whether this npc should persist after being unloaded. ---
---@param persistent boolean
---@return void
function npc.setPersistent(persistent) end

--- Sets whether to keep this npc alive. If true, the npc will never be unloaded as long as the world is loaded. ---
---@param keepAlive boolean
---@return void
function npc.setKeepAlive(keepAlive) end

--- Sets a damage team for the npc in the format: `{type = "enemy", team = 2}` ---
---@param damageTeam Json
---@return void
function npc.setDamageTeam(damageTeam) end

--- Sets whether the npc should be flagged as aggressive. ---
---@param aggressive boolean
---@return void
function npc.setAggressive(aggressive) end

--- Sets a unique ID for this npc that can be used to access it. A unique ID has to be unique for the world the npc is on, but not universally unique.
---@param uniqueId string
---@return void
function npc.setUniqueId(uniqueId) end
