This is documentation on the Entity userdata object returned by world.entity(EntityId entity).

This userdata only needs to be acquired once per entity; it updates with the entity.
It even continues to function after the entity is destroyed, keeping the entity in memory for as long as this userdata is kept in memory.
As such, it is recommended to destroy references to this in your scripts after they are no longer needed.
Some functions may throw errors if used on dead entities.

---

#### `bool` entity:exists()

Returns `true` if this entity exists in a world and `false` otherwise.

---

#### `EntityId` entity:id()

Returns the entity's id.

---

#### `DamageTeam` entity:damageTeam()

Returns the current damage team (team type and team number) of this entity.

---

#### `bool` entity:canDamage(`EntityId` targetId)

Returns `true` if this entity can damage the specified target entity using their current damage teams and `false` otherwise.

---

#### `bool` entity:aggressive()

Returns `true` if this entity is an aggressive monster or NPC and `false` otherwise.

---

#### `String` entity:type()

Returns the entity type name of this entity.

---

#### `Vec2F` entity:aimPosition()

Returns the aim position of this entity.

---

#### `Vec2F` entity:position()

Returns the current world position of this entity.

---

#### `Vec2F` entity:mouthPosition()

Returns the current world mouth position of the entity if it is a player, monster, NPC, or object, or `nil` if the entity doesn't exist or isn't a valid type.

---

#### `Vec2F` entity:velocity()

Returns the current velocity of the entity if it is a vehicle, monster, NPC, player, or projectile and `nil` otherwise.

---

#### `Vec2F` entity:metaBoundBox()

Returns the meta bound box of the entity, if any.

---

#### `unsigned` entity:currency(`String` currencyType)

Returns the entity's stock of the specified currency type, or `nil` if the entity is not a player.

---

#### `unsigned` entity:hasCountOfItem(`Json` itemDescriptor, [`bool` exactMatch])

Returns the nubmer of the specified item that the entity is currently carrying, or `nil` if the entity is not a player. If exactMatch is `true` then parameters as well as item name must match.

NOTE: This function currently does not work correctly over the network, making it inaccurate when not used from client side scripts such as status.

---

#### `Vec2F` entity:health()

Returns a `Vec2F` containing the entity's current and maximum health if the entity is a player, monster or NPC and `nil` otherwise.

---

#### `String` entity:species()

Returns the name of the specified entity's species if it is a player or NPC and `nil` otherwise.

---

#### `String` entity:gender()

Returns the name of the entity's gender if it is a player or NPC and `nil` otherwise.

---

#### `String` entity:name()

Returns a `String` name of the entity which has different behavior for different entity types. For players, monsters and NPCs, this will be the configured name of the specific entity. For objects or vehicles, this will be the name of the object or vehicle type. For item drops, this will be the name of the contained item.

---

#### `Json` entity:nametag()

Returns data on the nametag of the entity, if the entity has nametags.

---

#### `String` entity:typeName()

Similar to entity:name but returns the names of configured types for NPCs and monsters.

---

#### `String` entity:description([`String` species])

Returns the configured description for the entity if it has one. Will return a species-specific description if species is specified and a generic description otherwise.

---

#### `JsonArray` entity:portrait(`String` portraitMode)

Generates a portrait of the entity in the specified portrait mode and returns a list of drawables, or `nil` if the entity is not a portrait entity.

---

#### `String` entity:handItem(`String` handName)

Returns the name of the item held in the specified hand of the entity, or `nil` if the entity is not holding an item or is not a player or NPC. Hand name should be specified as "primary" or "alt".

---

#### `ItemDescriptor` entity:handItemDescriptor(`String` handName)

Similar to entity:handItem but returns the full descriptor of the item rather than the name.

---

#### `String` entity:uniqueId(`EntityId` entityId)

Returns the unique id of the entity, or `nil` if the entity does not have a unique id.

---

#### `Json` entity:statusProperty(`String` name, [`Json` default])

Returns the value of the entity's status property, or defaultValue or `nil` if the parameter is not set.
Returns `nil` if the entity is not an actor entity. Actor entities are players, npcs, and monsters.

---

#### `Maybe<float>` entity:stat(`String` name)

Returns the value of the entity's stat or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:statPositive(`String` name)

Returns whether the entity's stat is positive or not, or `nil` if the entity is not an actor entity.

---

#### `Maybe<StringList>` entity:resourceNames()

Returns the names of all the entity's resources or `nil` if the entity is not an actor entity.

---

#### `Maybe<float>` entity:resource(`String` name)

Returns the value of the entity's specified resource or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:isResource(`String` name)

Returns whether this resource exists on the entity, or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:resourcePositive(`String` name)

Returns whether the entity's specified resource is positive or not, or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:resourceLocked(`String` name)

Returns whether the entity's specified resource is locked or not, or `nil` if the entity is not an actor entity.

---

#### `Maybe<float>` entity:resourceMax(`String` name)

Returns the maximum value of the entity's specified resource or `nil` if the entity is not an actor entity.

