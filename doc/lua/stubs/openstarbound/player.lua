---@meta

--- player API
---@class player
player = {}

--- Serializes the player to Json the same way Starbound does for disk storage and returns it. ---
---@return Json
function player.save() end

--- Reloads the player from a Json **save**. This will reset active ScriptPanes and scripts running on the player. ---
---@param save Json
---@return void
function player.load(save) end

--- Returns the player's name. ---
---@return string
function player.name() end

--- Sets the player's name. ---
---@param name string
---@return void
function player.setName(name) end

--- Returns the player's description. ---
---@return string
function player.description() end

--- Sets the player's description. The new description will not be networked until the player warps or respawns. ---
---@param description string
---@return void
function player.setDescription(description) end

--- Refreshes the active humanoid parameters by re-building the current humanoid instance. This must be called when certain values in the humanoid config would be changed by a parameter, such as the animationConfig and movementParameters. This is implicitly called whenever the current humanoid identity's species or imagePath changes. This has to network the humanoid identity and humanoid parameters as intialization arguments for the new instance, so only use it when necessary. ---
---@return void
function player.refreshHumanoidParameters() end

--- Sets a parameter that overwrites a value in the humanoid config. ---
---@param key string
---@return void
function player.setHumanoidParameter(key) end

--- Replaces the current table of humanoid parameters that overwrite values in the humanoid config. ---
---@param parameters JsonObject
---@return void
function player.setHumanoidParameters(parameters) end

--- Gets a humanoid parameter. ---
---@param key string
---@return Maybe<Json>
function player.getHumanoidParameter(key) end

--- Gets a table of all the humanoid parameters. ---
---@return JsonObject
function player.getHumanoidParameters() end

--- Gets the active humanoid config. If withOverrides is true, returns it with the overrides applied by armor. The humanoid config controls how the player is animated, as well as their movement properties. ---
---@param withOverrides boolean
---@return JsonObject
function player.humanoidConfig(withOverrides) end

--- Sets the player's species. Must be a valid species. Implicitly calls `player.refreshHumanoidParameters()` if species is changed. ---
---@param species string
---@return void
function player.setSpecies(species) end

--- Sets the player's gender. ---
---@param gender string
---@return void
function player.setGender(gender) end

--- If the player has a custom humanoid image path set, returns it. otherwise, returns `nil`.
---@return string
function player.imagePath() end

--- Sets the player's image path. Specify `nil` to remove the image path. Implicitly calls `player.refreshHumanoidParameters()` if imagePath is changed. ---
---@param imagePath string
---@return void
function player.setImagePath(imagePath) end

--- Returns the player's personality as a `table` containing a `String` idle, `String` armIdle, `Vec2F` headOffset and `Vec2F` armOffset. ---
---@return Personality
function player.personality() end

--- Sets the player's personality. The **personality** must be a `table` containing at least one value as returned by `player.personality()`. ---
---@param personality Personality
---@return void
function player.setPersonality(personality) end

--- Returns the player's body directives. ---
---@return string
function player.bodyDirectives() end

--- Sets the player's body directives. ---
---@param bodyDirectives string
---@return void
function player.setBodyDirectives(bodyDirectives) end

--- Returns the player's emote directives. ---
---@return string
function player.emoteDirectives() end

--- Sets the player's emote directives. ---
---@param emoteDirectives string
---@return void
function player.setEmoteDirectives(emoteDirectives) end

--- Sets the player's hair group, type, and directives. ---
---@param hairGroup string
---@param hairType string
---@param hairDirectives string
---@return void
function player.setHair(hairGroup, hairType, hairDirectives) end

--- Returns the player's hair group. ---
---@return string
function player.hairGroup() end

--- Sets the player's hair group. ---
---@param hairGroup string
---@return void
function player.setHairGroup(hairGroup) end

--- Returns the player's hair type. ---
---@return string
function player.hairType() end

--- Sets the player's hair type. ---
---@param hairType string
---@return void
function player.setHairType(hairType) end

--- Returns the player's hair directives. ---
---@return string
function player.hairDirectives() end

--- Sets the player's hair directives. ---
---@param hairDirectives string
---@return void
function player.setHairDirectives(hairDirectives) end

--- Returns the player's facial hair type. Same as player.facialHairType? ---
---@return string
function player.facialHair() end

--- Sets the player's facial hair group, type, and directives. ---
---@param facialHairGroup string
---@param facialHairType string
---@param facialHairDirectives string
---@return void
function player.setFacialHair(facialHairGroup, facialHairType, facialHairDirectives) end

