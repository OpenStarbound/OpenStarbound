---@meta

--- world API
---@class world
world = {}

--- Returns a string describing the world's type. For terrestrial worlds this will be the primary biome, for instance worlds this will be the instance name, and for ship or generic worlds this will be 'unknown'. ---
---@return string
function world.type() end

--- Returns a `true` if the current world is a terrestrial world, i.e. a planet, and `false` otherwise. ---
---@return boolean
function world.terrestrial() end

--- Returns a vector describing the size of the current world. ---
---@return Vec2I
function world.size() end

--- Returns the magnitude of the distance between the specified world positions. Use this rather than simple vector subtraction to handle world wrapping. ---
---@param position1 Vec2F
---@param position2 Vec2F
---@return number
function world.magnitude(position1, position2) end

--- Returns the vector difference between the specified world positions. Use this rather than simple vector subtraction to handle world wrapping. ---
---@param position1 Vec2F
---@param position2 Vec2F
---@return Vec2F
function world.distance(position1, position2) end

--- Returns `true` if the specified poly contains the specified position in world space and `false` otherwise. ---
---@param poly PolyF
---@param position Vec2F
---@return boolean
function world.polyContains(poly, position) end

--- Returns the specified position with its X coordinate wrapped around the world width.
---@param position Vec2F
---@return Vec2F
function world.xwrap(position) end

--- Returns the specified X position wrapped around the world width. ---
---@param xPosition number
---@return number
function world.xwrap(xPosition) end

--- Returns the point nearest to (i.e. on the same side of the world as) the source point. Either argument can be specified as a `Vec2F` point or as a `float` X position. The type of the targetPosition determines the return type. ---
---@return Variant<Vec2F, float>
function world.nearestTo() end

--- Returns `true` if the generated collision geometry at the specified point matches any of the specified collision kinds and `false` otherwise. ---
---@param point Vec2F
---@param collisionKinds CollisionSet
---@return boolean
function world.pointCollision(point, collisionKinds) end

--- Returns `true` if the tile at the specified point matches any of the specified collision kinds and `false` otherwise. ---
---@param point Vec2F
---@param collisionKinds CollisionSet
---@return boolean
function world.pointTileCollision(point, collisionKinds) end

--- If the line between the specified points overlaps any generated collision geometry of the specified collision kinds, returns the point at which the line collides, or `nil` if the line does not collide. If intersecting a side of the poly, also returns the normal of the intersected side as a second return. ---
---@param startPoint Vec2F
---@param endPoint Vec2F
---@param collisionKinds CollisionSet
---@return Tuple<Maybe<Vec2F>, Maybe<Vec2F>>
function world.lineCollision(startPoint, endPoint, collisionKinds) end

--- Returns `true` if the line between the specified points overlaps any tiles of the specified collision kinds and `false` otherwise. ---
---@param startPoint Vec2F
---@param endPoint Vec2F
---@param collisionKinds CollisionSet
---@return boolean
function world.lineTileCollision(startPoint, endPoint, collisionKinds) end

--- Returns a table of {`position`, `normal`} where `position` is the position that the line intersects the first collidable tile, and `normal` is the collision normal. Returns `nil` if no tile is intersected. ---
---@param startPoint Vec2F
---@param endPoint Vec2F
---@param collisionKinds CollisionSet
---@return Maybe<pair<Vec2F, Vec2F>>
function world.lineTileCollisionPoint(startPoint, endPoint, collisionKinds) end

--- Returns `true` if the specified rectangle overlaps any generated collision geometry of the specified collision kinds and `false` otherwise. ---
---@param rect RectF
---@param collisionKinds CollisionSet
---@return boolean
function world.rectCollision(rect, collisionKinds) end

--- Returns `true` if the specified rectangle overlaps any tiles of the specified collision kinds and `false` otherwise. ---
---@param rect RectF
---@param collisionKinds CollisionSet
---@return boolean
function world.rectTileCollision(rect, collisionKinds) end

--- Returns `true` if the specified polygon overlaps any generated collision geometry of the specified collision kinds and `false` otherwise. If a position is specified, the polygon coordinates will be treated as relative to that world position. ---
---@param poly PolyF
---@param position Vec2F
---@param collisionKinds CollisionSet
---@return boolean
function world.polyCollision(poly, position, collisionKinds) end

--- Returns an ordered list of tile positions along the line between the specified points that match any of the specified collision kinds. If maxReturnCount is specified, the function will only return up to that number of points. ---
---@param startPoint Vec2F
---@param endPoint Vec2F
---@param collisionKinds CollisionSet
---@param maxReturnCount number
---@return List<Vec2I>
function world.collisionBlocksAlongLine(startPoint, endPoint, collisionKinds, maxReturnCount) end

--- Returns a list of pairs containing a position and a `LiquidLevel` for all tiles along the line between the specified points that contain any liquid. ---
---@param startPoint Vec2F
---@param endPoint Vec2F
---@return List<pair<Vec2I, LiquidLevel>>
function world.liquidAlongLine(startPoint, endPoint) end

