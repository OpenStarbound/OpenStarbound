---@meta

--- animator API
---@class animator
animator = {}

--- Sets an animation state. If startNew is true, restart the animation loop if it's already active. Returns whether the state was set. ---
---@param stateType string
---@param State string
---@param startNew boolean
---@return boolean
function animator.setAnimationState(stateType, State, startNew) end

--- Returns the current state for a state type. ---
---@param stateType string
---@return string
function animator.animationState(stateType) end

--- Returns the value of the specified property for a state type. ---
---@param stateType string
---@param propertyName string
---@return Json
function animator.animationStateProperty(stateType, propertyName) end

--- Sets a global animator tag. A global tag replaces any tag <tagName> with the specified tagValue across all animation parts. ---
---@param tagName string
---@param tagValue string
---@return void
function animator.setGlobalTag(tagName, tagValue) end

--- Sets a local animator tag. A part tag replaces any tag <tagName> with the specified tagValue in the partType animation part only. ---
---@param partType string
---@param tagName string
---@param tagValue string
---@return void
function animator.setPartTag(partType, tagName, tagValue) end

--- Sets whether the animator should be flipped horizontally. ---
---@param flipped boolean
---@return void
function animator.setFlipped(flipped) end

--- Sets the animation rate of the animator. ---
---@param rate number
---@return void
function animator.setAnimationRate(rate) end

--- Rotates a rotation group to the specified angle. If immediate, ignore rotation speed. *NOTE:* Rotation groups have largely been replaced by transformation groups and should only be used in a context where maintaining a rotation speed is important. When possible use transformation groups. ---
---@param rotationGroup string
---@param targetAngle number
---@param immediate boolean
---@return void
function animator.rotateGroup(rotationGroup, targetAngle, immediate) end

--- Returns the current angle for a rotation group. ---
---@param rotationGroup string
---@return number
function animator.currentRotationAngle(rotationGroup) end

--- Returns whether the animator contains the specified transformation group. ---
---@param transformationGroup string
---@return boolean
function animator.hasTransformationGroup(transformationGroup) end

--- Translates the specified transformation group. ---
---@param transformationGroup string
---@param translate Vec2F
---@return void
function animator.translateTransformationGroup(transformationGroup, translate) end

--- Rotates the specified transformation group by the specified angle in radians, optionally around the specified center point. ---
---@param transformationGroup string
---@param rotation number
---@param rotationCenter Vec2F
---@return void
function animator.rotateTransformationGroup(transformationGroup, rotation, rotationCenter) end

---@param transformationGroup string
---@param scale number
---@param scaleCenter Vec2F
---@return void
function animator.scaleTransformationGroup(transformationGroup, scale, scaleCenter) end

--- Scales the specified transformation group by the specified scale. Optionally scale it from a scaleCenter. ---
---@param transformationGroup string
---@param scale Vec2F
---@param scaleCenter Vec2F
---@return void
function animator.scaleTransformationGroup(transformationGroup, scale, scaleCenter) end

--- Applies a custom Mat3 transform to the specified transformationGroup. The applied matrix will be: [a, b, tx, c, d, ty, 0, 0, 1] ---
---@param transformationGroup string
---@param a number
---@param b number
---@param c number
---@param d number
---@param tx number
---@param ty number
---@return void
function animator.transformTransformationGroup(transformationGroup, a, b, c, d, tx, ty) end

--- Resets a transformationGroup to the identity transform. [1, 0, 0 0, 1, 0, 0, 1, 1] ---
---@param transformationGroup string
---@return void
function animator.resetTransformationGroup(transformationGroup) end

--- Sets a particle emitter to be active or inactive. ---
---@param emitterName string
---@param active boolean
---@return void
function animator.setParticleEmitterActive(emitterName, active) end

--- Sets the rate at which a particle emitter emits particles while active. ---
---@param emitterName string
---@param emissionRate number
---@return void
function animator.setParticleEmitterEmissionRate(emitterName, emissionRate) end

--- Sets the amount of each particle the emitter will emit when using burstParticleEmitter. ---
---@param emitterName string
---@param burstCount unsigned
---@return void
function animator.setParticleEmitterBurstCount(emitterName, burstCount) end

--- Sets an offset region for the particle emitter. Any particles spawned will have a randomized offset within the region added to their position. ---
---@param emitterName string
---@param offsetRegion RectF
---@return void
function animator.setParticleEmitterOffsetRegion(emitterName, offsetRegion) end

--- Spawns the entire set of particles `burstCount` times, where `burstCount` can be configured in the animator or set by setParticleEmitterBurstCount. ---
---@param emitterName string
---@return void
function animator.burstParticleEmitter(emitterName) end

--- Sets a light to be active/inactive. ---
---@param lightName string
---@return void
function animator.setLightActive(lightName) end

--- Sets the position of a light. ---
---@param lightName string
---@return void
function animator.setLightPosition(lightName) end

--- Sets the color of a light. Brighter color gives a higher light intensity. ---
---@param lightName string
---@return void
function animator.setLightColor(lightName) end

--- Sets the angle of a pointLight. ---
---@param lightName string
---@return void
function animator.setLightPointAngle(lightName) end

--- Returns whether the animator has a sound by the name of `soundName` ---
---@param soundName string
---@return boolean
function animator.hasSound(soundName) end

--- Sets the list of sound assets to pick from when playing a sound. ---
---@param soundName string
---@param soundPool List<String>
---@return void
function animator.setSoundPool(soundName, soundPool) end

--- Sets the position that a sound is played at. ---
---@param soundName string
---@param position Vec2F
---@return void
function animator.setSoundPosition(soundName, position) end

--- Plays a sound. Optionally loop `loops` times. 0 plays the sound once (no loops), -1 loops indefinitely. ---
---@param soundName string
---@param loops number
---@return void
function animator.playSound(soundName, loops) end

--- Sets the volume of a sound. Optionally smoothly transition the volume over `rampTime` seconds. ---
---@param soundName string
---@param volume number
---@param rampTime number
---@return void
function animator.setSoundVolume(soundName, volume, rampTime) end

--- Sets the relative pitch of a sound. Optionally smoothly transition the pitch over `rampTime` seconds. ---
---@param soundName string
---@param pitch number
---@param rampTime number
---@return void
function animator.setSoundPitch(soundName, pitch, rampTime) end

--- Stops all instances of the specified sound. ---
---@param soundName string
---@return void
function animator.stopAllSounds(soundName) end

--- Sets a configured effect to be active/inactive. ---
---@param effect string
---@param enabled boolean
---@return void
function animator.setEffectActive(effect, enabled) end

--- Returns a `Vec2F` configured in a part's properties with all of the part's transformations applied to it. ---
---@param partName string
---@param propertyName string
---@return Vec2F
function animator.partPoint(partName, propertyName) end

--- Returns a `PolyF` configured in a part's properties with all the part's transformations applied to it. ---
---@param partName string
---@param propertyName string
---@return PolyF
function animator.partPoly(partName, propertyName) end

--- Returns an animation part property without applying any transformations. ---
---@param partName string
---@param propertyName string
---@return Json
function animator.partProperty(partName, propertyName) end

--- Applies the specified part's transformation on the given point. ---
---@param partName string
---@param point Vec2F
---@return Json
function animator.transformPoint(partName, point) end

--- Applies the specified part's transformation on the given poly.
---@param partName string
---@param poly PolyF
---@return Json
function animator.transformPoly(partName, poly) end
