---@meta

--- player API
---@class player
player = {}

--- Returns the player's entity id. ---
---@return EntityId
function player.id() end

--- Returns the player's unique id. ---
---@return string
function player.uniqueId() end

--- Returns the player's species. ---
---@return string
function player.species() end

--- Returns the player's gender. ---
---@return string
function player.gender() end

--- Returns whether the player is admin. ---
---@return string
function player.isAdmin() end

--- Returns the value assigned to the specified generic player property. If there is no value set, returns default. ---
---@param name string
---@param default Json
---@return Json
function player.getProperty(name, default) end

--- Sets a generic player property to the specified value. ---
---@param name string
---@param value Json
---@return void
function player.setProperty(name, value) end

--- Adds the specified object to the player's scanned objects. ---
---@param name string
---@return void
function player.addScannedObject(name) end

--- Removes the specified object from the player's scanned objects. ---
---@param name string
---@return void
function player.removeScannedObject(name) end

--- Triggers an interact action on the player as if they had initiated an interaction and the result had returned the specified interaction type and configuration. Can be used to e.g. open GUI windows normally triggered by player interaction with entities. ---
---@param interactionType string
---@param config Json
---@param sourceEntityId EntityId
---@return void
function player.interact(interactionType, config, sourceEntityId) end

--- Returns a JSON object containing information about the player's current ship upgrades including "shipLevel", "maxFuel", "crewSize" and a list of "capabilities". ---
---@return Json
function player.shipUpgrades() end

--- Applies the specified ship upgrades to the player's ship. ---
---@param shipUpgrades Json
---@return void
function player.upgradeShip(shipUpgrades) end

--- Sets the specified universe flag on the player's current universe. ---
---@param flagName string
---@return void
function player.setUniverseFlag(flagName) end

--- Teaches the player any recipes which can be used to craft the specified item. ---
---@param item ItemDecriptor
---@return void
function player.giveBlueprint(item) end

--- Returns `true` if the player knows one or more recipes to create the specified item and `false` otherwise. ---
---@param item ItemDecriptor
---@return void
function player.blueprintKnown(item) end

--- Adds the specified tech to the player's list of available (unlockable) techs. ---
---@param tech string
---@return void
function player.makeTechAvailable(tech) end

--- Removes the specified tech from player's list of available (unlockable) techs. ---
---@param tech string
---@return void
function player.makeTechUnavailable(tech) end

--- Unlocks the specified tech, allowing it to be equipped through the tech GUI. ---
---@param tech string
---@return void
function player.enableTech(tech) end

--- Equips the specified tech. ---
---@param tech string
---@return void
function player.equipTech(tech) end

--- Unequips the specified tech. ---
---@param tech string
---@return void
function player.unequipTech(tech) end

--- Returns a list of the techs currently available to the player. ---
---@return JsonArray
function player.availableTechs() end

--- Returns a list of the techs currently unlocked by the player. ---
---@return JsonArray
function player.enabledTechs() end

--- Returns the name of the tech the player has currently equipped in the specified slot, or `nil` if no tech is equipped in that slot. ---
---@param slot string
---@return string
function player.equippedTech(slot) end

--- Returns the player's current total reserves of the specified currency. ---
---@param currencyName string
---@return unsigned
function player.currency(currencyName) end

--- Increases the player's reserve of the specified currency by the specified amount. ---
---@param currencyName string
---@param amount unsigned
---@return void
function player.addCurrency(currencyName, amount) end

--- Attempts to consume the specified amount of the specified currency and returns `true` if successful and `false` otherwise. ---
---@param currencyName string
---@param amount unsigned
---@return boolean
function player.consumeCurrency(currencyName, amount) end

--- Triggers an immediate cleanup of the player's inventory, removing item stacks with 0 quantity. May rarely be required in special cases of making several sequential modifications to the player's inventory within a single tick. ---
---@return void
function player.cleanupItems() end

--- Adds the specified item to the player's inventory. ---
---@param item ItemDescriptor
---@return void
function player.giveItem(item) end

--- Returns `true` if the player's inventory contains an item matching the specified descriptor and `false` otherwise. If exactMatch is `true` then parameters as well as item name must match. ---
---@param item ItemDescriptor
---@param exactMatch boolean
---@return boolean
function player.hasItem(item, exactMatch) end

--- Returns the total number of items in the player's inventory matching the specified descriptor. If exactMatch is `true` then parameters as well as item name must match. ---
---@param item ItemDescriptor
---@param exactMatch boolean
---@return unsigned
function player.hasCountOfItem(item, exactMatch) end

--- Attempts to consume the specified item from the player's inventory and returns the item consumed if successful. If consumePartial is `true`, matching stacks totalling fewer items than the requested count may be consumed, otherwise the operation will only be performed if the full count can be consumed. If exactMatch is `true` then parameters as well as item name must match. ---
---@param item ItemDescriptor
---@param consumePartial boolean
---@param exactMatch boolean
---@return ItemDescriptor
function player.consumeItem(item, consumePartial, exactMatch) end

