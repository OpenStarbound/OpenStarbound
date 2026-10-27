---@meta

--- monster API
---@class monster
monster = {}

--- Returns the monster's configured monster type. ---
---@return string
function monster.type() end

--- Returns a string representation of the monster's random seed. ---
---@return string
function monster.seed() end

--- Returns a table of the monster's unique (override) parameters. ---
---@return Json
function monster.uniqueParameters() end

--- Returns the monster's family index. ---
---@return unsigned
function monster.familyIndex() end

--- Returns the monster's level. ---
---@return number
function monster.level() end

--- Enables or disables the monster's touch damage. ---
---@param enabled boolean
---@return void
function monster.setDamageOnTouch(enabled) end

--- Sets the monster's active damage sources (or clears them if unspecified). ---
---@param damageSources List<DamageSource>
---@return void
function monster.setDamageSources(damageSources) end

--- Sets the monster's active damage parts. Damage parts must be defined in the monster's configuration parameters. A damage part specifies a damage source and an animation part to anchor the damage source to. The anchor part's transformation will be applied to the damage source's damage poly, and if a vector, the damage source's knockback. ```js "animationDamageParts" : { "beam" : { "anchorPart" : "partName", // animation part to anchor the damage source to "checkLineCollision" : false, // optional, if the damage source is a line, check for collision along the line "bounces" : 0, // optional, if the damage source is a line and it checks for collision "damageSource" : { "line" : [ [0.0, 0.0], [5.0, 0.0] ], "damage" : 10, "damageSourceKind" : "default", "teamType" : "enemy", "teamNumber" : 2 } } } ``` ```lua monster.setDamageParts({"beam"}) -- sets the "beam" damage part active ``` ---
---@param damageParts StringSet
---@return void
function monster.setDamageParts(damageParts) end

--- Sets whether the monster is currently aggressive. ---
---@param aggressive boolean
---@return void
function monster.setAggressive(aggressive) end

--- Sets the monster's drop pool, which determines the items that it will drop on death. This can be specified as the `String` name of a treasure pool, or as a `Map<String, String>` to specify different drop pools for different damage types. If specified as a map, the pool should contain a "default" entry for unhandled damage types. ---
---@param dropPool Json
---@return void
function monster.setDropPool(dropPool) end

--- Returns an absolute world position calculated from the given relative position. ---
---@param relativePosition Vec2F
---@return Vec2F
function monster.toAbsolutePosition(relativePosition) end

--- Returns the world position of the monster's mouth. ---
---@return Vec2F
function monster.mouthPosition() end

--- Causes the monster to controlFly toward the given world position. ---
---@param position Vec2F
---@return void
function monster.flyTo(position) end

--- Sets the name of the particle emitter (configured in the animation) to burst when the monster dies, or clears it if unspecified. ---
---@param particleEmitter string
---@return void
function monster.setDeathParticleBurst(particleEmitter) end

--- Sets the name of the sound (configured in the animation) to play when the monster dies, or clears it if unspecified. ---
---@param sound string
---@return void
function monster.setDeathSound(sound) end

--- Sets a list of physics force regions that the monster will project, used for applying forces to other nearby entities. Set an empty list to clear the force regions. ---
---@param forceRegions List<PhysicsForceRegion>
---@return void
function monster.setPhysicsForces(forceRegions) end

--- Sets the monster's name. ---
---@param name string
---@return void
function monster.setName(name) end

--- Sets whether the monster should display its nametag. ---
---@param enabled boolean
---@return void
function monster.setDisplayNametag(enabled) end

--- Causes the monster to say the line, optionally replacing any specified tags in the text. Returns `true` if anything is said (i.e. the line is not empty) and `false` otherwise. ---
---@param line string
---@param tags Map<String, String>
---@return boolean
function monster.say(line, tags) end

--- Similar to monster.say, but uses a portrait chat bubble with the specified portrait image. ---
---@param line string
---@param portrait string
---@param tags Map<String, String>
---@return boolean
function monster.sayPortrait(line, portrait, tags) end

--- Sets the monster's current damage team type and number. ---
---@param team DamageTeam
---@return void
function monster.setDamageTeam(team) end

--- Sets the monster's unique entity id, or clears it if unspecified. ---
---@param uniqueId string
---@return void
function monster.setUniqueId(uniqueId) end

--- Sets the type of damage bar that the monster should display. Valid options are "default", "none" and "special". ---
---@param damageBarType string
---@return void
function monster.setDamageBar(damageBarType) end

--- Sets whether the monster is currently interactive. ---
---@param interactive boolean
---@return void
function monster.setInteractive(interactive) end

--- Sets a networked scripted animator parameter to be used in a client side rendering script using animationConfig.getParameter.
---@param key string
---@param value Json
---@return void
function monster.setAnimationParameter(key, value) end