--- Returns the player's facial hair type. ---
---@return string
function player.facialHairType() end

--- Sets the player's facial hair type. ---
---@param facialHairType string
---@return void
function player.setFacialHairType(facialHairType) end

--- Returns the player's facial hair group. ---
---@return string
function player.facialHairGroup() end

--- Sets the player's facial hair group. ---
---@param facialHairGroup string
---@return void
function player.setFacialHairGroup(facialHairGroup) end

--- Returns the player's facial hair directives. ---
---@return string
function player.facialHairDirectives() end

--- Sets the player's facial hair directives. ---
---@param facialHairDirectives string
---@return void
function player.setFacialHairDirectives(facialHairDirectives) end

--- Returns the player's facial mask group. ---
---@return string
function player.facialMask() end

--- Sets the player's facial mask group, type, and directives. ---
---@param facialMaskGroup string
---@param facialMaskType string
---@param facialMaskDirectives string
---@return void
function player.setFacialMask(facialMaskGroup, facialMaskType, facialMaskDirectives) end

--- Returns the player's facial mask directives. ---
---@return string
function player.facialMaskDirectives() end

--- Sets the player's facial mask directives. ---
---@param facialMaskDirectives string
---@return void
function player.setFacialMaskDirectives(facialMaskDirectives) end

--- Returns the player's mode. ---
---@return PlayerMode
function player.mode() end

--- Sets the player's mode. **mode** must be either `"casual"`, `"survival"` or `"hardcore"`. ---
---@param mode string
---@return void
function player.setMode(mode) end

--- Returns the player's favorite color. It is used for the beam shown when wiring, placing, and highlighting with beam-tools (Matter Manipulator). ---
---@return Color
function player.favoriteColor() end

--- Sets the player's favorite color. **color** can have an optional fourth value for transparency. ---
---@param color Color
---@return void
function player.setFavoriteColor(color) end

--- Returns the player's aim position. ---
---@return Vec2F
function player.aimPosition() end

--- Makes the player do an emote with the default cooldown unless a **cooldown** is specified. ---
---@param emote string
---@param cooldown number
---@return void
function player.emote(emote, cooldown) end

--- Returns the player's current emote and the seconds left in it. ---
---@return string, number
function player.currentEmote() end

--- Returns the player's active action bar. ---
---@return unsigned
function player.actionBarGroup() end

--- Sets the player's active action bar. ---
---@param barId unsigned
---@return void
function player.setActionBarGroup(barId) end

--- Returns the player's selected action bar slot. ---
---@return Variant<unsigned, EssentialItem>
function player.selectedActionBarSlot() end

--- Sets the player's selected action bar slot. ---
---@return void
function player.setSelectedActionBarSlot() end

--- Sets the player's damage team. This must be called every frame to override the current damage team that the server has given the player (normally controlled by /pvp) ---
---@param team DamageTeam
---@return void
function player.setDamageTeam(team) end

--- Makes the player say a string. ---
---@param line string
---@return void
function player.say(line) end

--- Returns the specific humanoid identity of the player, containing information such as hair style and idle pose. ---
---@return Json
function player.humanoidIdentity() end

--- Sets the specific humanoid identity of the player. Implicitly calls `player.refreshHumanoidParameters()` if species or imagePath is changed. ---
---@param humanoidIdentity Json
---@return void
function player.setHumanoidIdentity(humanoidIdentity) end

--- Returns the contents of the specified itemSlot. ---
---@param itemSlot ItemSlot
---@return ItemDescriptor
function player.item(itemSlot) end

--- Puts the specified item into the specified itemSlot. Item slots in item bags are structured like so: `{String bagName, int slot}` ---
---@param itemSlot ItemSlot
---@param item ItemDescriptor
---@return void
function player.setItem(itemSlot, item) end

--- Returns the size of an item bag. ---
---@param itemBagName string
---@return number
function player.itemBagSize(itemBagName) end

--- Returns whether the specified item can enter the specified item bag. ---
---@param itemBagName string
---@param item ItemDescriptor
---@return boolean
function player.itemAllowedInBag(itemBagName, item) end

--- Returns the contents of the specified action bar link slot's specified hand. ---
---@param slot number
---@param hand string
---@return ActionBarLink
function player.actionBarSlotLink(slot, hand) end