--- Attempts to move the specified poly (relative to the specified position) such that it does not collide with any of the specified collision kinds. Will only move the poly up to the distance specified by maximumCorrection. Returns `nil` if the collision resolution fails. ---
---@param poly PolyF
---@param position Vec2F
---@param maximumCorrection number
---@param collisionKinds CollisionSet
---@return Vec2F
function world.resolvePolyCollision(poly, position, maximumCorrection, collisionKinds) end

--- Returns `true` if the specified tile position is occupied by a material or tile entity and `false` if it is empty. The check will be performed on the foreground tile layer if foregroundLayer is `true` (or unspecified) and the background tile layer if it is `false`. The check will include ephemeral tile entities such as preview objects if includeEphemeral is `true`, and will not include these entities if it is `false` (or unspecified). ---
---@param tilePosition Vec2I
---@param foregroundLayer boolean
---@param includeEphemeral boolean
---@return boolean
function world.tileIsOccupied(tilePosition, foregroundLayer, includeEphemeral) end

--- Attempts to place the specified object into the world at the specified position, preferring it to be right-facing if direction is positive (or unspecified) and left-facing if it is negative. If parameters are specified they will be applied to the object. Returns `true` if the object is placed successfully and `false` otherwise. ---
---@param objectName string
---@param tilePosition Vec2I
---@param direction number
---@param parameters Json
---@return boolean
function world.placeObject(objectName, tilePosition, direction, parameters) end

--- Attempts to spawn the specified item into the world as the specified position. If item is specified as a name, it will optionally apply the specified count and parameters. The item drop entity can also be spawned with an initial velocity and intangible time (delay before it can be picked up) if specified. Returns an `EntityId` of the item drop if successful and `nil` otherwise. ---
---@param item ItemDescriptor
---@param position Vec2F
---@param count unsigned
---@param parameters Json
---@param velocity Vec2F
---@param intangibleTime number
---@return EntityId
function world.spawnItem(item, position, count, parameters, velocity, intangibleTime) end

--- Attempts to spawn all items in an instance of the specified treasure pool with the specified level and seed at the specified world position. Returns a list of `EntityId`s of the item drops created if successful and `nil` otherwise. ---
---@param position Vec2F
---@param poolName string
---@param level number
---@param seed unsigned
---@return List<EntityId>
function world.spawnTreasure(position, poolName, level, seed) end

--- Attempts to spawn a monster of the specified type at the specified position. If parameters are specified they will be applied to the spawned monster. If they are unspecified, they default to an object setting aggressive to be randomly `true` or `false`. Level for the monster may be specified in parameters. Returns the `EntityId` of the spawned monster if successful and `nil` otherwise. ---
---@param monsterType string
---@param position Vec2F
---@param parameters Json
---@return EntityId
function world.spawnMonster(monsterType, position, parameters) end

--- Attempts to spawn an NPC of the specified type, species, level with the specified seed and parameters at the specified position. Returns `EntityId` of the spawned NPC if successful and `nil` otherwise. ---
---@param position Vec2F
---@param species string
---@param npcType string
---@param level number
---@param seed unsigned
---@param parameters Json
---@return EntityId
function world.spawnNpc(position, species, npcType, level, seed, parameters) end

--- Attempts to spawn a stagehand of the specified type at the specified position with the specified override parameters. Returns `EntityId` of the spawned stagehand if successful and `nil` otherwise. ---
---@param position Vec2F
---@param type string
---@param overrides Json
---@return EntityId
function world.spawnStagehand(position, type, overrides) end

--- Attempts to spawn a projectile of the specified type at the specified position with the specified source entity id, direction, and parameters. If trackSourceEntity is `true` then the projectile's position will be locked relative to its source entity's position. Returns the `EntityId` of the spawned projectile if successful and `nil` otherwise. ---
---@param projectileName string
---@param position Vec2F
---@param sourceEntityId EntityId
---@param direction Vec2F
---@param trackSourceEntity boolean
---@param parameters Json
---@return EntityId
function world.spawnProjectile(projectileName, position, sourceEntityId, direction, trackSourceEntity, parameters) end

--- Attempts to spawn a vehicle of the specified type at the specified position with the specified override parameters. Returns the `EntityId` of the spawned vehicle if successful and `nil` otherwise. ---
---@param vehicleName string
---@param position Vec2F
---@param overrides Json
---@return EntityId
function world.spawnVehicle(vehicleName, position, overrides) end

--- Returns the threat level of the current world. ---
---@return number
function world.threatLevel() end

--- Returns the absolute time of the current world. ---
---@return number
function world.time() end

--- Returns the absolute numerical day of the current world. ---
---@return unsigned
function world.day() end

--- Returns a value between 0 and 1 indicating the time within the day of the current world. ---
---@return number
function world.timeOfDay() end

--- Returns the duration of a day on the current world. ---
---@return number
function world.dayLength() end

--- Returns the JSON value of the specified world property, or defaultValue or `nil` if it is not set. ---
---@param propertyName string
---@param defaultValue Json
---@return Json
function world.getProperty(propertyName, defaultValue) end

--- Sets the specified world property to the specified value. ---
---@param propertyName string
---@param value Json
---@return void
function world.setProperty(propertyName, value) end

--- Returns the `LiquidLevel` at the specified tile position, or `nil` if there is no liquid.
---@param position Vec2I
---@return LiquidLevel
function world.liquidAt(position) end