--- Returns a summary of all tags of all items in the player's inventory. Keys in the returned map are tag names and their corresponding values are the total count of items including that tag. ---
---@return Map<String, unsigned>
function player.inventoryTags() end

--- Returns a list of `ItemDescriptor`s for all items in the player's inventory that include the specified tag. ---
---@param tag string
---@return JsonArray
function player.itemsWithTag(tag) end

--- Consumes items from the player's inventory that include the matching tag, up to the specified count of items. ---
---@param tag string
---@param count unsigned
---@return void
function player.consumeTaggedItem(tag, count) end

--- Returns `true` if the player's inventory contains at least one item which has the specified parameter set to the specified value. ---
---@param parameter string
---@param value Json
---@return boolean
function player.hasItemWithParameter(parameter, value) end

--- Consumes items from the player's inventory that have the specified parameter set to the specified value, upt to the specified count of items. ---
---@param parameter string
---@param value Json
---@param count unsigned
---@return void
function player.consumeItemWithParameter(parameter, value, count) end

--- Returns the first item in the player's inventory that has the specified parameter set to the specified value, or `nil` if no such item is found. ---
---@param parameter string
---@param value Json
---@return ItemDescriptor
function player.getItemWithParameter(parameter, value) end

--- Returns the player's currently equipped primary hand item, or `nil` if no item is equipped. ---
---@return ItemDescriptor
function player.primaryHandItem() end

--- Returns the player's currently equipped alt hand item, or `nil` if no item is equipped. ---
---@return ItemDescriptor
function player.altHandItem() end

--- Returns a list of the tags on the currently equipped primary hand item, or `nil` if no item is equipped. ---
---@return JsonArray
function player.primaryHandItemTags() end

--- Returns a list of the tags on the currently equipped alt hand item, or `nil` if no item is equipped. ---
---@return JsonArray
function player.altHandItemTags() end

--- Returns the contents of the specified essential slot, or `nil` if the slot is empty. Essential slot names are "beamaxe", "wiretool", "painttool" and "inspectiontool". ---
---@param slotName string
---@return ItemDescriptor
function player.essentialItem(slotName) end

--- Sets the contents of the specified essential slot to the specified item. ---
---@param slotName string
---@param item ItemDescriptor
---@return void
function player.giveEssentialItem(slotName, item) end

--- Removes the essential item in the specified slot. ---
---@param slotName string
---@return void
function player.removeEssentialItem(slotName) end

--- Returns the contents of the specified equipment slot, or `nil` if the slot is empty. Equipment slot names are "head", "chest", "legs", "back", "headCosmetic", "chestCosmetic", "legsCosmetic" and "backCosmetic". ---
---@param slotName string
---@return ItemDescriptor
function player.equippedItem(slotName) end

--- Sets the item in the specified equipment slot to the specified item. ---
---@param slotName string
---@param item Json
---@return void
function player.setEquippedItem(slotName, item) end

--- Returns the contents of the player's swap (cursor) slot, or `nil` if the slot is empty. ---
---@return ItemDescriptor
function player.swapSlotItem() end

--- Sets the item in the player's swap (cursor) slot to the specified item. ---
---@param item Json
---@return void
function player.setSwapSlotItem(item) end

--- Returns `true` if the player meets all of the prerequisites to start the specified quest and `false` otherwise. ---
---@param questDescriptor Json
---@return boolean
function player.canStartQuest(questDescriptor) end

--- Starts the specified quest, optionally using the specified server Uuid and world id, and returns the quest id of the started quest. ---
---@param questDescriptor Json
---@param serverUuid string
---@param worldId string
---@return QuestId
function player.startQuest(questDescriptor, serverUuid, worldId) end

--- Returns `true` if the player has a quest, in any state, with the specified quest id and `false` otherwise. ---
---@param questId string
---@return boolean
function player.hasQuest(questId) end

--- Returns `true` if the player has accepted a quest (which may be active, completed, or failed) with the specified quest id and `false` otherwise. ---
---@param questId string
---@return boolean
function player.hasAcceptedQuest(questId) end

--- Returns `true` if the player has a currently active quest with the specified quest id and `false` otherwise. ---
---@param questId string
---@return boolean
function player.hasActiveQuest(questId) end

--- Returns `true` if the player has a completed quest with the specified quest id and `false` otherwise. ---
---@param questId string
---@return boolean
function player.hasCompletedQuest(questId) end

--- If the player's currently tracked quest has an associated world, returns the id of that world. ---
---@return Maybe<WorldId>
function player.currentQuestWorld() end

--- Returns a list of world ids for worlds relevant to the player's current quests, along with a boolean indicating whether that quest is tracked. ---
---@return List<pair<WorldId, bool>>
function player.questWorlds() end

--- If the player's currently tracked quest has an associated location (CelestialCoordinate, system orbit, UUID, or system position) returns that location. ---
---@return Maybe<Json>
function player.currentQuestLocation() end

