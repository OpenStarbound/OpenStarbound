---@meta

--- status API
---@class status
status = {}

--- Returns the value assigned to the specified status property. If there is no value set, returns default. ---
---@param name string
---@param default Json
---@return Json
function status.statusProperty(name, default) end

--- Sets a status property to the specified value. ---
---@param name string
---@param value Json
---@return void
function status.setStatusProperty(name, value) end

--- Returns the value for the specified stat. Defaults to 0.0 if the stat does not exist. ---
---@param statName string
---@return number
function status.stat(statName) end

--- Returns whether the stat value is greater than 0. ---
---@param statName string
---@return boolean
function status.statPositive(statName) end

--- Returns a list of the names of all the configured resources; ---
---@return List<String>
function status.resourceNames() end

--- Returns whether the specified resource exists in this status controller. ---
---@param resourceName string
---@return boolean
function status.isResource(resourceName) end

--- Returns the value of the specified resource. ---
---@param resourceName string
---@return number
function status.resource(resourceName) end

--- Returns whether the value of the specified resource is greater than 0. ---
---@param resourceName string
---@return boolean
function status.resourcePositive(resourceName) end

--- Sets a resource to the specified value. ---
---@param resourceName string
---@param value number
---@return void
function status.setResource(resourceName, value) end

--- Adds the specified value to a resource. ---
---@param resourceName string
---@param value number
---@return void
function status.modifyResource(resourceName, value) end

--- Adds the specified value to a resource. Returns any overflow. ---
---@param resourceName string
---@param value number
---@return number
function status.giveResource(resourceName, value) end

--- Tries to consume the specified amount from a resource. Returns whether the full amount was able to be consumes. Does not modify the resource if unable to consume the full amount. ---
---@param resourceName string
---@param amount number
---@return boolean
function status.consumeResource(resourceName, amount) end

--- Tries to consume the specified amount from a resource. If unable to consume the full amount, will consume all the remaining amount. Returns whether it was able to consume any at all of the resource. ---
---@param resourceName string
---@param amount number
---@return boolean
function status.overConsumeResource(resourceName, amount) end

--- Returns whether the resource is currently locked. ---
---@param resourceName string
---@return boolean
function status.resourceLocked(resourceName) end

--- Sets a resource to be locked/unlocked. A locked resource cannot be consumed. ---
---@param resourceName string
---@param locked boolean
---@return void
function status.setResourceLocked(resourceName, locked) end

--- Resets a resource to its base value. ---
---@param resourceName string
---@return void
function status.resetResource(resourceName) end

--- Resets all resources to their base values. ---
---@return void
function status.resetAllResources() end

--- Returns the max value for the specified resource. ---
---@param resourceName string
---@return number
function status.resourceMax(resourceName) end

--- Returns the percentage of max that the resource is currently at. From 0.0 to 1.0. ---
---@param resourceName string
---@return number
function status.resourcePercentage(resourceName) end

--- Sets a resource to a percentage of the max value for the resource. From 0.0 to 1.0. ---
---@param resourceName string
---@param value number
---@return void
function status.setResourcePercentage(resourceName, value) end

--- Adds a percentage of the max resource value to the current value of the resource. ---
---@param resourceName string
---@param value number
---@return void
function status.modifyResourcePercentage(resourceName, value) end

--- Returns a list of the currently active persistent effects in the specified effect category. ---
---@param effectCategory string
---@return JsonArray
function status.getPersistentEffects(effectCategory) end

--- Adds a status effect to the specified effect category. ---
---@param effectCategory string
---@param effect Json
---@return void
function status.addPersistentEffect(effectCategory, effect) end

--- Adds a list of effects to the specified effect category. ---
---@param effectCategory string
---@param effects JsonArray
---@return void
function status.addPersistentEffects(effectCategory, effects) end

--- Sets the list of effects of the specified effect category. Replaces the current list active effects. ---
---@param effectCategory string
---@param effects JsonArray
---@return void
function status.setPersistentEffects(effectCategory, effects) end

--- Clears any status effects from the specified effect category. ---
---@param effectCategory string
---@return void
function status.clearPersistentEffects(effectCategory) end

--- Clears all persistent status effects from all effect categories. ---
---@return void
function status.clearAllPersistentEffects() end

--- Adds the specified unique status effect. Optionally with a custom duration, and optionally with a source entity id accessible in the status effect. ---
---@param effectName string
---@param duration number
---@param sourceEntity EntityId
---@return void
function status.addEphemeralEffect(effectName, duration, sourceEntity) end

--- Adds a list of unique status effects. Optionally with a source entity id. Unique status effects can be specified either as a string, "myuniqueeffect", or as a table, {effect = "myuniqueeffect", duration = 5}. Remember that this function takes a `list` of these effect descriptors. This is a valid list of effects: { "myuniqueeffect", {effect = "myothereffect", duration = 5} } ---
---@param effects JsonArray
---@param sourceEntity EntityId
---@return void
function status.addEphemeralEffects(effects, sourceEntity) end

--- Removes the specified unique status effect. ---
---@param effectName string
---@return void
function status.removeEphemeralEffect(effectName) end

--- Clears all ephemeral status effects. ---
---@return void
function status.clearEphemeralEffects() end

--- Returns a list of two element tables describing all unique status effects currently active on the status controller. Each entry consists of the `String` name of the effect and a `float` between 0 and 1 indicating the remaining portion of that effect's duration. ---
---@return JsonArray
function status.activeUniqueStatusEffectSummary() end

--- Returns `true` if the specified unique status effect is currently active and `false` otherwise. ---
---@param effectName string
---@return boolean
function status.uniqueStatusEffectActive(effectName) end

--- Returns the primary set of image processing directives applied to the animation of the entity using this status controller. ---
---@return string
function status.primaryDirectives() end

--- Sets the primary set of image processing directives that should be applied to the animation of the entity using this status controller. ---
---@param directives string
---@return void
function status.setPrimaryDirectives(directives) end

--- Directly applies the specified damage request to the entity using this status controller.
---@param damageRequest DamageRequest
---@return void
function status.applySelfDamageRequest(damageRequest) end