--- Returns the average `LiquidLevel` of the most plentiful liquid within the specified region, or `nil` if there is no liquid. ---
---@param region RectF
---@return LiquidLevel
function world.liquidAt(region) end

--- Returns the gravity at the specified position. This should be consistent for all non-dungeon tiles in a world but can be altered by dungeons. ---
---@param position Vec2F
---@return number
function world.gravity(position) end

--- Attempts to place the specified quantity of the specified liquid at the specified position. Returns `true` if successful and `false` otherwise. ---
---@param position Vec2F
---@param liquid LiquidId
---@param quantity number
---@return boolean
function world.spawnLiquid(position, liquid, quantity) end

--- Removes any liquid at the specified position and returns the LiquidLevel containing the type and quantity of liquid removed, or `nil` if no liquid is removed. ---
---@param position Vec2F
---@return LiquidLevel
function world.destroyLiquid(position) end

--- Returns `true` if the tile at the specified position is protected and `false` otherwise. ---
---@param position Vec2F
---@return boolean
function world.isTileProtected(position) end

--- Attempts to synchronously pathfind between the specified positions using the specified movement and pathfinding parameters. Returns the path as a list of nodes as successful, or `nil` if no path is found. ---
---@param startPosition Vec2F
---@param endPosition Vec2F
---@param movementParameters ActorMovementParameters
---@param searchParameters PlatformerAStar::Parameters
---@return PlatformerAStar::Path
function world.findPlatformerPath(startPosition, endPosition, movementParameters, searchParameters) end

--- Creates and returns a Lua UserData value which can be used for pathfinding over multiple frames. The `PathFinder` returned has the following two methods:
---@param startPosition Vec2F
---@param endPosition Vec2F
---@param movementParameters ActorMovementParameters
---@param searchParameters PlatformerAStar::Parameters
---@return PlatformerAStar::PathFinder
function world.platformerPathStart(startPosition, endPosition, movementParameters, searchParameters) end

--- Returns the current logical light level at the specified position. Requires recalculation of lighting, so this should be used sparingly. ---
---@param position Vec2F
---@return number
function world.lightLevel(position) end

--- Returns the current wind level at the specified position. ---
---@param position Vec2F
---@return number
function world.windLevel(position) end

--- Returns `true` if the world is breathable at the specified position and `false` otherwise. ---
---@param position Vec2F
---@return boolean
function world.breathable(position) end

--- Returns a list of the environmental status effects at the specified position. ---
---@param position Vec2F
---@return List<String>
function world.environmentStatusEffects(position) end

--- Returns `true` if the specified position is below the world's surface level and `false` otherwise. ---
---@param position Vec2F
---@return boolean
function world.underground(position) end

--- Returns `true` if the world is terrestrial and the specified position is within its surface layer, and `false` otherwise.
---@param position Vec2I
---@return boolean
function world.inSurfaceLayer(position) end

--- Returns the surface layer base height. ---
---@return number
function world.surfaceLevel() end

--- If the specified position is within a region that has ocean (endless) liquid, returns the world Y level of that ocean's surface, or 0 if there is no ocean in the specified region. ---
---@param position Vec2I
---@return number
function world.oceanLevel(position) end

--- Returns the name of the material at the specified position and layer. Layer can be specified as 'foreground' or 'background'. Returns `false` if the space is empty in that layer. Returns `nil` if the material is NullMaterial (e.g. if the position is in an unloaded sector). ---
---@param position Vec2F
---@param layerName string
---@return Variant<String, bool>
function world.material(position, layerName) end

--- Returns the name of the mod at the specified position and layer, or `nil` if there is no mod. ---
---@param position Vec2F
---@param layerName string
---@return string
function world.mod(position, layerName) end

--- Returns the hue shift of the material at the specified position and layer. ---
---@param position Vec2F
---@param layerName string
---@return number
function world.materialHueShift(position, layerName) end

--- Returns the hue shift of the mod at the specified position and layer. ---
---@param position Vec2F
---@param layerName string
---@return number
function world.modHueShift(position, layerName) end

--- Returns the color variant (painted color) of the material at the specified position and layer. ---
---@param position Vec2F
---@param layerName string
---@return unsigned
function world.materialColor(position, layerName) end

--- Sets the color variant of the material at the specified position and layer to the specified color. ---
---@param position Vec2F
---@param layerName string
---@param color unsigned
---@return void
function world.setMaterialColor(position, layerName, color) end

--- Damages all tiles in the specified layer and positions by the specified amount. The source position of the damage determines the initial direction of the damage particles. Damage types are: "plantish", "blockish", "beamish", "explosive", "fire", "tilling". Harvest level determines whether destroyed materials or mods will drop as items. Returns `true` if any damage was done and `false` otherwise. ---
---@param positions List<Vec2I>
---@param layerName string
---@param sourcePosition Vec2F
---@param damageType string
---@param damageAmount number
---@param harvestLevel unsigned
---@param sourceEntity EntityId
---@return boolean
function world.damageTiles(positions, layerName, sourcePosition, damageType, damageAmount, harvestLevel, sourceEntity) end

