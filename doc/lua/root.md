The `root` table contains functions that reference the game's currently loaded assets and don't relate to any more specific context such as a particular world or universe.

---

#### `Json` root.assetJson(`String` assetPath)

Returns the contents of the specified JSON asset file.

---

#### `Json` root.makeCurrentVersionedJson(`String` versioningIdentifier, `Json` content)

Returns a versioned JSON representation of the given JSON content with the given identifier and the most recent version as specified in `versioning.config`.

---

#### `Json` root.loadVersionedJson(`Json` versionedContent, `String` versioningIdentifier)

Returns the given JSON content and identifier after applying appropriate versioning scripts to bring it up to the most recent version as specified in `versioning.config`.

---

#### `double` root.evalFunction(`String` functionName, `double` input)

Returns the evaluation of the specified univariate function (as defined in a `.functions` file) for the given input value.

---

#### `double` root.evalFunction2(`String` functionName, `double` input1, `double` input2)

Returns the evaluation of the specified bivariate function (as defined in a `.2functions` file) for the given input values.

---

#### `Vec2U` root.imageSize(`String` imagePath)

Returns the pixel dimensions of the specified image asset.

---

#### `List<Vec2I>` root.imageSpaces(`String` imagePath, `Vec2F` worldPosition, `float` spaceScan, `bool` flip)

Returns a list of the world tile spaces the image would occupy if placed at the given position using the specified spaceScan value (the portion of a space that must be non-transparent for that space to count as filled).

---

#### `RectU` root.nonEmptyRegion(`String` imagePath)

Returns the rectangle containing the portion of the specified asset image that is non-transparent.

---

#### `Json` root.npcConfig(`String` npcType)

Returns a representation of the generated JSON configuration for an NPC of the given type.

---

#### `Json` root.npcVariant(`String` species, `String` npcType, `float` level, [`unsigned` seed], [`Json` parameters])

Generates an NPC with the specified species, type, level, seed and parameters, and returns its configuration.

---

#### `float` root.projectileGravityMultiplier(`String` projectileName)

Returns the gravity multiplier of the given projectile's movement controller configuration as configured in `physics.config`.

---

#### `Json` root.projectileConfig(`String` projectileName)

Returns a representation of the JSON configuration for the given projectile.

---

#### `Json` root.itemDescriptorsMatch(`ItemDescriptor` descriptor1, `ItemDescriptor` descriptor2, [`bool` exactMatch])

Returns `true` if the given item descriptors match. If exactMatch is `true` then both names and parameters will be compared, otherwise only names.

---

#### `JsonArray` root.recipesForItem(`String` itemName)

Returns a list of JSON configurations of all recipes which output the given item.

---

#### `String` root.itemType(`String` itemName)

Returns the item type name for the specified item.

---

#### `JsonArray` root.itemTags(`String` itemName)

Returns a list of the tags applied to the specified item.

---

#### `bool` root.itemHasTag(`String` itemName, `String` tagName)

Returns true if the given item's tags include the specified tag and false otherwise.

---

#### `Json` root.itemConfig(`ItemDescriptor` descriptor, [`float` level], [`unsigned` seed])

Generates an item from the specified descriptor, level and seed and returns a JSON object containing the `directory`, `config` and `parameters` for that item.

---

#### `ItemDescriptor` root.createItem(`ItemDescriptor` descriptor, [`float` level], [`unsigned` seed])

Generates an item from the specified descriptor, level and seed and returns a new item descriptor for the resulting item.

---

#### `Json` root.tenantConfig(`String` tenantName)

Returns the JSON configuration for the given tenant.

---

#### `JsonArray` root.getMatchingTenants(`map<String, unsigned>` colonyTags)

Returns an array of JSON configurations of tenants matching the given map of colony tags and corresponding object counts.

---

#### `JsonArray` root.liquidStatusEffects(`LiquidId` liquid)

Returns an array of status effects applied by the given liquid.

---

#### `String` root.generateName(`String` assetPath, [`unsigned` seed])

Returns a randomly generated name using the specified name gen config and seed.

---

#### `Json` root.questConfig(`String` questTemplateId)

Returns the JSON configuration of the specified quest template.

---

#### `JsonArray` root.npcPortrait(`String` portraitMode, `String` species, `String` npcType, `float` level, [`unsigned` seed], [`Json` parameters])

Generates an NPC with the specified type, level, seed and parameters and returns a portrait in the given portraitMode as a list of drawables.

---

