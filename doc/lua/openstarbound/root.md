# Root

The `root` table now contains extra asset bindings and bindings to return the tile variant that is used for material and matmods at any position.

---

#### `unsigned` root.materialVariant(`Vec2U` tilePosition, `String` layer, `unsigned` variants)

Returns the variant index starting from 0 for the material variant used at the specified **tilePosition**. **layer** must be either `"foreground"`, `"background"` or `"platform"`, and **variants** must be the amount of variants the material has, which you can use the vanilla `root.materialConfig` function to fetch.

#### `unsigned` root.modVariant(`Vec2U` tilePosition, `String` layer, `unsigned` variants)

Works like `root.materialVariant` but for matmods.

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

---

#### `String` root.assetData(`String` path, [`bool` allowRemote])

Returns the raw data of an asset.

If `allowRemote` is true, allows reading the binary data of embedded assets.
    This is disallowed by default for security reasons in cases where an older script may `loadstring` this data, only enable this if you know what you are doing!

---

#### `String, Maybe<LuaTable>` root.assetOrigin(`String` path, [`bool` getPatches])

Returns the asset source path of an asset, or nil if the asset doesn't exist. If you specify getPatches as true then also returns the patches for the asset as an array, each element containing the source path and patch path in indexes 1 and 2 respectively.

---

#### `LuaTable` root.assetSourcePaths([`bool` withMetadata])

Without metadata: Returns an array with all the asset source paths.
With metadata: Returns a table, key/value being source path/metadata.

---

#### `Json` root.assetSourceMetadata(`String` path)

Returns the metadata of an asset source.

---

#### `Image` root.assetImage(`String` image)

Returns an image.

---

#### `Json` root.assetFrames(`String` imagePath)

Returns an array containing a `file` (the frames file used for the image) and `frames` (the frame data of the image).

---

#### `JsonArray` root.assetPatches(`String` asset)

Returns a list of asset sources which patch the specified asset and the paths to those patches.

---

#### `Json` root.getConfiguration(`String` key)

Gets a configuration value in `/storage/starbound.config`.

---

#### `Json` root.getConfigurationPath(`String` path)

Gets a configuration value in `/storage/starbound.config` by path.

*Both getters will error if you try to get `title`, as that can contain the player's saved server login.*

---

#### `Json` root.setConfiguration(`String` key, `Json` value)

Sets a configuration value in `/storage/starbound.config`.

---

#### `Json` root.setConfigurationPath(`String` path,  `Json` value)

Sets a configuration value in `/storage/starbound.config` by path.

*Both setters will error if you try to set `safeScripts`, as that can break Starbound's sandbox.*

---

### Mod storage

A mod can keep data between sessions with `root.getModStorage` and the functions next to it. This is a key/value store that is private to the mod and works with `safeScripts` on, so a mod does not need to turn the sandbox off to save its settings or progress.

- **Which mod:** the mod is the asset source (the pak or asset directory) that the calling script was loaded from. A script cannot name another mod and so cannot read or change another mod's data. The functions only work in scripts loaded from assets, and error in code made with `loadstring`.
- **Name:** the storage is named after the `name` in the source's `_metadata`, or after its file name if it has none. If several loaded sources have the same name, the first in load order keeps it and the others are stored as `name (file name)`, so they never share data.
- **Where:** `/storage/modstorage/<name>.json`, one file per mod. Characters that are not letters, digits, `-` or `_` are written as `%XX` in the file name. It is plain JSON, so it can be read or edited by hand while the game is closed.
- **Values:** anything JSON can hold (`nil` is "not set"). Lua tables become JSON objects or arrays in the usual way, and the usual rules for JSON in Lua apply (a table with keys 1 to n is an array, any other table is an object, so an empty table is an object, use the global `jarray()` to store an empty array). Functions and other values JSON cannot hold make the call error.
- **Size:** at most 4 MiB of serialized data per mod. A `set` that would go over errors and changes nothing.
- **Saving:** changes are kept in memory and written about two seconds after the first change, and when the game exits, so setting a key every frame is cheap. A crash can lose the last two seconds of changes, use `root.flushModStorage` to write at once.
- **Scope:** the data is per mod and per machine. A server runs its own copy in its own storage directory. To keep data per character or per world, put the id in the key, for example `player.uniqueId() .. ".volume"` or `world.id() .. ".visited"`.