--- Identical to world.damageTiles but applies to tiles in a circular radius around the specified center point. ---
---@param center Vec2F
---@param radius number
---@param layerName string
---@param sourcePosition Vec2F
---@param damageType string
---@param damageAmount number
---@param harvestLevel unsigned
---@param sourceEntity EntityId
---@return boolean
function world.damageTileArea(center, radius, layerName, sourcePosition, damageType, damageAmount, harvestLevel, sourceEntity) end

--- Returns a list of existing tiles within `radius` of the given position, on the specified tile layer. ---
---@param position Vec2F
---@param radius number
---@param layerName string
---@return List<Vec2I>
function world.radialTileQuery(position, radius, layerName) end

--- Attempts to place the specified material in the specified position and layer. If allowOverlap is `true` the material can be placed in a space occupied by mobile entities, otherwise such placement attempts will fail. Returns `true` if the placement succeeds and `false` otherwise. ---
---@param position Vec2I
---@param layerName string
---@param materialName string
---@param hueShift number
---@param allowOverlap boolean
---@return boolean
function world.placeMaterial(position, layerName, materialName, hueShift, allowOverlap) end

--- Attempts to place the specified mod in the specified position and layer. If allowOverlap is `true` the mod can be placed in a space occupied by mobile entities, otherwise such placement attempts will fail. Returns `true` if the placement succeeds and `false` otherwise. ---
---@param position Vec2I
---@param layerName string
---@param modName string
---@param hueShift number
---@param allowOverlap boolean
---@return boolean
function world.placeMod(position, layerName, modName, hueShift, allowOverlap) end

--- Queries for entities in a specified area of the world and returns a list of their entity ids. Area can be specified either as the `Vec2F` lower left and upper right positions of a rectangle, or as the `Vec2F` center and `float` radius of a circular area. The following additional parameters can be specified in options: * __withoutEntityId__ - Specifies an `EntityId` that will be excluded from the returned results * __includedTypes__ - Specifies a list of one or more `String` entity types that the query will return. In addition to standard entity type names, this list can include "mobile" for all mobile entity types or "creature" for players, monsters and NPCs. * __boundMode__ - Specifies the bounding mode for determining whether entities fall within the query area. Valid options are "position", "collisionarea", "metaboundbox". Defaults to "collisionarea" if unspecified. * __order__ - A `String` used to specify how the results will be ordered. If this is set to "nearest" the entities will be sorted by ascending distance from the first positional argument. If this is set to "random" the list of results will be shuffled. * __callScript__ - Specifies a `String` name of a function that should be called in the script context of all scripted entities matching the query. * __callScriptArgs__ - Specifies a list of arguments that will be passed to the function called by callScript. * __callScriptResult__ - Specifies a `LuaValue` that the function called by callScript must return; entities whose script calls do not return this value will be excluded from the results. Defaults to `true`. ---
---@param position Vec2F
---@param options Json
---@return List<EntityId>
function world.entityQuery(position, options) end

--- Identical to world.entityQuery but only considers monsters. ---
---@param position Vec2F
---@param options Json
---@return List<EntityId>
function world.monsterQuery(position, options) end

--- Identical to world.entityQuery but only considers NPCs. ---
---@param position Vec2F
---@param options Json
---@return List<EntityId>
function world.npcQuery(position, options) end

--- Similar to world.entityQuery but only considers objects. Allows an additional option, __name__, which specifies a `String` object type name and will only return objects of that type. ---
---@param position Vec2F
---@param options Json
---@return List<EntityId>
function world.objectQuery(position, options) end

--- Identical to world.entityQuery but only considers item drops. ---
---@param position Vec2F
---@param options Json
---@return List<EntityId>
function world.itemDropQuery(position, options) end

--- Identical to world.entityQuery but only considers players. ---
---@param position Vec2F
---@param options Json
---@return List<EntityId>
function world.playerQuery(position, options) end

--- Similar to world.entityQuery but only considers loungeable entities. Allows an additional option, __orientation__, which specifies the `String` name of a loungeable orientation ("sit", "lay" or "stand") and only returns loungeable entities which use that orientation. ---
---@param position Vec2F
---@param options Json
---@return List<EntityId>
function world.loungeableQuery(position, options) end

--- Similar to world.entityQuery but only returns entities that intersect the line between the specified positions. ---
---@param startPosition Vec2F
---@param endPosition Vec2F
---@param options Json
---@return List<EntityId>
function world.entityLineQuery(startPosition, endPosition, options) end

--- Identical to world.entityLineQuery but only considers objects. ---
---@param startPosition Vec2F
---@param endPosition Vec2F
---@param options Json
---@return List<EntityId>
function world.objectLineQuery(startPosition, endPosition, options) end

--- Identical to world.entityLineQuery but only considers NPCs. ---
---@param startPosition Vec2F
---@param endPosition Vec2F
---@param options Json
---@return List<EntityId>
function world.npcLineQuery(startPosition, endPosition, options) end

--- Returns the entity id of any object occupying the specified tile position, or `nil` if the position is not occupied by an object. ---
---@param tilePosition Vec2I
---@return EntityId
function world.objectAt(tilePosition) end

--- Returns `true` if an entity with the specified id exists in the world and `false` otherwise. ---
---@param entityId EntityId
---@return boolean
function world.entityExists(entityId) end