---

#### `Maybe<float>` entity:resourcePercentage(`String` name)

Returns the percentage of max of the entity's specified resource or `nil` if the entity is not an actor entity.

---

#### `Maybe<JsonArray>` entity:getPersistentEffects(`String` name)

Returns the entity's active persistent effects under the given category or `nil` if the entity is not an actor entity.

---

#### `Maybe<List<JsonArray>>` entity:activeUniqueStatusEffectSummary()

Returns the active unique status effects of the entity or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:uniqueStatusEffectActive(`String` name)

Returns if the specified status effect is active on the entity or `nil` if the entity is not an actor entity.

---

#### `Maybe<float>` entity:mass()

Returns the entity's mass or `nil` if the entity is not a mobile entity.
Mobile entities are actor entities as well as vehicles, projectiles, item drops, and plant drops (falling trees/vines)

---

#### `Maybe<RectF>` entity:boundBox()

Returns the entity's collision poly bound box or `nil` if the entity is not a mobile entity.

---

#### `Maybe<PolyF>` entity:collisionPoly()

Returns the entity's collision poly or `nil` if the entity is not a mobile entity.

---

#### `Maybe<PolyF>` entity:collisionBody()

Returns the entity's collision body or `nil` if the entity is not a mobile entity.

---

#### `Maybe<RectF>` entity:collisionBoundBox()

Returns the entity's collision poly bound box in world space coordinates or `nil` if the entity is not a mobile entity.

---

#### `Maybe<RectF>` entity:localBoundBox()

Returns the entity's movement controller local bound box or `nil` if the entity is not a mobile entity.

---

#### `Maybe<float>` entity:rotation()

Returns the entity's rotation or `nil` if the entity is not a mobile entity.

---

#### `Maybe<bool>` entity:isColliding()

Returns if the entity is colliding or `nil` if the entity is not a mobile entity.

---

#### `Maybe<bool>` entity:isNullColliding()

Returns if the entity is colliding with null collision or `nil` if the entity is not a mobile entity.

---

#### `Maybe<bool>` entity:isCollisionStuck()

Returns if the entity is sticking or `nil` if the entity is not a mobile entity.

---

#### `Maybe<float>` entity:stickingDirection()

Returns the direction the entity is sticking at or `nil` if the entity is not a mobile entity or isn't stuck.

---

#### `Maybe<float>` entity:liquidPercentage()

Returns the entity's percentage of being submerged in liquid or `nil` if the entity is not a mobile entity.

---

#### `Maybe<LiquidId>` entity:liquidId()

Returns the liquid the entity is submerged in or `nil` if the entity is not a mobile entity.

---

#### `Maybe<bool>` entity:onGround()

Returns if the entity is on the ground or `nil` if the entity is not a mobile entity.

---

#### `Maybe<bool>` entity:zeroG()

Returns if the entity is in zero-G movement or `nil` if the entity is not a mobile entity.

---

#### `Maybe<bool>` entity:atWorldLimit()

Returns if the entity is at the world limit or `nil` if the entity is not a mobile entity.

---

#### `Maybe<Json>` entity:baseMovementParameters()

Returns the entity's base movement parameters or `nil` if the entity is not an actor entity.

---

#### `Maybe<Json>` entity:movementParameters()

Returns the entity's current movement parameters or `nil` if the entity is not a mobile entity.

---

#### `Maybe<bool>` entity:walking()

Returns if the entity is walking or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:running()

Returns if the entity is running or `nil` if the entity is not an actor entity.

---

#### `Maybe<int>` entity:facingDirection()

Returns the entity's facing direction or `nil` if the entity is not an actor entity.

---

#### `Maybe<int>` entity:movingDirection()

Returns the entity's moving direction or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:crouching()

Returns if the entity is crouching or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:flying()

Returns if the entity is flying or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:falling()

Returns if the entity is falling or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:canJump()

Returns if the entity can jump or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:jumping()

Returns if the entity is jumping or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:groundMovement()

Returns if the entity is in ground movement or `nil` if the entity is not an actor entity.

---

#### `Maybe<bool>` entity:liquidMovement()

Returns if the entity is in liquid movement or `nil` if the entity is not an actor entity.

---

#### `Json` entity:getParameter(`String` parameterName, [`Json` defaultValue])

Returns the value of the entity's parameter, or defaultValue or `nil` if the parameter is not set.
Returns `nil` if the entity is not an entity parameters can be read from.
Currently, parameters can only be read from npcs, objects, projectiles, and stagehands.
---

#### `List<Vec2I>` entity:objectSpaces()

Returns a list of tile positions that the entity occupies, or `nil` if the entity is not an object.

---

#### `int` entity:farmableStage()

Returns the current growth stage of the specified farmable object, or `nil` if the entity is not a farmable object.

---

#### `int` entity:containerSize()

Returns the total capacity of the entity, or `nil` if the entity is not a container.

---

#### `bool` entity:containerClose()

Visually closes the entity. Returns `true` if the entity is a container and `false` otherwise.

