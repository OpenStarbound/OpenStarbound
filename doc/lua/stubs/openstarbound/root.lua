---@meta

--- root API
---@class root
root = {}

--- Returns an array containing all assets with the specified file extension. By the way, here's a list of every file extension the game does Special Things™ for when loading assets. - Items: `item`, `liqitem`, `matitem`, `miningtool`, `flashlight`, `wiretool`, `beamaxe`, `tillingtool`, `painttool`, `harvestingtool`, `head`, `chest`, `legs`, `back`, `currency`, `consumable`, `blueprint`, `inspectiontool`, `instrument`, `thrownitem`, `unlock`, `activeitem`, `augment` - Materials: `material`, `matmod` - Liquids: `liquid` - NPCs: `npctype` - Tenants: `tenant` - Objects: `object` - Vehicles: `vehicle` - Monsters: `monstertype`, `monsterpart`, `monsterskill`, `monstercolors` - Plants: `modularstem`, `modularfoliage`, `grass`, `bush` - Projectiles: `projectile` - Particles: `particle` - Name Gen: `namesource` - AI Missions: `aimission` - Quests: `questtemplate` - Radio Messages: `radiomessages` - Spawn Types: `spawntypes` - Species: `species` - Stagehand: `stagehand` - Behaviors: `nodes`, `behavior` - Biomes: `biome`, `weather` - Terrain: `terrain` - Treasure: `treasurepools`, `treasurechests` - Codex Entries: `codex` - Collections: `collection` - Statistics: `event`, `achievement` - Status Effects: `statuseffect` - Functions: `functions`, `2functions`, `configfunctions` - Tech: `tech` - Damage: `damage` - Dances: `dance` - Effect Sources: `effectsource` - Command Macros: `macros` - Recipes: `recipe` ---
---@param extension string
---@return string[]
function root.assetsByExtension(extension) end

--- Returns the raw data of an asset. ---
---@param path string
---@return string
function root.assetData(path) end

--- Returns the asset source path of an asset, or nil if the asset doesn't exist. If you specify getPatches as true then also returns the patches for the asset as an array, each element containing the source path and patch path in indexes 1 and 2 respectively. ---
---@param path string
---@param getPatches boolean
---@return string, Maybe<LuaTable>
function root.assetOrigin(path, getPatches) end

--- Without metadata: Returns an array with all the asset source paths. With metadata: Returns a table, key/value being source path/metadata. ---
---@param withMetadata boolean
---@return LuaTable
function root.assetSourcePaths(withMetadata) end

--- Returns the metadata of an asset source. ---
---@param path string
---@return Json
function root.assetSourceMetadata(path) end

--- Returns an image. ---
---@param image string
---@return Image
function root.assetImage(image) end

--- Returns an array containing a `file` (the frames file used for the image) and `frames` (the frame data of the image). ---
---@param imagePath string
---@return Json
function root.assetFrames(imagePath) end

--- Returns a list of asset sources which patch the specified asset and the paths to those patches. ---
---@param asset string
---@return JsonArray
function root.assetPatches(asset) end

--- Gets a configuration value in `/storage/starbound.config`. ---
---@param key string
---@return Json
function root.getConfiguration(key) end

--- Gets a configuration value in `/storage/starbound.config` by path. *Both getters will error if you try to get `title`, as that can contain the player's saved server login.* ---
---@param path string
---@return Json
function root.getConfigurationPath(path) end

--- Sets a configuration value in `/storage/starbound.config`. ---
---@param key string
---@param value Json
---@return Json
function root.setConfiguration(key, value) end

--- Sets a configuration value in `/storage/starbound.config` by path. *Both setters will error if you try to set `safeScripts`, as that can break Starbound's sandbox.* ---
---@param path string
---@param value Json
---@return Json
function root.setConfigurationPath(path, value) end

--- Returns all recipes. ---
---@return JsonArray
function root.allRecipes() end

--- Returns the asset file path for the specified item. ---
---@param itemName string
---@return string
function root.itemFile(itemName) end

--- Returns the config for the species. ---
---@param species string
---@return JsonObject
function root.speciesConfig(species) end

--- Generates a random humanoid identity for the given species. Optionally supply a seed and or predetermined gender. Additionaly supplies a humanoidParameters object, as well as an object of the clothing that would be chosen with those choices humanoidParameters will have the `choices` value set to an array of the indexes of the choices made. ---
---@param species string
---@param seed Maybe<uint64_t>
---@param gender Maybe<String>
---@return LuaTupleReturn<Json, JsonObject, JsonObject>
function root.generateHumanoidIdentity(species, seed, gender) end

--- Generates a the identity for the given choices on the character creation screen for the given species. Additionaly supplies a humanoidParameters object, as well as an object of the clothing that would be chosen with those choices humanoidParameters will have the `choices` value set to an array of the indexes of the choices made. Additional arguments **ext** are not used by the retail character creation, but will be passed to a species' character creation script if it is defined. ---
---@param name string
---@param speciesChoice string
---@param genderChoice size_t
---@param bodyColorChoice size_t
---@param alty size_t
---@param hairChoice size_t
---@param heady size_t
---@param shirtChoice size_t
---@param shirtColor size_t
---@param pantsChoice size_t
---@param pantsColor size_t
---@param personality size_t
---@param ext LuaVariadic<LuaValue>
---@return LuaTupleReturn<Json, JsonObject, JsonObject>
function root.createHumanoid(name, speciesChoice, genderChoice, bodyColorChoice, alty, hairChoice, heady, shirtChoice, shirtColor, pantsChoice, pantsColor, personality, ext) end

--- Returns the config for the status effect.
---@param effect string
---@return JsonObject
function root.effectConfig(effect) end