--- Returns the current damage team (team type and team number) of the specified entity, or `nil` if the entity doesn't exist. ---
---@param entityId EntityId
---@return DamageTeam
function world.entityDamageTeam(entityId) end

--- Returns `true` if the specified source entity can damage the specified target entity using their current damage teams and `false` otherwise. ---
---@param sourceId EntityId
---@param targetId EntityId
---@return boolean
function world.entityCanDamage(sourceId, targetId) end

--- Returns `true` if the specified entity is an aggressive monster or NPC and `false` otherwise. ---
---@param entity EntityId
---@return boolean
function world.entityAggressive(entity) end

--- Returns the entity type name of the specified entity, or `nil` if the entity doesn't exist. ---
---@param entityId EntityId
---@return string
function world.entityType(entityId) end

--- Returns the current world position of the specified entity, or `nil` if the entity doesn't exist. ---
---@param entityId EntityId
---@return Vec2F
function world.entityPosition(entityId) end

--- Returns the current world mouth position of the specified player, monster, NPC or object, or `nil` if the entity doesn't exist or isn't a valid type. ---
---@param entityId EntityId
---@return Vec2F
function world.entityMouthPosition(entityId) end

--- Returns the current velocity of the entity if it is a vehicle, monster, NPC or player and `nil` otherwise. ---
---@param entityId EntityId
---@return Vec2F
function world.entityVelocity(entityId) end

--- Returns the meta bound box of the entity, if any. ---
---@param entityId EntityId
---@return Vec2F
function world.entityMetaBoundBOx(entityId) end

--- Returns the specified player entity's stock of the specified currency type, or `nil` if the entity is not a player. ---
---@param entityId EntityId
---@param currencyType string
---@return unsigned
function world.entityCurrency(entityId, currencyType) end

--- Returns the nubmer of the specified item that the specified player entity is currently carrying, or `nil` if the entity is not a player. If exactMatch is `true` then parameters as well as item name must match. NOTE: This function currently does not work correctly over the network, making it inaccurate when not used from client side scripts such as status. ---
---@param entityId EntityId
---@param itemDescriptor Json
---@param exactMatch boolean
---@return unsigned
function world.entityHasCountOfItem(entityId, itemDescriptor, exactMatch) end

--- Returns a `Vec2F` containing the specified entity's current and maximum health if the entity is a player, monster or NPC and `nil` otherwise. ---
---@param entityId EntityId
---@return Vec2F
function world.entityHealth(entityId) end

--- Returns the name of the specified entity's species if it is a player or NPC and `nil` otherwise. ---
---@param entityId EntityId
---@return string
function world.entitySpecies(entityId) end

--- Returns the name of the specified entity's gender if it is a player or NPC and `nil` otherwise. ---
---@param entityId EntityId
---@return string
function world.entityGender(entityId) end

--- Returns a `String` name of the specified entity which has different behavior for different entity types. For players, monsters and NPCs, this will be the configured name of the specific entity. For objects or vehicles, this will be the name of the object or vehicle type. For item drops, this will be the name of the contained item. ---
---@param entityId EntityId
---@return string
function world.entityName(entityId) end

--- Similar to world.entityName but returns the names of configured types for NPCs and monsters. ---
---@param entityId EntityId
---@return string
function world.entityTypeName(entityId) end

--- Returns the configured description for the specified inspectable entity (currently only objects and plants support this). Will return a species-specific description if species is specified and a generic description otherwise. ---
---@param entityId EntityId
---@param species string
---@return string
function world.entityDescription(entityId, species) end

--- Generates a portrait of the specified entity in the specified portrait mode and returns a list of drawables, or `nil` if the entity is not a portrait entity. ---
---@param entityId EntityId
---@param portraitMode string
---@return JsonArray
function world.entityPortrait(entityId, portraitMode) end

--- Returns the name of the item held in the specified hand of the specified player or NPC, or `nil` if the entity is not holding an item or is not a player or NPC. Hand name should be specified as "primary" or "alt". ---
---@param entityId EntityId
---@param handName string
---@return string
function world.entityHandItem(entityId, handName) end

--- Similar to world.entityHandItem but returns the full descriptor of the item rather than the name. --- ### `ItemDescriptor` world.itemDropItem(`EntityId` entityId) Returns the item descriptor of an item drop's contents. --- ### `Maybe<List<MaterialId>>` world.biomeBlocksAt(`Vec2I` position) Returns the list of biome specific blocks that can place in the biome at the specified position. ---
---@param entityId EntityId
---@param handName string
---@return ItemDescriptor
function world.entityHandItemDescriptor(entityId, handName) end

--- Returns the unique id of the specified entity, or `nil` if the entity does not have a unique id. ---
---@param entityId EntityId
---@return string
function world.entityUniqueId(entityId) end

--- Returns the value of the specified NPC's variant script config parameter, or defaultValue or `nil` if the parameter is not set or the entity is not an NPC. ---
---@param entityId EntityId
---@param parameterName string
---@param defaultValue Json
---@return Json
function world.getNpcScriptParameter(entityId, parameterName, defaultValue) end

