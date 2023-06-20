# animator

The *animator* table contains functions that relate to an attached networked animator. Networked animators are found in:

* tech
* monsters
* vehicles
* status effects
* active items

---

#### `bool` animator.setAnimationState(`String` stateType, `String` State, `bool` startNew = false)

Sets an animation state. If startNew is true, restart the animation loop if it's already active. Returns whether the state was set.

---

#### `String` animator.animationState(`String` stateType)

Returns the current state for a state type.

---

#### `Json` animator.animationStateProperty(`String` stateType, `String` propertyName)

Returns the value of the specified property for a state type.

---

#### `void` animator.setGlobalTag(`String` tagName, `String` tagValue)

Sets a global animator tag. A global tag replaces any tag <tagName> with the specified tagValue across all animation parts.

---

#### `void` animator.setPartTag(`String` partType, `String` tagName, `String` tagValue)

Sets a local animator tag. A part tag replaces any tag <tagName> with the specified tagValue in the partType animation part only.

---

#### `void` animator.setFlipped(`bool` flipped)

Sets whether the animator should be flipped horizontally.

---

#### `void` animator.setAnimationRate(`float` rate)

Sets the animation rate of the animator.

---

#### `void` animator.rotateGroup(`String` rotationGroup, `float` targetAngle, `bool` immediate)

Rotates a rotation group to the specified angle. If immediate, ignore rotation speed.

*NOTE:* Rotation groups have largely been replaced by transformation groups and should only be used in a context where maintaining a rotation speed is important. When possible use transformation groups.

---

#### `float` animator.currentRotationAngle(`String` rotationGroup)

Returns the current angle for a rotation group.

---

#### `bool` animator.hasTransformationGroup(`String` transformationGroup)

Returns whether the animator contains the specified transformation group.

---

#### `void` animator.translateTransformationGroup(`String` transformationGroup, `Vec2F` translate)

Translates the specified transformation group.

---

#### `void` animator.rotateTransformationGroup(`String` transformationGroup, `float` rotation, [`Vec2F` rotationCenter])

Rotates the specified transformation group by the specified angle in radians, optionally around the specified center point.

---

#### `void` animator.scaleTransformationGroup(`String` transformationGroup, `float` scale, [`Vec2F` scaleCenter])

#### `void` animator.scaleTransformationGroup(`String` transformationGroup, `Vec2F` scale, [`Vec2F` scaleCenter])

Scales the specified transformation group by the specified scale. Optionally scale it from a scaleCenter.

---

#### `void` animator.transformTransformationGroup(`String` transformationGroup, `float` a, `float` b, `float` c, `float` d, `float` tx, `float` ty)

Applies a custom Mat3 transform to the specified transformationGroup. The applied matrix will be:

[a, b, tx,
 c, d, ty,
 0, 0, 1]

---

#### `void` animator.resetTransformationGroup(`String` transformationGroup)

Resets a transformationGroup to the identity transform.

[1, 0, 0
 0, 1, 0,
 0, 1, 1]

---

#### `void` animator.setParticleEmitterActive(`String` emitterName, `bool` active)

Sets a particle emitter to be active or inactive.

---

#### `void` animator.setParticleEmitterEmissionRate(`String` emitterName, `float` emissionRate)

Sets the rate at which a particle emitter emits particles while active.

---

#### `void` animator.setParticleEmitterBurstCount(`String` emitterName, `unsigned` burstCount)

Sets the amount of each particle the emitter will emit when using burstParticleEmitter.

---

#### `void` animator.setParticleEmitterOffsetRegion(`String` emitterName, `RectF` offsetRegion)

Sets an offset region for the particle emitter. Any particles spawned will have a randomized offset within the region added to their position.

---

#### `void` animator.burstParticleEmitter(`String` emitterName)

Spawns the entire set of particles `burstCount` times, where `burstCount` can be configured in the animator or set by setParticleEmitterBurstCount.

---

#### `void` animator.setLightActive(`String` lightName, bool active)

Sets a light to be active/inactive.

---

#### `void` animator.setLightPosition(`String` lightName, Vec2F position)

Sets the position of a light.

---

#### `void` animator.setLightColor(`String` lightName, Color color)

Sets the color of a light. Brighter color gives a higher light intensity.

---

#### `void` animator.setLightPointAngle(`String` lightName, float angle)

Sets the angle of a pointLight.

---

#### `bool` animator.hasSound(`String` soundName)

Returns whether the animator has a sound by the name of `soundName`

---

#### `void` animator.setSoundPool(`String` soundName, `List<String>` soundPool)

Sets the list of sound assets to pick from when playing a sound.

---

#### `void` animator.setSoundPosition(`String` soundName, `Vec2F` position)

Sets the position that a sound is played at.

---

#### `void` animator.playSound(`String` soundName, [`int` loops = 0])

Plays a sound. Optionally loop `loops` times. 0 plays the sound once (no loops), -1 loops indefinitely.

---

#### `void` animator.setSoundVolume(`String` soundName, `float` volume, [`float` rampTime = 0.0])

Sets the volume of a sound. Optionally smoothly transition the volume over `rampTime` seconds.

---

#### `void` animator.setSoundPitch(`String` soundName, `float` pitch, [`float` rampTime = 0.0])

Sets the relative pitch of a sound. Optionally smoothly transition the pitch over `rampTime` seconds.

---

#### `void` animator.stopAllSounds(`String` soundName)

Stops all instances of the specified sound.

---

#### `void` animator.setEffectActive(`String` effect, `bool` enabled)

Sets a configured effect to be active/inactive.

---

#### `Vec2F` animator.partPoint(`String` partName, `String` propertyName)

Returns a `Vec2F` configured in a part's properties with all of the part's transformations applied to it.

---

#### `PolyF` animator.partPoly(`String` partName, `String` propertyName)

Returns a `PolyF` configured in a part's properties with all the part's transformations applied to it.

---

#### `Json` animator.partProperty(`String` partName, `String` propertyName)

Returns an animation part property without applying any transformations.

---

#### `Json` animator.transformPoint(`String` partName, `Vec2F` point)

Applies the specified part's transformation on the given point.

---

#### `Json` animator.transformPoly(`String` partName, `PolyF` poly)

Applies the specified part's transformation on the given poly.