```lua
function init()
  -- 1 + the number of earlier sessions, kept across restarts
  local runs = (root.getModStorage("runs") or 0) + 1
  root.setModStorage("runs", runs)

  -- per character
  local key = player.uniqueId() .. ".settings"
  local settings = root.getModStorage(key) or {volume = 5}
  settings.volume = math.min(settings.volume + 1, 10)
  root.setModStorage(key, settings)
end
```

---

#### `Json` root.getModStorage(`String` key)

Returns the value stored under **key** in the calling mod's storage, or `nil` if there is none. The result is a copy, so changing a returned table does not change the stored value until it is passed to `root.setModStorage` again.

Errors if the script was not loaded from assets.

---

#### `void` root.setModStorage(`String` key, `Json` value)

Stores **value** under **key** in the calling mod's storage. Passing `nil` removes the key. Setting a value equal to the stored one does nothing, and the write to disk comes after a short delay, see above.

Errors, and leaves the storage as it was, if the mod's data would be larger than 4 MiB afterwards, or if the script was not loaded from assets.

---

#### `JsonArray` root.modStorageKeys()

Returns all keys in the calling mod's storage, in no particular order.

---

#### `void` root.clearModStorage()

Removes every key from the calling mod's storage.

---

#### `void` root.flushModStorage()

Writes all mod storage changes that are waiting for disk to disk now, instead of waiting for the delay. Use it right before something that could crash the game, or after a change that must not be lost. It does nothing when no change is waiting, so it is cheap to call.

Unlike the other functions it applies to the storage of every mod, not only the calling one.

---

#### `JsonArray` root.allRecipes(`Maybe<StringSet>` filter)

Returns all recipes.

Optionally apply a filter for recipe groups.

---

#### `String` root.itemFile(`String` itemName)

Returns the asset file path for the specified item.

---

#### `JsonObject` root.speciesConfig(`String` species)

Returns the config for the species.

---

#### `LuaTupleReturn<Json, JsonObject, JsonObject>` root.generateHumanoidIdentity(`String` species, `Maybe<uint64_t>` seed, `Maybe<String>` gender)

Generates a random humanoid identity for the given species. Optionally supply a seed and or predetermined gender.

Additionaly supplies a humanoidParameters object, as well as an object of the clothing that would be chosen with those choices

humanoidParameters will have the `choices` value set to an array of the indexes of the choices made.

---

#### `LuaTupleReturn<Json, JsonObject, JsonObject>` root.createHumanoid(`String` name, `String` speciesChoice, `size_t` genderChoice, `size_t` bodyColorChoice, `size_t` alty, `size_t` hairChoice, `size_t` heady, `size_t` shirtChoice, `size_t` shirtColor, `size_t` pantsChoice, `size_t` pantsColor, `size_t` personality, `LuaVariadic<LuaValue>` ext)

Generates a the identity for the given choices on the character creation screen for the given species.

Additionaly supplies a humanoidParameters object, as well as an object of the clothing that would be chosen with those choices

humanoidParameters will have the `choices` value set to an array of the indexes of the choices made.

Additional arguments **ext** are not used by the retail character creation, but will be passed to a species' character creation script if it is defined.

---

#### `JsonObject` root.effectConfig(`String` effect)

Returns the config for the status effect.

---

#### `Json` root.materialConfig(`MaterialId` materialId)

Returns a JSON object containing the `path` and base `config` for the specified material by its ID if it is a real material, or `nil` if it is a metamaterial or invalid.

---

#### `Json` root.modConfig(`ModId` modId)

Returns a JSON object containing the `path` and base `config` for the specified mod by its ID if it is a real mod, or `nil` if it is a metamod or invalid.

---

#### `Json` root.monsterConfig(`String` typeName)

Returns the base config for the monsterType.

---

#### `Json` root.biomeConfig(`String` typeName)

Returns a json object of the config, name, and path of the given biome.

---

#### `Json` root.treeFoliageConfig(`String` typeName)

Returns a json object of the config, and path for the given foliage.

---

#### `Json` root.treeStemConfig(`String` typeName)

Returns a json object of the config, and path for the given stem.