--- Returns the value of the specified object's config parameter, or defaultValue or `nil` if the parameter is not set or the entity is not an object. ---
---@param entityId EntityId
---@param parameterName string
---@param defaultValue Json
---@return Json
function world.getObjectParameter(entityId, parameterName, defaultValue) end

--- Returns a list of tile positions that the specified object occupies, or `nil` if the entity is not an object. ---
---@param entityId EntityId
---@return List<Vec2I>
function world.objectSpaces(entityId) end

--- Returns the current growth stage of the specified farmable object, or `nil` if the entity is not a farmable object. ---
---@param entityId EntityId
---@return number
function world.farmableStage(entityId) end

--- Returns the total capacity of the specified container, or `nil` if the entity is not a container. ---
---@param entityId EntityId
---@return number
function world.containerSize(entityId) end

--- Visually closes the specified container. Returns `true` if the entity is a container and `false` otherwise. ---
---@param entityId EntityId
---@return boolean
function world.containerClose(entityId) end

--- Visually opens the specified container. Returns `true` if the entity is a container and `false` otherwise. ---
---@param entityId EntityId
---@return boolean
function world.containerOpen(entityId) end

--- Returns a list of pairs of item descriptors and container positions of all items in the specified container, or `nil` if the entity is not a container. ---
---@param entityId EntityId
---@return JsonArray
function world.containerItems(entityId) end

--- Returns an item descriptor of the item at the specified position in the specified container, or `nil` if the entity is not a container or the offset is out of range. ---
---@param entityId EntityId
---@param offset unsigned
---@return ItemDescriptor
function world.containerItemAt(entityId, offset) end

--- Attempts to consume items from the specified container that match the specified item descriptor and returns `true` if successful, `false` if unsuccessful, or `nil` if the entity is not a container. Only succeeds if the full count of the specified item can be consumed. ---
---@param entityId EntityId
---@param item ItemDescriptor
---@return boolean
function world.containerConsume(entityId, item) end

--- Similar to world.containerConsume but only considers the specified slot within the container. ---
---@param entityId EntityId
---@param offset unsigned
---@param count unsigned
---@return boolean
function world.containerConsumeAt(entityId, offset, count) end

--- Returns the number of the specified item that are currently available to consume in the specified container, or `nil` if the entity is not a container. ---
---@param entityId EntityId
---@param item ItemDescriptor
---@return unsigned
function world.containerAvailable(entityId, item) end

--- Similar to world.containerItems but consumes all items in the container. ---
---@param entityId EntityId
---@return JsonArray
function world.containerTakeAll(entityId) end

--- Similar to world.containerItemAt, but consumes all items in the specified slot of the container. ---
---@param entityId EntityId
---@param offset unsigned
---@return ItemDescriptor
function world.containerTakeAt(entityId, offset) end

--- Similar to world.containerTakeAt, but consumes up to (but not necessarily equal to) the specified count of items from the specified slot of the container and returns only the items consumed. ---
---@param entityId EntityId
---@param offset unsigned
---@param count unsigned
---@return ItemDescriptor
function world.containerTakeNumItemsAt(entityId, offset, count) end

--- Returns the number of times the specified item can fit in the specified container, or `nil` if the entity is not a container. ---
---@param entityId EntityId
---@param item ItemDescriptor
---@return unsigned
function world.containerItemsCanFit(entityId, item) end

--- Returns a JsonObject containing a list of "slots" the specified item would fit and the count of "leftover" items that would remain after attempting to add the items. Returns `nil` if the entity is not a container. ---
---@param entityId EntityId
---@param items ItemDescriptor
---@return Json
function world.containerItemsFitWhere(entityId, items) end

--- Adds the specified items to the specified container. Returns the leftover items after filling the container, or all items if the entity is not a container. ---
---@param entityId EntityId
---@param items ItemDescriptor
---@return ItemDescriptor
function world.containerAddItems(entityId, items) end

--- Similar to world.containerAddItems but will only combine items with existing stacks and will not fill empty slots. ---
---@param entityId EntityId
---@param items ItemDescriptor
---@return ItemDescriptor
function world.containerStackItems(entityId, items) end

--- Similar to world.containerAddItems but only considers the specified slot in the container. ---
---@param entityId EntityId
---@param items ItemDescriptor
---@param offset unsigned
---@return ItemDescriptor
function world.containerPutItemsAt(entityId, items, offset) end

--- Attempts to combine the specified items with the current contents (if any) of the specified container slot and returns any items unable to be placed into the slot. ---
---@param entityId EntityId
---@param items ItemDescriptor
---@param offset unsigned
---@return ItemDescriptor
function world.containerItemApply(entityId, items, offset) end

--- Places the specified items into the specified container slot and returns the previous contents of the slot if successful, or the original items if unsuccessful. ---
---@param entityId EntityId
---@param items ItemDescriptor
---@param offset unsigned
---@return ItemDescriptor
function world.containerSwapItemsNoCombine(entityId, items, offset) end

--- A combination of world.containerItemApply and world.containerSwapItemsNoCombine that attempts to combine items before swapping and returns the leftovers if stacking was successful or the previous contents of the container slot if the items did not stack. ---
---@param entityId EntityId
---@param items ItemDescriptor
---@param offset unsigned
---@return ItemDescriptor
function world.containerSwapItems(entityId, items, offset) end

