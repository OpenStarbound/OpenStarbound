The player table contains functions with privileged access to the player which run in a few contexts on the client such as scripted interface panes, quests, and player companions.

---

#### `EntityId` player.id()

Returns the player's entity id.

---

#### `String` player.uniqueId()

Returns the player's unique id.

---

#### `String` player.species()

Returns the player's species.

---

#### `String` player.gender()

Returns the player's gender.

---

#### `String` player.isAdmin()

Returns whether the player is admin.

---

#### `Json` player.getProperty(`String` name, `Json` default)

Returns the value assigned to the specified generic player property. If there is no value set, returns default.

---

#### `void` player.setProperty(`String` name, `Json` value)

Sets a generic player property to the specified value.

---

#### `void` player.addScannedObject(`String` name)

Adds the specified object to the player's scanned objects.

---

#### `void` player.removeScannedObject(`String` name)

Removes the specified object from the player's scanned objects.

---

#### `void` player.interact(`String` interactionType, `Json` config, [`EntityId` sourceEntityId])

Triggers an interact action on the player as if they had initiated an interaction and the result had returned the specified interaction type and configuration. Can be used to e.g. open GUI windows normally triggered by player interaction with entities.

---

#### `Json` player.shipUpgrades()

Returns a JSON object containing information about the player's current ship upgrades including "shipLevel", "maxFuel", "crewSize" and a list of "capabilities".

---

#### `void` player.upgradeShip(`Json` shipUpgrades)

Applies the specified ship upgrades to the player's ship.

---

#### `void` player.setUniverseFlag(`String` flagName)

Sets the specified universe flag on the player's current universe.

---

#### `void` player.giveBlueprint(`ItemDecriptor` item)

Teaches the player any recipes which can be used to craft the specified item.

---

#### `void` player.blueprintKnown(`ItemDecriptor` item)

Returns `true` if the player knows one or more recipes to create the specified item and `false` otherwise.

---

#### `void` player.makeTechAvailable(`String` tech)

Adds the specified tech to the player's list of available (unlockable) techs.

---

#### `void` player.makeTechUnavailable(`String` tech)

Removes the specified tech from player's list of available (unlockable) techs.

---

#### `void` player.enableTech(`String` tech)

Unlocks the specified tech, allowing it to be equipped through the tech GUI.

---

#### `void` player.equipTech(`String` tech)

Equips the specified tech.

---

#### `void` player.unequipTech(`String` tech)

Unequips the specified tech.

---

#### `JsonArray` player.availableTechs()

Returns a list of the techs currently available to the player.

---

#### `JsonArray` player.enabledTechs()

Returns a list of the techs currently unlocked by the player.

---

#### `String` player.equippedTech(`String` slot)

Returns the name of the tech the player has currently equipped in the specified slot, or `nil` if no tech is equipped in that slot.

---

#### `unsigned` player.currency(`String` currencyName)

Returns the player's current total reserves of the specified currency.

---

#### `void` player.addCurrency(`String` currencyName, `unsigned` amount)

Increases the player's reserve of the specified currency by the specified amount.

---

#### `bool` player.consumeCurrency(`String` currencyName, `unsigned` amount)

Attempts to consume the specified amount of the specified currency and returns `true` if successful and `false` otherwise.

---

#### `void` player.cleanupItems()

Triggers an immediate cleanup of the player's inventory, removing item stacks with 0 quantity. May rarely be required in special cases of making several sequential modifications to the player's inventory within a single tick.

---

#### `void` player.giveItem(`ItemDescriptor` item)

Adds the specified item to the player's inventory.

---

#### `bool` player.hasItem(`ItemDescriptor` item, [`bool` exactMatch])

Returns `true` if the player's inventory contains an item matching the specified descriptor and `false` otherwise. If exactMatch is `true` then parameters as well as item name must match.

---

#### `unsigned` player.hasCountOfItem(`ItemDescriptor` item, [`bool` exactMatch])

Returns the total number of items in the player's inventory matching the specified descriptor. If exactMatch is `true` then parameters as well as item name must match.

---

#### `ItemDescriptor` player.consumeItem(`ItemDescriptor` item, [`bool` consumePartial], [`bool` exactMatch])

Attempts to consume the specified item from the player's inventory and returns the item consumed if successful. If consumePartial is `true`, matching stacks totalling fewer items than the requested count may be consumed, otherwise the operation will only be performed if the full count can be consumed. If exactMatch is `true` then parameters as well as item name must match.

