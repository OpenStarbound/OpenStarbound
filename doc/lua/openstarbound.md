# Unsorted

These are functions that aren't in any specific table.

---

#### `Maybe<LuaFunction>, Maybe<String>` loadstring(`String` source, [`String` name, [`LuaValue` env]])

Compiles the provided **source** and returns it as a callable function.
If there are any syntax errors, returns `nil` and the error as a string instead.

- **name** is used for error messages, the default is the name of the script that called `loadstring`.
- **env** is used as the environment of the returned function, the default is `_ENV`.

---

# Root

The root table now contains extra asset bindings and bindings to return the tile variant that is used for material and matmods at any position.

---

#### `String[]` root.assetsByExtension(`String` extension)

Returns an array containing all assets with the specified file extension.

By the way, here's a list of every file extension the game does Special Things™ for when loading assets.

<details><summary><b>File Extensions</b></summary>

- Items: `item`, `liqitem`, `matitem`, `miningtool`, `flashlight`, `wiretool`, `beamaxe`, `tillingtool`, `painttool`, `harvestingtool`, `head`, `chest`, `legs`, `back`, `currency`, `consumable`, `blueprint`, `inspectiontool`, `instrument`, `thrownitem`, `unlock`, `activeitem`, `augment`
- Materials: `material`, `matmod`
- Liquids: `liquid`
- NPCs: `npctype`
- Tenants: `tenant`
- Objects: `object`
- Vehicles: `vehicle`
- Monsters: `monstertype`, `monsterpart`, `monsterskill`, `monstercolors`
- Plants: `modularstem`, `modularfoliage`, `grass`, `bush`
- Projectiles: `projectile`
- Particles: `particle`
- Name Gen: `namesource`
- AI Missions: `aimission`
- Quests: `questtemplate`
- Radio Messages: `radiomessages`
- Spawn Types: `spawntypes`
- Species: `species`
- Stagehand: `stagehand`
- Behaviors: `nodes`, `behavior`
- Biomes: `biome`, `weather`
- Terrain: `terrain`
- Treasure: `treasurepools`, `treasurechests`
- Codex Entries: `codex`
- Collections: `collection`
- Statistics: `event`, `achievement`
- Status Effects: `statuseffect`
- Functions: `functions`, `2functions`, `configfunctions`
- Tech: `tech`
- Damage: `damage`
- Dances: `dance`
- Effect Sources: `effectsource`
- Command Macros: `macros`
- Recipes: `recipe`
</details>

#### `String` root.assetData(`String` path)

Returns the raw data of an asset.

#### `String, Maybe<LuaTable>` root.assetOrigin(`String` path, [`bool` getPatches])

Returns the asset source path of an asset, or nil if the asset doesn't exist. If you specify getPatches as true then also returns the patches for the asset as an array, each element containing the source path and patch path in indexes 1 and 2 respectively.

#### `LuaTable` root.assetSourcePaths([`bool` withMetadata])

Without metadata: Returns an array with all the asset source paths.
With metadata: Returns a table, key/value being source path/metadata.

#### `?` root.assetImage(`String` image)

*TODO*

#### `JsonArray` root.assetPatches(`String` asset)

Returns a list of asset sources which patch the specified asset and the paths to those patches.

---

#### `Json` root.getConfiguration(`String` key)

Gets a configuration value in `/storage/starbound.config`.

#### `Json` root.getConfigurationPath(`String` path)

Gets a configuration value in `/storage/starbound.config` by path.

*Both getters will error if you try to get `title`, as that can contain the player's saved server login.*

#### `Json` root.setConfiguration(`String` key, `Json` value)

Sets a configuration value in `/storage/starbound.config`.

#### `Json` root.setConfigurationPath(`String` path,  `Json` value)

Sets a configuration value in `/storage/starbound.config` by path.

*Both setters will error if you try to set `safeScripts`, as that can break Starbound's sandbox.*

---

#### `JsonArray` root.allRecipes()

Returns all recipes.

---

# Player

The player table now contains bindings which contains functions to save/load, access and modify the player's identity, mode, aim, emote and more.

---

#### `Json` player.save()

Serializes the player to Json the same way Starbound does for disk storage and returns it.

#### `void` player.load(`Json` save)

Reloads the player from a Json **save**. This will reset active ScriptPanes and scripts running on the player.
  
---

#### `String` player.name()

Returns the player's name.

#### `void` player.setName(`String` name)

Sets the player's name.

---

#### `String` player.description()

Returns the player's description.

#### `void` player.setDescription(`String` description)

Sets the player's description. The new description will not be networked buntil the player warps or respawns.

---

#### `void` player.setSpecies(`String` species)

Sets the player's species. Must be a valid species.

---

#### `void` player.setGender(`String` gender)

Sets the player's gender.

---

#### `String` player.imagePath()

If the player has a custom humanoid image path set, returns it. otherwise, returns `nil`.

#### `void` player.setImagePath(`String` imagePath)

Sets the player's image path. Specify `nil` to remove the image path.

---

#### `Personality` player.personality()

Returns the player's personality as a `table` containing a `string` idle, `string` armIdle, `Vec2F` headOffset and `Vec2F` armOffset.

#### `void` player.setPersonality(`Personality` personality)

Sets the player's personality. The **personality** must be a `table` containing at least one value as returned by `player.personality()`.

---

#### `String` player.bodyDirectives()