--- Attempts to call the specified function name in the context of the specified scripted entity with the specified arguments and returns the result. This method is synchronous and thus can only be used on local master entities, i.e. scripts run on the server may only call scripted entities that are also server-side master and scripts run on the client may only call scripted entities that are client-side master on that client. For more featureful entity messaging, use world.sendEntityMessage. ---
---@param entityId EntityId
---@param functionName string
---@param args LuaValue
---@return LuaValue
function world.callScriptedEntity(entityId, functionName, args) end

--- Sends an asynchronous message to an entity with the specified entity id or unique id with the specified message type and arguments and returns an `RpcPromise` which can be used to receive the result of the message when available. See the message table for information on entity message handling. This function __should not be called in any entity's init function__ as the sending entity will not have been fully loaded. ---
---@param messageType string
---@param args LuaValue
---@return RpcPromise<Json>
function world.sendEntityMessage(messageType, args) end

--- Attempts to find an entity on the server by unique id and returns an `RpcPromise` that can be used to get the position of that entity if successful. ---
---@param uniqueId string
---@return RpcPromise<Vec2F>
function world.findUniqueEntity(uniqueId) end

--- Checks whether the specified loungeable entity is currently occupied and returns `true` if it is occupied, `false` if it is unoccupied, or `nil` if it is not a loungeable entity. ---
---@param entityId EntityId
---@return boolean
function world.loungeableOccupied(entityId) end

--- Returns `true` if the specified entity exists and is a monster and `false` otherwise. If aggressive is specified, will return `false` unless the monster's aggressive state matches the specified value. ---
---@param entityId EntityId
---@param aggressive boolean
---@return boolean
function world.isMonster(entityId, aggressive) end

--- Returns the monster type of the specified monster, or `nil` if the entity is not a monster. ---
---@param entityId EntityId
---@return string
function world.monsterType(entityId) end

--- Returns `true` if the specified entity exists and is an NPC and `false` otherwise. If damageTeam is specified, will return `false` unless the NPC's damage team number matches the specified value. ---
---@param entityId EntityId
---@param damageTeam number
---@return boolean
function world.isNpc(entityId, damageTeam) end

--- Returns `true` if an entity with the specified id is player interactive and `false` otherwise. ---
---@param entityId EntityId
---@return boolean
function world.isEntityInteractive(entityId) end

--- Returns the NPC type of the specified NPC, or `nil` if the entity is not an NPC. ---
---@param entityId EntityId
---@return string
function world.npcType(entityId) end

--- Returns the stagehand type of the specified stagehand, or `nil` if the entity is not a stagehand. ---
---@param entityId EntityId
---@return string
function world.stagehandType(entityId) end

--- Displays a point visible in debug mode at the specified world position. ---
---@param position Vec2F
---@param color Color
---@return void
function world.debugPoint(position, color) end

--- Displayes a line visible in debug mode between the specified world positions. ---
---@param startPosition Vec2F
---@param endPosition Vec2F
---@param color Color
---@return void
function world.debugLine(startPosition, endPosition, color) end

--- Displays a polygon consisting of the specified points that is visible in debug mode. ---
---@param poly PolyF
---@param color Color
---@return void
function world.debugPoly(poly, color) end

--- Displays text visible in debug mode at the specified position using the specified format string and optional formatted values. --- The following additional world bindings are available only for scripts running on the server. ---
---@param formatString string
---@param formatValues LuaValue
---@param position Vec2F
---@param color Color
---@return void
function world.debugText(formatString, formatValues, position, color) end

--- Breaks the specified object and returns `true` if successful and `false` otherwise. If smash is `true` the object will not (by default) drop any items. ---
---@param entityId EntityId
---@param smash boolean
---@return boolean
function world.breakObject(entityId, smash) end

--- Returns `true` if any part of the specified region overlaps any player's screen area and `false` otherwise. ---
---@param region RectF
---@return boolean
function world.isVisibleToPlayer(region) end

--- Attempts to load all sectors overlapping the specified region and returns `true` if all sectors are fully loaded and `false` otherwise. ---
---@param region RectF
---@return boolean
function world.loadRegion(region) end

--- Returns `true` if all sectors overlapping the specified region are fully loaded and `false` otherwise. ---
---@param region RectF
---@return boolean
function world.regionActive(region) end

--- Enables or disables tile protection for the specified dungeon id. ---
---@param dungeonId DungeonId
---@param protected boolean
---@return void
function world.setTileProtection(dungeonId, protected) end

--- Returns the dungeon id at the specified world position. ---
---@param position Vec2F
---@return DungeonId
function world.dungeonId(position) end

--- Sets the dungeonId of all tiles within the specified area. ---
---@param tileArea RectI
---@param dungeonId DungeonId
---@return DungeonId
function world.setDungeonId(tileArea, dungeonId) end

--- Enqueues a biome distribution config for placement through world generation. The returned promise is fulfilled with the position of the placement, once it has been placed. ---
---@param distributionConfigs List<Json>
---@param id DungeonId
---@return Promise<Vec2I>
function world.enqueuePlacement(distributionConfigs, id) end

