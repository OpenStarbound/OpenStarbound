
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

#### `Json` root.assetSourceMetadata(`String` path)

Returns the metadata of an asset source.

#### `Image` root.assetImage(`String` image)

Returns an image.

#### `Json` root.assetFrames(`String` imagePath)

Returns an array containing a `file` (the frames file used for the image) and `frames` (the frame data of the image).

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

#### `String` root.itemFile(`String` itemName)

Returns the asset file path for the specified item.

---

#### `JsonObject` root.effectConfig(`String` effect)

Returns the config for the status effect.

---