Returns the player's body directives.

#### `void` player.setBodyDirectives(`String` bodyDirectives)

Sets the player's body directives.

---

#### `String` player.emoteDirectives()

Returns the player's emote directives.

#### `void` player.setEmoteDirectives(`String` emoteDirectives)

Sets the player's emote directives.

---

#### `String` player.hair()

Returns the player's hair group.

#### `void` player.setHair(`String` hairGroup)

Sets the player's hair group.

---

#### `String` player.hairType()

Returns the player's hair type.

#### `void` player.setHairType(`String` hairType)

Sets the player's hair type.

---

#### `String` player.hairDirectives()

Returns the player's hair directives.

#### `void` player.setHairDirectives(`String` hairDirectives)

Sets the player's hair directives.

---

#### `String` player.facialHair()

Returns the player's facial hair type. Same as player.facialHairType?

#### `void` player.setFacialHair(`String` facialHairGroup, `String` facialHairType, `String` facialHairDirectives)

Sets the player's facial hair group, type, and directives.

---

#### `String` player.facialHairType()

Returns the player's facial hair type.

#### `void` player.setFacialHairType(`String` facialHairType)

Sets the player's facial hair type.

---

#### `String` player.facialHairGroup()

Returns the player's facial hair group.

#### `void` player.setFacialHairGroup(`String` facialHairGroup)

Sets the player's facial hair group.

---

#### `String` player.facialHairDirectives()

Returns the player's facial hair directives.

#### `void` player.setFacialHairDirectives(`String` facialHairDirectives)

Sets the player's facial hair directives.

---

#### `String` player.facialMask()

Returns the player's facial mask group.

#### `void` player.setFacialMask(`String` facialMaskGroup, `String` facialMaskType, `String` facialMaskDirectives)

Sets the player's facial mask group, type, and directives.

---

#### `String` player.facialMaskDirectives()

Returns the player's facial mask directives.

#### `void` player.setFacialMaskDirectives(`String` facialMaskDirectives)

Sets the player's facial mask directives.

---

#### `PlayerMode` player.mode()

Returns the player's mode.

#### `void` player.setMode(`String` mode)

Sets the player's mode. **mode** must be either `"casual"`, `"survival"` or `"hardcore"`.

---

#### `Color` player.favoriteColor()

Returns the player's favorite color.
It is used for the beam shown when wiring, placing, and highlighting with beam-tools (Matter Manipulator).

#### `void` player.setFavoriteColor(`Color` color)

Sets the player's favorite color. **color** can have an optional fourth value for transparency.

---

#### `Vec2F` player.aimPosition()

Returns the player's aim position.

---

#### `void` player.emote(`String` emote, [`float` cooldown])

Makes the player do an emote with the default cooldown unless a **cooldown** is specified.

#### `String, float` player.currentEmote()

Returns the player's current emote and the seconds left in it.
  
---

#### `unsigned` player.actionBarGroup()

Returns the player's active action bar.
  
#### `void` player.setActionBarGroup(`unsigned` barId)
  
Sets the player's active action bar.
  
#### `Variant<unsigned, EssentialItem>` player.selectedActionBarSlot()

Returns the player's selected action bar slot.
  
#### `void` player.setSelectedActionBarSlot(`Variant<unsigned, EssentialItem>` slot)
  
Sets the player's selected action bar slot.
  
#### `void` player.setDamageTeam(`DamageTeam` team)
  
Sets the player's damage team. This must be called every frame to override the current damage team that the server has given the player (normally controlled by /pvp)

---

#### `void` player.say(`String` line)

Makes the player say a string.

---

#### `Json` player.humanoidIdentity()

Returns the specific humanoid identity of the player, containing information such as hair style and idle pose.

#### `void` player.setHumanoidIdentity(`Json` humanoidIdentity)

Sets the specific humanoid identity of the player.

---

#### `ItemDescriptor` player.item(`ItemSlot` itemSlot)

Returns the contents of the specified itemSlot.

#### `void` player.setItem(`ItemSlot` itemSlot, `ItemDescriptor` item)

Puts the specified item into the specified itemSlot.
Item slots in item bags are structured like so: `{String bagName, int slot}`

---

#### `int` player.itemBagSize(`String` itemBagName)

Returns the size of an item bag.

#### `bool` player.itemAllowedInBag(`String` itemBagName, `ItemDescriptor` item)

Returns whether the specified item can enter the specified item bag. 

---

#### `ActionBarLink` player.actionBarSlotLink(`int` slot, `String` hand)

Returns the contents of the specified action bar link slot's specified hand.

#### `bool` player.setActionBarSlotLink(`String` itemBagName, `ItemDescriptor` item)

Returns whether the specified item can enter the specified item bag. 

---

#### `Float` player.interactRadius()

Returns the player's interact radius.

#### `void` player.setInteractRadius(`Float` interactRadius)

Sets the player's interact radius. This does not persist upon returning to the main menu.

---

#### `JsonArray` player.availableRecipes()

Returns all the recipes the player can craft with their currently held items and currencies.

---
#### `Json` player.getSecretProperty(`String` name, `Json` default)

Returns the value assigned to the specified temporary property (which is cleared upon returning to the main menu). If there is no value set, returns default.

---

#### `void` player.setSecretProperty(`String` name, `Json` value)

Sets a generic temporary property (which is cleared upon returning to the main menu) to the specified value.