--- Returns `true` if any tile within the specified region has been modified (placed or broken) by a player and `false` otherwise. ---
---@param region RectI
---@return boolean
function world.isPlayerModified(region) end

--- Identical to world.destroyLiquid but ignores tile protection. ---
---@param position Vec2F
---@return LiquidLevel
function world.forceDestroyLiquid(position) end

--- Forces (synchronous) loading of the specified unique entity and returns its non-unique entity id or 0 if no such unique entity exists. ---
---@param uniqueId string
---@return EntityId
function world.loadUniqueEntity(uniqueId) end

--- Sets the unique id of the specified entity to the specified unique id or clears it if no unique id is specified. ---
---@param entityId EntityId
---@param uniqueId string
---@return void
function world.setUniqueId(entityId, uniqueId) end

--- Takes the specified item drop and returns an `ItemDescriptor` of its contents or `nil` if the operation fails. If a source entity id is specified, the item drop will briefly animate toward that entity. ---
---@param targetEntityId EntityId
---@param sourceEntityId EntityId
---@return ItemDescriptor
function world.takeItemDrop(targetEntityId, sourceEntityId) end

--- Sets the world's default beam-down point to the specified position. If respawnInWorld is set to `true` then players who die in that world will respawn at the specified start position rather than being returned to their ships. ---
---@param position Vec2F
---@param respawnInWorld boolean
---@return void
function world.setPlayerStart(position, respawnInWorld) end

--- Returns a list of the entity ids of all players currently in the world. ---
---@return List<EntityId>
function world.players() end

--- Returns the name of the fidelity level at which the world is currently running. See worldserver.config for fidelity configuration. ---
---@return string
function world.fidelity() end

--- Returns the current flight status of a ship world. ---
---@return string
function world.flyingType() end

--- Returns the current warp phase of a ship world. ---
---@return string
function world.warpPhase() end

--- Sets the specified universe flag on the current universe. ---
---@param flagName string
---@return void
function world.setUniverseFlag(flagName) end

--- Returns a list of all universe flags set on the current universe. ---
---@return List<String>
function world.universeFlags() end

--- Returns `true` if the specified universe flag is set and `false` otherwise. ---
---@param flagName string
---@return boolean
function world.universeFlagSet(flagName) end

--- Returns the current time for the world's sky. ---
---@return number
function world.skyTime() end

--- Sets the current time for the world's sky to the specified value. ---
---@param time number
---@return void
function world.setSkyTime(time) end

--- Generates the specified dungeon in the world at the specified position, ignoring normal dungeon anchoring rules. If a dungeon id is specified, it will be assigned to the dungeon. ---
---@param dungeonName string
---@param position Vec2I
---@param dungeonId DungeonId
---@return void
function world.placeDungeon(dungeonName, position, dungeonId) end

--- Generates the specified dungeon in the world at the specified position. Does not ignore anchoring rules, will fail if the dungeon can't be placed. If a dungeon id is specified, it will be assigned to the dungeon. ---
---@param dungeonName string
---@param position Vec2I
---@param dungeonId DungeonId
---@return void
function world.placeDungeon(dungeonName, position, dungeonId) end

--- Adds a biome region to the world, centered on `position`, `width` blocks wide. ---
---@param position Vec2I
---@param biomeName string
---@param subBlockSelector string
---@param width number
---@return void
function world.addBiomeRegion(position, biomeName, subBlockSelector, width) end

--- Expands the biome region currently at `position` by `width` blocks. ---
---@param position Vec2I
---@param width number
---@return void
function world.expandBiomeRegion(position, width) end

--- Signals a region for asynchronous generation. The region signaled is the region that needs to be generated to add a biome region of `width` tiles to `position`. ---
---@param position Vec2I
---@param width number
---@return void
function world.pregenerateAddBiome(position, width) end

--- Signals a region for asynchronous generation. The region signaled is the region that needs to be generated to expand the biome at `position` by `width` blocks. ---
---@param position Vec2I
---@param width number
---@return void
function world.pregenerateExpandBiome(position, width) end

--- Sets the environment biome for a layer to the biome at `position`. ---
---@param position Vec2I
---@return void
function world.setLayerEnvironmentBiome(position) end

--- Sets the planet type of the current world to `planetType` with primary biome `primaryBiomeName`. ---
---@param planetType string
---@return void
function world.setPlanetType(planetType) end

--- Sets the overriding gravity for the specified dungeon id, or returns it to the world default if unspecified. ---
---@param dungeonId DungeonId
---@param gravity Maybe<float>
---@return void
function world.setDungeonGravity(dungeonId, gravity) end

--- Sets the overriding breathability for the specified dungeon id, or returns it to the world default if unspecified.
---@param dungeonId DungeonId
---@param breathable Maybe<bool>
---@return void
function world.setDungeonBreathable(dungeonId, breathable) end

--- Global functions

--- Explores the path up to the specified node count limit. Returns `true` if the pathfinding is complete and `false` if it is still incomplete. If nodeLimit is unspecified, this will search up to the configured maximum number of nodes, making it equivalent to world.platformerPathStart.
---@param nodeLimit number
---@return boolean
function explore(nodeLimit) end

--- Returns the completed path. ---
---@return PlatformerAStar::Path
function result() end
