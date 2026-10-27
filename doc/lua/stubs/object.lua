---@meta

--- object API
---@class object
object = {}

--- Returns the object's type name. ---
---@return string
function object.name() end

--- Returns the object's facing direction. This will be 1 for right or -1 for left. ---
---@return number
function object.direction() end

--- Returns the object's tile position. This is identical to entity.position(), so use that instead. ---
---@return Vec2F
function object.position() end

--- Sets whether the object is currently interactive. ---
---@param interactive boolean
---@return void
function object.setInteractive(interactive) end

--- Returns the object's unique entity id, or `nil` if no unique id is set. This should be identical to entity.uniqueId(), so use that instead. ---
---@return string
function object.uniqueId() end

--- Sets the objects unique entity id, or clears it if unspecified. ---
---@param uniqueId string
---@return void
function object.setUniqueId(uniqueId) end

--- Returns the object's metaBoundBox in world space. ---
---@return RectF
function object.boundBox() end

--- Returns a list of the tile spaces that the object occupies. ---
---@return List<Vec2I>
function object.spaces() end

--- Sets the image processing directives that should be applied to the object's animation. ---
---@param directives string
---@return void
function object.setProcessingDirectives(directives) end

--- Enables or disables the object's persistent sound effect, if one is configured. ---
---@param enabled boolean
---@return void
function object.setSoundEffectEnabled(enabled) end

--- Breaks the object. If smash is `true` then it will be smashed, causing it to (by default) drop no items. ---
---@param smash boolean
---@return void
function object.smash(smash) end

--- Returns the "level" parameter if set, otherwise returns the current world's threat level. ---
---@return number
function object.level() end

--- Returns an absolute world position calculated from the given relative position. ---
---@param relativePosition Vec2F
---@return Vec2F
function object.toAbsolutePosition(relativePosition) end

--- Causes the object to say the line, optionally replacing any specified tags in the text, and using the provided additional chat configuration. Returns `true` if anything is said (i.e. the line is not empty) and `false` otherwise. ---
---@param line string
---@param tags Map<String, String>
---@param config Json
---@return boolean
function object.say(line, tags, config) end

--- Similar to object.say, but uses a portrait chat bubble with the specified portrait image. ---
---@param line string
---@param portrait string
---@param tags Map<String, String>
---@param config Json
---@return boolean
function object.sayPortrait(line, portrait, tags, config) end

--- Returns `true` if the specified entity's collision area overlaps the object's bound box and `false` otherwise. ---
---@param entityId EntityId
---@return boolean
function object.isTouching(entityId) end

--- Sets the color of light for the object to emit. This is not the same as animator.setLightColor and the animator light configuration should be used for more featureful light sources. ---
---@param color Color
---@return void
function object.setLightColor(color) end

--- Returns the object's currently configured light color. ---
---@return Color
function object.getLightColor() end

--- Returns the number of wire input nodes the object has. ---
---@return unsigned
function object.inputNodeCount() end

--- Returns the number of wire output nodes the object has. ---
---@return unsigned
function object.outputNodeCount() end

--- Returns the relative position of the specified wire input node. ---
---@param nodeIndex unsigned
---@return Vec2I
function object.getInputNodePosition(nodeIndex) end

--- Returns the relative position of the specified wire output node. ---
---@param nodeIndex unsigned
---@return Vec2I
function object.getOutputNodePosition(nodeIndex) end

--- Returns the current level of the specified wire input node. ---
---@param nodeIndex unsigned
---@return boolean
function object.getInputNodeLevel(nodeIndex) end

--- Returns the current level of the specified wire output node. ---
---@param nodeIndex unsigned
---@return boolean
function object.getOutputNodeLevel(nodeIndex) end

--- Returns `true` if any wires are currently connected to the specified wire input node and `false` otherwise. ---
---@param nodeIndex unsigned
---@return boolean
function object.isInputNodeConnected(nodeIndex) end

--- Returns `true` if any wires are currently connected to the specified wire output node and `false` otherwise ---
---@param nodeIndex unsigned
---@return boolean
function object.isOutputNodeConnected(nodeIndex) end

--- Returns a map of the entity id of each wire entity connected to the given wire input node and the index of that entity's output node to which the input node is connected. ---
---@param nodeIndex unsigned
---@return Map<EntityId, unsigned>
function object.getInputNodeIds(nodeIndex) end

--- Returns a map of the entity id of each wire entity connected to the given wire output node and the index of that entity's input node to which the output node is connected. ---
---@param nodeIndex unsigned
---@return Map<EntityId, unsigned>
function object.getOutputNodeIds(nodeIndex) end

--- Sets the level of the specified wire output node. ---
---@param nodeIndex unsigned
---@param level boolean
---@return void
function object.setOutputNodeLevel(nodeIndex, level) end

--- Sets the level of all wire output nodes. ---
---@param level boolean
---@return void
function object.setAllOutputNodes(level) end

--- Sets the list of quests that the object will offer to start, or clears them if unspecified. ---
---@param quests JsonArray
---@return void
function object.setOfferedQuests(quests) end

--- Sets the list of quests that the object will accept turn-in for, or clears them if unspecified. ---
---@param quests JsonArray
---@return void
function object.setTurnInQuests(quests) end

--- Sets the specified override configuration parameter for the object. ---
---@param key string
---@param value Json
---@return void
function object.setConfigParameter(key, value) end

--- Sets the specified animation parameter for the object's scripted animator. ---
---@param key string
---@param value Json
---@return void
function object.setAnimationParameter(key, value) end

--- Sets the object's material spaces to the specified list, or clears them if unspecified. List entries should be in the form of `pair<Vec2I, String>` specifying the relative position and material name of materials to be set. __Objects should only set material spaces within their occupied tile spaces to prevent Bad Things TM from happening.__ ---
---@param spaces JsonArray
---@return void
function object.setMaterialSpaces(spaces) end

--- Sets the object's active damage sources (or clears them if unspecified). ---
---@param damageSources List<DamageSource>
---@return void
function object.setDamageSources(damageSources) end

--- Returns the object's current health. ---
---@return number
function object.health() end

--- Sets the object's current health.
---@param health number
---@return void
function object.setHealth(health) end
