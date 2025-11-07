---@meta

--- root API
---@class root
root = {}

--- Returns the contents of the specified JSON asset file. ---
---@param assetPath string
---@return Json
function root.assetJson(assetPath) end

--- Returns a versioned JSON representation of the given JSON content with the given identifier and the most recent version as specified in `versioning.config`. ---
---@param versioningIdentifier string
---@param content Json
---@return Json
function root.makeCurrentVersionedJson(versioningIdentifier, content) end

--- Returns the given JSON content and identifier after applying appropriate versioning scripts to bring it up to the most recent version as specified in `versioning.config`. ---
---@param versionedContent Json
---@param versioningIdentifier string
---@return Json
function root.loadVersionedJson(versionedContent, versioningIdentifier) end

--- Returns the evaluation of the specified univariate function (as defined in a `.functions` file) for the given input value. ---
---@param functionName string
---@param input number
---@return number
function root.evalFunction(functionName, input) end

--- Returns the evaluation of the specified bivariate function (as defined in a `.2functions` file) for the given input values. ---
---@param functionName string
---@param input1 number
---@param input2 number
---@return number
function root.evalFunction2(functionName, input1, input2) end

--- Returns the pixel dimensions of the specified image asset. ---
---@param imagePath string
---@return Vec2U
function root.imageSize(imagePath) end

--- Returns a list of the world tile spaces the image would occupy if placed at the given position using the specified spaceScan value (the portion of a space that must be non-transparent for that space to count as filled). ---
---@param imagePath string
---@param worldPosition Vec2F
---@param spaceScan number
---@param flip boolean
---@return List<Vec2I>
function root.imageSpaces(imagePath, worldPosition, spaceScan, flip) end

--- Returns the rectangle containing the portion of the specified asset image that is non-transparent. ---
---@param imagePath string
---@return RectU
function root.nonEmptyRegion(imagePath) end

--- Returns a representation of the generated JSON configuration for an NPC of the given type. ---
---@param npcType string
---@return Json
function root.npcConfig(npcType) end

--- Generates an NPC with the specified species, type, level, seed and parameters, and returns its configuration. ---
---@param species string
---@param npcType string
---@param level number
---@param seed unsigned
---@param parameters Json
---@return Json
function root.npcVariant(species, npcType, level, seed, parameters) end

--- Returns the gravity multiplier of the given projectile's movement controller configuration as configured in `physics.config`. ---
---@param projectileName string
---@return number
function root.projectileGravityMultiplier(projectileName) end

--- Returns a representation of the JSON configuration for the given projectile. ---
---@param projectileName string
---@return Json
function root.projectileConfig(projectileName) end

--- Returns `true` if the given item descriptors match. If exactMatch is `true` then both names and parameters will be compared, otherwise only names. ---
---@param descriptor1 ItemDescriptor
---@param descriptor2 ItemDescriptor
---@param exactMatch boolean
---@return Json
function root.itemDescriptorsMatch(descriptor1, descriptor2, exactMatch) end

--- Returns a list of JSON configurations of all recipes which output the given item. ---
---@param itemName string
---@return JsonArray
function root.recipesForItem(itemName) end

--- Returns the item type name for the specified item. ---
---@param itemName string
---@return string
function root.itemType(itemName) end

--- Returns a list of the tags applied to the specified item. ---
---@param itemName string
---@return JsonArray
function root.itemTags(itemName) end

--- Returns true if the given item's tags include the specified tag and false otherwise. ---
---@param itemName string
---@param tagName string
---@return boolean
function root.itemHasTag(itemName, tagName) end

--- Generates an item from the specified descriptor, level and seed and returns a JSON object containing the `directory`, `config` and `parameters` for that item. ---
---@param descriptor ItemDescriptor
---@param level number
---@param seed unsigned
---@return Json
function root.itemConfig(descriptor, level, seed) end

--- Generates an item from the specified descriptor, level and seed and returns a new item descriptor for the resulting item. ---
---@param descriptor ItemDescriptor
---@param level number
---@param seed unsigned
---@return ItemDescriptor
function root.createItem(descriptor, level, seed) end

--- Returns the JSON configuration for the given tenant. ---
---@param tenantName string
---@return Json
function root.tenantConfig(tenantName) end

--- Returns an array of JSON configurations of tenants matching the given map of colony tags and corresponding object counts. ---
---@return JsonArray
function root.getMatchingTenants() end

--- Returns an array of status effects applied by the given liquid. ---
---@param liquid LiquidId
---@return JsonArray
function root.liquidStatusEffects(liquid) end

--- Returns a randomly generated name using the specified name gen config and seed. ---
---@param assetPath string
---@param seed unsigned
---@return string
function root.generateName(assetPath, seed) end

--- Returns the JSON configuration of the specified quest template. ---
---@param questTemplateId string
---@return Json
function root.questConfig(questTemplateId) end

--- Generates an NPC with the specified type, level, seed and parameters and returns a portrait in the given portraitMode as a list of drawables. ---
---@param portraitMode string
---@param species string
---@param npcType string
---@param level number
---@param seed unsigned
---@param parameters Json
---@return JsonArray
function root.npcPortrait(portraitMode, species, npcType, level, seed, parameters) end