#### `JsonArray` root.monsterPortrait(`String` typeName, [`Json` parameters])

Generates a monster of the given type with the given parameters and returns its portrait as a list of drawables.

---

#### `bool` root.isTreasurePool(`String` poolName)

Returns true if the given treasure pool exists and false otherwise. Can be used to guard against errors attempting to generate invalid treasure.

---

#### `JsonArray` root.createTreasure(`String` poolName, `float` level, [`unsigned` seed])

Generates an instance of the specified treasure pool, level and seed and returns the contents as a list of item descriptors.

---

#### `String` root.materialMiningSound(`String` materialName, [`String` modName])

Returns the path of the mining sound asset for the given material and mod combination, or `nil` if no mining sound is set.

---

#### `String` root.materialFootstepSound(`String` materialName, [`String` modName])

Returns the path of the footstep sound asset for the given material and mod combination, or `nil` if no footstep sound is set.

---

#### `float` root.materialHealth(`String` materialName)

Returns the configured health value for the specified material.

---

#### `Json` root.materialConfig(`String` materialName)

Returns a JSON object containing the `path` and base `config` for the specified material if it is a real material, or `nil` if it is a metamaterial or invalid.

---

#### `Json` root.modConfig(`String` modName)

Returns a JSON object containing the `path` and base `config` for the specified mod if it is a real mod, or `nil` if it is a metamod or invalid.

---

#### `Json` root.liquidConfig(`LiquidId` liquidId)

#### `Json` root.liquidConfig(`String` liquidName)

Returns a JSON object containing the `path` and base `config` for the specified liquid name or id if it is a real liquid, or `nil` if the liquid is empty or invalid.

---

#### `String` root.liquidName(`LiquidId` liquidId)

Returns the string name of the liquid with the given ID.

---

#### `LiquidId` root.liquidId(`String` liquidName)

Returns the numeric ID of the liquid with the given name.

---

#### `Json` root.monsterSkillParameter(`String` skillName, `String` parameterName)

Returns the value of the specified parameter for the specified monster skill.

---

#### `Json` root.monsterParameters(`String` monsterType, [uint64_t seed])

Returns the parameters for a monster type.

---

#### `ActorMovementParameters` root.monsterMovementSettings(`String` monsterType, [uint64_t seed])

Returns the configured base movement parameters for the specified monster type.

---

#### `Json` root.createBiome(`String` biomeName, `unsigned` seed, `float` verticalMidPoint, `float` threatLevel)

Generates a biome with the specified name, seed, vertical midpoint and threat level, and returns a JSON object containing the configuration for the generated biome.

---

#### `String` root.hasTech(`String` techName)

Returns `true` if a tech with the specified name exists and `false` otherwise.

---

#### `String` root.techType(`String` techName)

Returns the type (tech slot) of the specified tech.

---

#### `Json` root.techConfig(`String` techName)

Returns the JSON configuration for the specified tech.

---

#### `String` root.treeStemDirectory(`String` stemName)

Returns the path within assets from which the specified tree stem type was loaded.

---

#### `String` root.treeFoliageDirectory(`String` foliageName)

Returns the path within assets from which the specified tree foliage type was loaded.

---

#### `Collection` root.collection(`String` collectionName)

Returns the metadata for the specified collection.

---

#### `List<Collectable>` root.collectables(`String` collectionName)

Returns a list of collectables for the specified collection.

---

#### `String` root.elementalResistance(`String` elementalType)

Returns the name of the stat used to calculate elemental resistance for the specified elemental type.

---

#### `Json` root.dungeonMetadata(`String` dungeonName)

Returns the metadata for the specified dungeon definition.

---

#### `BehaviorState` root.behavior(`LuaTable` context, `Json` config, `JsonObject` parameters)

Loads a behavior and returns the behavior state as userdata.

context is the current lua context called from, in almost all cases _ENV.

config can be either the `String` name of a behavior tree, or an entire behavior tree configuration to be built.

parameters is overrides for parameters for the behavior tree.

BehaviorState contains 2 methods:

behavior:init(_ENV) -- initializes the behavior, loads required scripts, and returns a new behavior state
behavior:run(state, dt) -- runs the behavior, takes a behavior state for the first argument
behavior:clear(state) -- resets the internal state of the behavior

Example:

```lua
function init()
  self.behavior = root.behavior("monster", {})
  self.behaviorState = self.behavior:init(_ENV)
end

function update(dt)
  self.behavior:run(self.behaviorState, dt)
end
```