---

#### `Map<String, unsigned>` player.inventoryTags()

Returns a summary of all tags of all items in the player's inventory. Keys in the returned map are tag names and their corresponding values are the total count of items including that tag.

---

#### `JsonArray` player.itemsWithTag(`String` tag)

Returns a list of `ItemDescriptor`s for all items in the player's inventory that include the specified tag.

---

#### `void` player.consumeTaggedItem(`String` tag, `unsigned` count)

Consumes items from the player's inventory that include the matching tag, up to the specified count of items.

---

#### `bool` player.hasItemWithParameter(`String` parameter, `Json` value)

Returns `true` if the player's inventory contains at least one item which has the specified parameter set to the specified value.

---

#### `void` player.consumeItemWithParameter(`String` parameter, `Json` value, `unsigned` count)

Consumes items from the player's inventory that have the specified parameter set to the specified value, upt to the specified count of items.

---

#### `ItemDescriptor` player.getItemWithParameter(`String` parameter, `Json` value)

Returns the first item in the player's inventory that has the specified parameter set to the specified value, or `nil` if no such item is found.

---

#### `ItemDescriptor` player.primaryHandItem()

Returns the player's currently equipped primary hand item, or `nil` if no item is equipped.

---

#### `ItemDescriptor` player.altHandItem()

Returns the player's currently equipped alt hand item, or `nil` if no item is equipped.

---

#### `JsonArray` player.primaryHandItemTags()

Returns a list of the tags on the currently equipped primary hand item, or `nil` if no item is equipped.

---

#### `JsonArray` player.altHandItemTags()

Returns a list of the tags on the currently equipped alt hand item, or `nil` if no item is equipped.

---

#### `ItemDescriptor` player.essentialItem(`String` slotName)

Returns the contents of the specified essential slot, or `nil` if the slot is empty. Essential slot names are "beamaxe", "wiretool", "painttool" and "inspectiontool".

---

#### `void` player.giveEssentialItem(`String` slotName, `ItemDescriptor` item)

Sets the contents of the specified essential slot to the specified item.

---

#### `void` player.removeEssentialItem(`String` slotName)

Removes the essential item in the specified slot.

---

#### `ItemDescriptor` player.equippedItem(`String` slotName)

Returns the contents of the specified equipment slot, or `nil` if the slot is empty. Equipment slot names are "head", "chest", "legs", "back", "headCosmetic", "chestCosmetic", "legsCosmetic" and "backCosmetic".

---

#### `void` player.setEquippedItem(`String` slotName, `Json` item)

Sets the item in the specified equipment slot to the specified item.

---

#### `ItemDescriptor` player.swapSlotItem()

Returns the contents of the player's swap (cursor) slot, or `nil` if the slot is empty.

---

#### `void` player.setSwapSlotItem(`Json` item)

Sets the item in the player's swap (cursor) slot to the specified item.

---

#### `bool` player.canStartQuest(`Json` questDescriptor)

Returns `true` if the player meets all of the prerequisites to start the specified quest and `false` otherwise.

---

#### `QuestId` player.startQuest(`Json` questDescriptor, [`String` serverUuid], [`String` worldId])

Starts the specified quest, optionally using the specified server Uuid and world id, and returns the quest id of the started quest.

---

#### `bool` player.hasQuest(`String` questId)

Returns `true` if the player has a quest, in any state, with the specified quest id and `false` otherwise.

---

#### `bool` player.hasAcceptedQuest(`String` questId)

Returns `true` if the player has accepted a quest (which may be active, completed, or failed) with the specified quest id and `false` otherwise.

---

#### `bool` player.hasActiveQuest(`String` questId)

Returns `true` if the player has a currently active quest with the specified quest id and `false` otherwise.

---

#### `bool` player.hasCompletedQuest(`String` questId)

Returns `true` if the player has a completed quest with the specified quest id and `false` otherwise.

---

#### `Maybe<WorldId>` player.currentQuestWorld()

If the player's currently tracked quest has an associated world, returns the id of that world.

---

#### `List<pair<WorldId, bool>>` player.questWorlds()

Returns a list of world ids for worlds relevant to the player's current quests, along with a boolean indicating whether that quest is tracked.

---

#### `Maybe<Json>` player.currentQuestLocation()

If the player's currently tracked quest has an associated location (CelestialCoordinate, system orbit, UUID, or system position) returns that location.

---

#### `List<pair<Json, bool>>` player.questLocations()