--- Generates a monster of the given type with the given parameters and returns its portrait as a list of drawables. ---
---@param typeName string
---@param parameters Json
---@return JsonArray
function root.monsterPortrait(typeName, parameters) end

--- Returns true if the given treasure pool exists and false otherwise. Can be used to guard against errors attempting to generate invalid treasure. ---
---@param poolName string
---@return boolean
function root.isTreasurePool(poolName) end

--- Generates an instance of the specified treasure pool, level and seed and returns the contents as a list of item descriptors. ---
---@param poolName string
---@param level number
---@param seed unsigned
---@return JsonArray
function root.createTreasure(poolName, level, seed) end

--- Returns the path of the mining sound asset for the given material and mod combination, or `nil` if no mining sound is set. ---
---@param materialName string
---@param modName string
---@return string
function root.materialMiningSound(materialName, modName) end

--- Returns the path of the footstep sound asset for the given material and mod combination, or `nil` if no footstep sound is set. ---
---@param materialName string
---@param modName string
---@return string
function root.materialFootstepSound(materialName, modName) end

--- Returns the configured health value for the specified material. ---
---@param materialName string
---@return number
function root.materialHealth(materialName) end

--- Returns a JSON object containing the `path` and base `config` for the specified material if it is a real material, or `nil` if it is a metamaterial or invalid. ---
---@param materialName string
---@return Json
function root.materialConfig(materialName) end

--- Returns a JSON object containing the `path` and base `config` for the specified mod if it is a real mod, or `nil` if it is a metamod or invalid. ---
---@param modName string
---@return Json
function root.modConfig(modName) end

---@param liquidId LiquidId
---@return Json
function root.liquidConfig(liquidId) end

--- Returns a JSON object containing the `path` and base `config` for the specified liquid name or id if it is a real liquid, or `nil` if the liquid is empty or invalid. ---
---@param liquidName string
---@return Json
function root.liquidConfig(liquidName) end

--- Returns the string name of the liquid with the given ID. ---
---@param liquidId LiquidId
---@return string
function root.liquidName(liquidId) end

--- Returns the numeric ID of the liquid with the given name. ---
---@param liquidName string
---@return LiquidId
function root.liquidId(liquidName) end

--- Returns the value of the specified parameter for the specified monster skill. ---
---@param skillName string
---@param parameterName string
---@return Json
function root.monsterSkillParameter(skillName, parameterName) end

--- Returns the parameters for a monster type. ---
---@param monsterType string
---@return Json
function root.monsterParameters(monsterType) end

--- Returns the configured base movement parameters for the specified monster type. ---
---@param monsterType string
---@return ActorMovementParameters
function root.monsterMovementSettings(monsterType) end

--- Generates a biome with the specified name, seed, vertical midpoint and threat level, and returns a JSON object containing the configuration for the generated biome. ---
---@param biomeName string
---@param seed unsigned
---@param verticalMidPoint number
---@param threatLevel number
---@return Json
function root.createBiome(biomeName, seed, verticalMidPoint, threatLevel) end

--- Returns `true` if a tech with the specified name exists and `false` otherwise. ---
---@param techName string
---@return string
function root.hasTech(techName) end

--- Returns the type (tech slot) of the specified tech. ---
---@param techName string
---@return string
function root.techType(techName) end

--- Returns the JSON configuration for the specified tech. ---
---@param techName string
---@return Json
function root.techConfig(techName) end

--- Returns the path within assets from which the specified tree stem type was loaded. ---
---@param stemName string
---@return string
function root.treeStemDirectory(stemName) end

--- Returns the path within assets from which the specified tree foliage type was loaded. ---
---@param foliageName string
---@return string
function root.treeFoliageDirectory(foliageName) end

--- Returns the metadata for the specified collection. ---
---@param collectionName string
---@return Collection
function root.collection(collectionName) end

--- Returns a list of collectables for the specified collection. ---
---@param collectionName string
---@return List<Collectable>
function root.collectables(collectionName) end

--- Returns the name of the stat used to calculate elemental resistance for the specified elemental type. ---
---@param elementalType string
---@return string
function root.elementalResistance(elementalType) end

--- Returns the metadata for the specified dungeon definition. ---
---@param dungeonName string
---@return Json
function root.dungeonMetadata(dungeonName) end

--- Loads a behavior and returns the behavior state as userdata. context is the current lua context called from, in almost all cases _ENV. config can be either the `String` name of a behavior tree, or an entire behavior tree configuration to be built. parameters is overrides for parameters for the behavior tree. BehaviorState contains 2 methods: behavior:init(_ENV) -- initializes the behavior, loads required scripts, and returns a new behavior state behavior:run(state, dt) -- runs the behavior, takes a behavior state for the first argument behavior:clear(state) -- resets the internal state of the behavior Example: ```lua function init() self.behavior = root.behavior("monster", {}) self.behaviorState = self.behavior:init(_ENV) end function update(dt) self.behavior:run(self.behaviorState, dt) end ```
---@param context LuaTable
---@param config Json
---@param parameters JsonObject
---@return BehaviorState
function root.behavior(context, config, parameters) end