--- Returns a list of locations for worlds relevant to the player's current quests, along with a boolean indicating whether that quest is tracked. ---
---@return List<pair<Json, bool>>
function player.questLocations() end

--- Adds the specified mission to the player's list of available missions. ---
---@param missionName string
---@return void
function player.enableMission(missionName) end

--- Adds the specified mission to the player's list of completed missions. ---
---@param missionName string
---@return void
function player.completeMission(missionName) end

--- Returns whether the player has completed the specified mission. ---
---@param missionName string
---@return void
function player.hasCompletedMission(missionName) end

--- Triggers the specified radio message for the player, either immediately or with the specified delay. ---
---@param messageConfig Json
---@param delay number
---@return void
function player.radioMessage(messageConfig, delay) end

--- Returns a `String` representation of the world id of the player's current world. ---
---@return string
function player.worldId() end

--- Returns a `String` representation of the player's Uuid on the server. ---
---@return string
function player.serverUuid() end

--- Returns a `String` representation of the world id of the player's ship world. ---
---@return string
function player.ownShipWorldId() end

--- Triggers the player to lounge in the specified loungeable entity at the specified lounge anchor index (default is 0). ---
---@param loungeableId EntityId
---@param anchorIndex unsigned
---@return boolean
function player.lounge(loungeableId, anchorIndex) end

--- Returns `true` if the player is currently occupying a loungeable entity and `false` otherwise. ---
---@return boolean
function player.isLounging() end

--- If the player is currently lounging, returns the entity id of what they are lounging in. ---
---@return EntityId
function player.loungingIn() end

--- Returns the total played time for the player. ---
---@return number
function player.playTime() end

--- Returns `true` if the player is marked as having completed the intro instance and `false` otherwise. ---
---@return boolean
function player.introComplete() end

--- Sets whether the player is marked as having completed the intro instance. ---
---@param complete boolean
---@return void
function player.setIntroComplete(complete) end

--- Immediately warps the player to the specified warp target, optionally using the specified warp animation and deployment. ---
---@param warpAction string
---@param animation string
---@param deploy boolean
---@return void
function player.warp(warpAction, animation, deploy) end

--- Returns whether the player has a deployable mech. ---
---@return boolean
function player.canDeploy() end

--- Returns whether the player is currently deployed. ---
---@return boolean
function player.isDeployed() end

--- Displays a confirmation dialog to the player with the specified dialog configuration and returns an `RpcPromise` which can be used to retrieve the player's response to that dialog. ---
---@param dialogConfig Json
---@return RpcPromise
function player.confirm(dialogConfig) end

--- Triggers the specified cinematic to be displayed for the player. If unique is `true` the cinematic will only be shown to that player once. ---
---@param cinematic Json
---@param unique boolean
---@return void
function player.playCinematic(cinematic, unique) end

--- Triggers the specified event on the player with the specified fields. Used to record data e.g. for achievements. ---
---@param event string
---@param fields Json
---@return void
function player.recordEvent(event, fields) end

--- Returns whether the player has a bookmark for the specified celestial coordinate. ---
---@param coordinate Json
---@return boolean
function player.worldHasOrbitBookmark(coordinate) end

--- Returns a list of orbit bookmarks with their system coordinates. ---
---@return List<pair<Vec3I, Json>>
function player.orbitBookmarks() end

--- Returns a list of orbit bookmarks in the specified system. ---
---@param systemCoordinate Json
---@return List<Json>
function player.systemBookmarks(systemCoordinate) end

--- Adds the specified bookmark to the player's bookmark list and returns `true` if the bookmark was successfully added (and was not already known) and `false` otherwise. ---
---@param systemCoordinate Json
---@param bookmarkConfig Json
---@return boolean
function player.addOrbitBookmark(systemCoordinate, bookmarkConfig) end

--- Removes the specified bookmark from the player's bookmark list and returns `true` if the bookmark was successfully removed and `false` otherwise. ---
---@param systemCoordinate Json
---@param bookmarkConfig Json
---@return boolean
function player.removeOrbitBookmark(systemCoordinate, bookmarkConfig) end

--- Lists all of the player's teleport bookmarks. ---
---@return List<Json>
function player.teleportBookmarks() end

--- Adds the specified bookmark to the player's bookmark list and returns `true` if the bookmark was successfully added (and was not already known) and `false` otherwise. ---
---@param bookmarkConfig Json
---@return boolean
function player.addTeleportBookmark(bookmarkConfig) end

--- Removes the specified teleport bookmark. ---
---@param bookmarkConfig Json
---@return boolean
function player.removeTeleportBoookmark(bookmarkConfig) end

--- Returns whether the player has previously visited the specified coordinate. ---
---@param coordinate Json
---@return boolean
function player.isMapped(coordinate) end

--- Returns uuid, type, and orbits for all system objects in the specified system; ---
---@param systemCoordinate Json
---@return Json
function player.mappedObjects(systemCoordinate) end

--- Returns a list of names of the collectables the player has unlocked in the specified collection. ---
---@param collectionName string
---@return List<String>
function player.collectables(collectionName) end