---

#### `bool` entity:containerOpen()

Visually opens the entity. Returns `true` if the entity is a container and `false` otherwise.

---

#### `JsonArray` entity:containerItems()

Returns a list of pairs of item descriptors and container positions of all items in the entity, or `nil` if the entity is not a container.

---

#### `ItemDescriptor` entity:containerItemAt(`unsigned` offset)

Returns an item descriptor of the item at the specified position in the entity, or `nil` if the entity is not a container or the offset is out of range.

---

#### `bool` entity:containerConsume(`ItemDescriptor` item)

Attempts to consume items from the entity that match the specified item descriptor and returns `true` if successful, `false` if unsuccessful, or `nil` if the entity is not a container. Only succeeds if the full count of the specified item can be consumed.

---

#### `bool` entity:containerConsumeAt(`unsigned` offset, `unsigned` count)

Similar to world.containerConsume but only considers the specified slot within the entity.

---

#### `unsigned` entity:containerAvailable(`ItemDescriptor` item)

Returns the number of the specified item that are currently available to consume in the entity, or `nil` if the entity is not a container.

---

#### `JsonArray` entity:containerTakeAll()

Similar to world.containerItems but consumes all items in the entity.

---

#### `ItemDescriptor` entity:containerTakeAt(`unsigned` offset)

Similar to world.containerItemAt, but consumes all items in the specified slot of the entity.

---

#### `ItemDescriptor` entity:containerTakeNumItemsAt(`unsigned` offset, `unsigned` count)

Similar to world.containerTakeAt, but consumes up to (but not necessarily equal to) the specified count of items from the specified slot of the entity and returns only the items consumed.

---

#### `unsigned` entity:containerItemsCanFit(`ItemDescriptor` item)

Returns the number of times the specified item can fit in the entity, or `nil` if the entity is not a container.

---

#### `Json` entity:containerItemsFitWhere(`ItemDescriptor` items)

Returns a JsonObject containing a list of "slots" the specified item would fit and the count of "leftover" items that would remain after attempting to add the items. Returns `nil` if the entity is not a container.

---

#### `ItemDescriptor` entity:containerAddItems(`ItemDescriptor` items)

Adds the specified items to the entity. Returns the leftover items after filling the entity, or all items if the entity is not a container.

---

#### `ItemDescriptor` entity:containerStackItems(`ItemDescriptor` items)

Similar to world.containerAddItems but will only combine items with existing stacks and will not fill empty slots.

---

#### `ItemDescriptor` entity:containerPutItemsAt(`ItemDescriptor` items, `unsigned` offset)

Similar to world.containerAddItems but only considers the specified slot in the entity.

---

#### `ItemDescriptor` entity:containerItemApply(`ItemDescriptor` items, `unsigned` offset)

Attempts to combine the specified items with the current contents (if any) of the container slot and returns any items unable to be placed into the slot.

---

#### `ItemDescriptor` entity:containerSwapItemsNoCombine(`ItemDescriptor` items, `unsigned` offset)

Places the specified items into the entity slot and returns the previous contents of the slot if successful, or the original items if unsuccessful.

---

#### `ItemDescriptor` entity:containerSwapItems(`ItemDescriptor` items, `unsigned` offset)

A combination of world.containerItemApply and world.containerSwapItemsNoCombine that attempts to combine items before swapping and returns the leftovers if stacking was successful or the previous contents of the container slot if the items did not stack.

---

#### `LuaValue` entity:callScript(`String` functionName, [`LuaValue` args ...])

Attempts to call the specified function name in the context of the entity with the specified arguments and returns the result. This method is synchronous and thus can only be used on local master entities, i.e. scripts run on the server may only call scripted entities that are also server-side master and scripts run on the client may only call scripted entities that are client-side master on that client. For more featureful entity messaging, use entity:sendMessage.

---

#### `RpcPromise<Json>` entity:sendMessage(`String` messageType, [`LuaValue` args ...])

Sends an asynchronous message to the entity with the specified message type and arguments and returns an `RpcPromise` which can be used to receive the result of the message when available. See the message table for information on entity message handling. This function __should not be called in any entity's init function__ as the sending entity will not have been fully loaded.

---

#### `List<EntityId>` entity:loungingEntities()

Returns every entity lounging on the entity.

---

#### `bool` entity:loungeableOccupied()

Checks whether the specified loungeable entity is currently occupied and returns `true` if it is occupied, `false` if it is unoccupied, or `nil` if it is not a loungeable entity.

---

#### `int` entity:loungeableAnchorCount()

Returns the amount of anchors on the entity or `nil` if it is not a loungeable entity.

---

#### `bool` entity:isInteractive()

Returns `true` if the entity is player interactive and `false` otherwise.

---

#### `int` entity:movingCollisionCount()

Returns the amount of moving collisions in the entity.

---

#### `PhysicsMovingCollision` entity:movingCollision(`int` index)

Returns the entity's moving collision at the given index, or `nil` if it is disabled or does not exist.

---