--- Links the specified slot's hand to the specified itemSlot. ---
---@param slot number
---@param hand string
---@param itemSlot ItemSlot
---@return boolean
function player.setActionBarSlotLink(slot, hand, itemSlot) end

--- Returns the player's interact radius. ---
---@return Float
function player.interactRadius() end

--- Sets the player's interact radius. This does not persist upon returning to the main menu. ---
---@param interactRadius Float
---@return void
function player.setInteractRadius(interactRadius) end

--- Returns all the recipes the player can craft with their currently held items and currencies. ---
---@return JsonArray
function player.availableRecipes() end

--- Returns the player's current movement state. idle<br> walk<br> run<br> jump<br> fall<br> swim<br> swimIdle<br> lounge<br> crouch<br> teleportIn<br> teleportOut<br> ---
---@return string
function player.currentState() end

--- Returns an array, each entry being a table with `name`, `uuid`, `entity`, `healthPercentage` and `energyPercentage` ---
---@return List<Json>
function player.teamMembers() end

--- Returns `true` if the player knows the specified codexId, and `false` otherwise. ---
---@param codexId string
---@return boolean
function player.isCodexKnown(codexId) end

--- Returns `true` if the player has read the specified codexId, and `false` otherwise. ---
---@param codexId string
---@return boolean
function player.isCodexRead(codexId) end

--- Marks the specified codexId as read by the player. Returns `true` if the codex is known by the player and was marked as unread and `false` otherwise. ---
---@param codexId string
---@return boolean
function player.markCodexRead(codexId) end

--- Marks the specified codexId as not read by the player. Returns `true` if the codex is known by the player and was marked as read and `false` otherwise. ---
---@param codexId string
---@return boolean
function player.markCodexUnread(codexId) end

--- Make the player learn the specified codexId. If markRead is `true`, then the codex will be marked as read by default. ---
---@param codexId string
---@param markRead boolean
---@return void
function player.learnCodex(codexId, markRead) end

--- Returns a JSON object of key-value pairs, which are the codex ID and read status respectively. ---
---@return Json
function player.getCodexes() end

--- Returns the codex ID of the newest codex not read and `nil` otherwise. ---
---@return string
function player.getNewCodex() end

--- Calls a function in the specified quest script. ---
---@param questId string
---@param functionName string
---@param args LuaVariadic
---@return Maybe<LuaValue>
function player.callQuest(questId, functionName, args) end

--- Returns `true` if the player can turn in the specified quest. ---
---@param questId string
---@return Bool
function player.canTurnInQuest(questId) end

--- Returns the current quest as JSON, or `nil` if no quest is active. ---
---@return Maybe<Json>
function player.currentQuest() end

--- Returns the current quest ID, or `nil` if no quest is active. ---
---@return Maybe<String>
function player.currentQuestId() end

--- Makes the player perform a dance. Pass `nil` to stop dancing. ---
---@param dance string
---@return void
function player.dance(dance) end

--- Returns the effects animator callbacks table for the player. ---
---@return LuaTable
function player.effectsAnimator() end

--- Returns the player's nametag. ---
---@return string
function player.nametag() end

--- Returns quest information as JSON. ---
---@param questId string
---@return Json
function player.quest(questId) end

--- Returns a list of all quest IDs the player has. ---
---@return StringList
function player.questIds() end

--- Returns the objectives for the specified quest. ---
---@param questId string
---@return Json
function player.questObjectives(questId) end

--- Returns the portrait for the specified quest. ---
---@param questId string
---@return string
function player.questPortrait(questId) end

--- Returns the state of the specified quest. ---
---@param questId string
---@return string
function player.questState(questId) end

--- Removes a teleport bookmark. Returns `true` if successful. ---
---@param bookmarkConfig Json
---@return Bool
function player.removeTeleportBookmark(bookmarkConfig) end

--- Returns a list of server quest IDs. ---
---@return StringList
function player.serverQuestIds() end

--- Sets an animation parameter for the player's humanoid animator. ---
---@param parameterName string
---@param value Json
---@return void
function player.setAnimationParameter(parameterName, value) end

--- Sets the player's nametag. ---
---@param nametag string
---@return void
function player.setNametag(nametag) end

--- Sets the currently tracked quest. ---
---@param questId string
---@return void
function player.setTrackedQuest(questId) end

--- Makes the player stop lounging. ---
---@return void
function player.stopLounging() end

--- Returns the ID of the currently tracked quest.
---@return string
function player.trackedQuestId() end
