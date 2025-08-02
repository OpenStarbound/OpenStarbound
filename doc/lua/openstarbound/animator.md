# animator

The *animator* table contains functions that relate to an attached networked animator. Networked animators are found in:

* tech
* monsters
* vehicles
* status effects
* active items

In OSB, If a player/npc's humanoid config contains an `animation`, then they will use that instead of the hardcoded humanoid animations.

* NPCs get the animator callbacks in their main scripts.
* Players get access to the animator callbacks in their generic script contexts.

Animator callbacks will be added and removed from player/npc script contexts depending on whether the active humanoid config uses an animation or not.

---

#### `void` animator.setGlobalTag(`String` tagName, `Maybe<String>` tagValue)

Sets a global animator tag. A global tag replaces any tag <tagName> with the specified tagValue across all animation parts.

If tagValue is nothing, then the tag will be removed, empty global tags default to `default`.

#### `void` animator.setPartTag(`String` partType, `String` tagName, `Maybe<String>` tagValue)

Sets a local animator tag. A part tag replaces any tag <tagName> with the specified tagValue in the partType animation part only.

If tagValue is nothing, then the tag will be removed, therefore reverting back to inheriting the global tag, empty global tags default to `default`.

#### `void` animator.setLocalTag(`String` tagName, `Maybe<String>` tagValue)

Sets an animator tag. A tag replaces any tag <tagName> with the specified tagValue across all animation parts.

If tagValue is nothing, then the tag will be removed, inheriting from part and global tags again, empty global tags default to `default`.

Local tags are not networked, they are intended for use in client animation scripts.

---

### `String` animator.applyPartTags(`String` part, `String` input)

Returns the input string with the animation tags for the specified part applied to it.

---

#### `Json` animator.animationStateProperty(`String` stateType, `String` propertyName, `Maybe<String>` state, `Maybe<int>` frame)

Returns the value of the specified property for a state type.

If **state** and **frame** are supplied, the value for that specific state and frame is returned, even if that state is not active.

#### `Json` animator.animationStateNextProperty(`String` stateType, `String` propertyName)

Returns the value of the specified property of the next frame for a state type.

---

#### `Json` animator.partProperty(`String` partName, `String` propertyName, `Maybe<String>` stateType, `Maybe<String>` state, `Maybe<int>` frame)

Returns an animation part property without applying any transformations.

If **stateType**, **state**, and **frame** are supplied, the value for that specific state and frame is returned, even if that state is not active.

#### `Json` animator.partNextProperty(`String` partName, `String` propertyName)

Returns an animation part property of the next frame without applying any transformations.

---

#### `bool` animator.setAnimationState(`String` stateType, `String` State, `bool` startNew = false, `bool` reverse = false)

Sets an animation state. If startNew is true, restart the animation loop if it's already active. Returns whether the state was set.

If reverse is true, the animation will play backwards, starting at the final frame and decrementing towards the first.

#### `bool` animator.setLocalAnimationState(`String` stateType, `String` State, `bool` startNew = false, `bool` reverse = false)

Sets an animation state locally. If startNew is true, restart the animation loop if it's already active. Returns whether the state was set.

If reverse is true, the animation will play backwards, starting at the final frame and decrementing towards the first.

Does not trigger a networking update. This is intended to be used in client animation scripts.

---

#### `int` animator.animationStateFrame(`String` stateType)

Returns the current frame index of the sateType.

#### `int` animator.animationStateNextFrame(`String` stateType)

Returns the next frame index of the sateType.

#### `float` animator.animationStateFrameProgress(`String` stateType)

Returns percentage of progress for the current frame of the stateType.

#### `float` animator.animationStateTimer(`String` stateType)

Returns timer for the current animation of the stateType.

#### `float` animator.animationStateFrameProgress(`String` stateType)

Returns percentage of progress for the current frame of the stateType.

#### `float` animator.animationStateTimer(`String` stateType)

Returns timer for the current animation of the stateType.

#### `bool` animator.animationStateReverse(`String` stateType)

Returns true if the stateType is playing in reverse.

---

#### `bool` animator.hasState(`String` stateType, `Maybe<String>` state)

Returns true if the animator has the stateType.

If **state** is valid, then it will only return true if the state also exists.

---

#### `float` animator.stateCycle(`String` stateType, `Maybe<String>` state)

Returns the animation cycle of the current animation for stateType.

If **state** is valid, then it will return the animation cycle of that state instead.

#### `int` animator.stateFrames(`String` stateType, `Maybe<String>` state)

Returns the number of frames of the current animation for stateType.

If **state** is valid, then it will return the number of frames of that state instead.

---

#### `void` animator.rotateDegreesTransformationGroup(`String` transformationGroup, `float` rotation, [`Vec2F` rotationCenter])

Rotates the specified transformation group by the specified angle in degrees, optionally around the specified center point.

#### `void` animator.setTransformationGroup(`String` transformationGroup, `Mat3F` transformation)

Overwrites the current Mat3F transformation with the supplied one.

#### `Mat3F` animator.getTransformationGroup(`String` transformationGroup)

Returns the current Mat3F transformation.

---

#### `void` animator.translateLocalTransformationGroup(`String` transformationGroup, `Vec2F` translate)
#### `void` animator.rotateLocalTransformationGroup(`String` transformationGroup, `float` rotation, [`Vec2F` rotationCenter])
#### `void` animator.rotateDegreesLocalTransformationGroup(`String` transformationGroup, `float` rotation, [`Vec2F` rotationCenter])
#### `void` animator.scaleLocalTransformationGroup(`String` transformationGroup, `float` scale, [`Vec2F` scaleCenter])
#### `void` animator.scaleLocalTransformationGroup(`String` transformationGroup, `Vec2F` scale, [`Vec2F` scaleCenter])
#### `void` animator.transformLocalTransformationGroup(`String` transformationGroup, `float` a, `float` b, `float` c, `float` d, `float` tx, `float` ty)
#### `void` animator.resetLocalTransformationGroup(`String` transformationGroup)
#### `void` animator.setLocalTransformationGroup(`String` transformationGroup, `Mat3F` transformation)
#### `Mat3F` animator.getLocalTransformationGroup(`String` transformationGroup)

Local transformations for a group are added onto the networked counterparts, they do not replace them.

These are intended for client animation scripts. As when using the networked transformation groups, transformations often become visually desynced with the frame timing on clients.

---

#### `void` animator.addPartDrawables(`String` part, `List<Drawable>` drawables)

Adds the list of drawables attached to the specified part. These drawables inherit all transformations from the part they are attached to.

These are not networked, intended for use in client animation scripts.

#### `void` animator.setPartDrawables(`String` part, `List<Drawable>` drawables)

Overwrites the list of drawables attached to the specified part. These drawables inherit all transformations from the part they are attached to.

These are not networked, intended for use in client animation scripts.

---