Returns a list of locations for worlds relevant to the player's current quests, along with a boolean indicating whether that quest is tracked.

---

#### `void` player.enableMission(`String` missionName)

Adds the specified mission to the player's list of available missions.

---

#### `void` player.completeMission(`String` missionName)

Adds the specified mission to the player's list of completed missions.

---

#### `void` player.hasCompletedMission(`String` missionName)

Returns whether the player has completed the specified mission.

---

#### `void` player.radioMessage(`Json` messageConfig, [`float` delay])

Triggers the specified radio message for the player, either immediately or with the specified delay.

---

#### `String` player.worldId()

Returns a `String` representation of the world id of the player's current world.

---

#### `String` player.serverUuid()

Returns a `String` representation of the player's Uuid on the server.

---

#### `String` player.ownShipWorldId()

Returns a `String` representation of the world id of the player's ship world.

---

#### `bool` player.lounge(`EntityId` loungeableId, [`unsigned` anchorIndex])

Triggers the player to lounge in the specified loungeable entity at the specified lounge anchor index (default is 0).

---

#### `bool` player.isLounging()

Returns `true` if the player is currently occupying a loungeable entity and `false` otherwise.

---

#### `EntityId` player.loungingIn()

If the player is currently lounging, returns the entity id of what they are lounging in.

---

#### `double` player.playTime()

Returns the total played time for the player.

---

#### `bool` player.introComplete()

Returns `true` if the player is marked as having completed the intro instance and `false` otherwise.

---

#### `void` player.setIntroComplete(`bool` complete)

Sets whether the player is marked as having completed the intro instance.

---

#### `void` player.warp(`String` warpAction, [`String` animation], [`bool` deploy])

Immediately warps the player to the specified warp target, optionally using the specified warp animation and deployment.

---

#### `bool` player.canDeploy()

Returns whether the player has a deployable mech.

---

#### `bool` player.isDeployed()

Returns whether the player is currently deployed.

---

#### `RpcPromise` player.confirm(`Json` dialogConfig)

Displays a confirmation dialog to the player with the specified dialog configuration and returns an `RpcPromise` which can be used to retrieve the player's response to that dialog.

---

#### `void` player.playCinematic(`Json` cinematic, [`bool` unique])

Triggers the specified cinematic to be displayed for the player. If unique is `true` the cinematic will only be shown to that player once.

---

#### `void` player.recordEvent(`String` event, `Json` fields)

Triggers the specified event on the player with the specified fields. Used to record data e.g. for achievements.

---

#### `bool` player.worldHasOrbitBookmark(`Json` coordinate)

Returns whether the player has a bookmark for the specified celestial coordinate.

---

#### `List<pair<Vec3I, Json>>` player.orbitBookmarks()

Returns a list of orbit bookmarks with their system coordinates.

---

#### `List<Json>` player.systemBookmarks(`Json` systemCoordinate)

Returns a list of orbit bookmarks in the specified system.

---

#### `bool` player.addOrbitBookmark(`Json` systemCoordinate, `Json` bookmarkConfig)

Adds the specified bookmark to the player's bookmark list and returns `true` if the bookmark was successfully added (and was not already known) and `false` otherwise.

---

#### `bool` player.removeOrbitBookmark(`Json` systemCoordinate, `Json` bookmarkConfig)

Removes the specified bookmark from the player's bookmark list and returns `true` if the bookmark was successfully removed and `false` otherwise.

---

#### `List<Json>` player.teleportBookmarks()

Lists all of the player's teleport bookmarks.

---

#### `bool` player.addTeleportBookmark(`Json` bookmarkConfig)

Adds the specified bookmark to the player's bookmark list and returns `true` if the bookmark was successfully added (and was not already known) and `false` otherwise.

---

#### `bool` player.removeTeleportBoookmark(`Json` bookmarkConfig)

Removes the specified teleport bookmark.

---

#### `bool` player.isMapped(`Json` coordinate)

Returns whether the player has previously visited the specified coordinate.

---

#### `Json` player.mappedObjects(`Json` systemCoordinate)

Returns uuid, type, and orbits for all system objects in the specified system;

---

#### `List<String>` player.collectables(`String` collectionName)

Returns a list of names of the collectables the player has unlocked in the specified collection.

---

#### `List<Json>` player.teamMembers()

Returns an array, each entry being a table with `name`, `uuid`, `entity`, `healthPercentage` and `energyPercentage`
