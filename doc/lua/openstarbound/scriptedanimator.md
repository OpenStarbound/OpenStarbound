# animationConfig

The `animationConfig` table contains functions for getting configuration options from the base entity and its networked animationConfig.

It is available only in client side rendering scripts.

---

#### `bool` animationConfig.flipped()
True if the animator is flipped.

#### `float` animationConfig.flippedRelativeCenterLine()
Returns the center line the animator was flipped at.

---

#### `float` animationConfig.animationRate()
Returns the animation rate.

---

#### `void` animationConfig.setLocalTag(`String` tagName, `Maybe<String>` tagValue)

Sets an animator tag. A tag replaces any tag <tagName> with the specified tagValue across all animation parts.

If tagValue is nothing, then the tag will be removed, inheriting from part and global tags again, empty global tags default to `default`.

Local tags are not networked, they are intended for use in client animation scripts.

---

### `String` animationConfig.applyPartTags(`String` part, `String` input)

Returns the input string with the animation tags for the specified part applied to it.

---

#### `Json` animationConfig.animationStateProperty(`String` stateType, `String` propertyName, `Maybe<String>` state, `Maybe<int>` frame)

Returns the value of the specified property for a state type.

If **state** and **frame** are supplied, the value for that specific state and frame is returned, even if that state is not active.

#### `Json` animationConfig.animationStateNextProperty(`String` stateType, `String` propertyName)

Returns the value of the specified property of the next frame for a state type.

---

#### `Json` animationConfig.partProperty(`String` partName, `String` propertyName, `Maybe<String>` stateType, `Maybe<String>` state, `Maybe<int>` frame)

Returns an animation part property without applying any transformations.

If **stateType**, **state**, and **frame** are supplied, the value for that specific state and frame is returned, even if that state is not active.

#### `Json` animationConfig.partNextProperty(`String` partName, `String` propertyName)

Returns an animation part property of the next frame without applying any transformations.

---

#### `bool` animationConfig.setLocalAnimationState(`String` stateType, `String` State, `bool` startNew = false, `bool` reverse = false)

Sets an animation state locally. If startNew is true, restart the animation loop if it's already active. Returns whether the state was set.

If reverse is true, the animation will play backwards, starting at the final frame and decrementing towards the first.

Does not trigger a networking update. This is intended to be used in client animation scripts.

---

#### `String` animationConfig.animationState(`String` stateType)

Returns the current state for a state type.

#### `int` animationConfig.animationStateFrame(`String` stateType)

Returns the current frame index of the sateType.

#### `int` animationConfig.animationStateNextFrame(`String` stateType)

Returns the next frame index of the sateType.

#### `float` animationConfig.animationStateFrameProgress(`String` stateType)

Returns percentage of progress for the current frame of the stateType.

#### `float` animationConfig.animationStateTimer(`String` stateType)

Returns timer for the current animation of the stateType.

#### `float` animationConfig.animationStateFrameProgress(`String` stateType)

Returns percentage of progress for the current frame of the stateType.

#### `float` animationConfig.animationStateTimer(`String` stateType)

Returns timer for the current animation of the stateType.

#### `bool` animationConfig.animationStateReverse(`String` stateType)

Returns true if the stateType is playing in reverse.

---

#### `bool` animationConfig.hasState(`String` stateType, `Maybe<String>` state)

Returns true if the animator has the stateType.

If **state** is valid, then it will only return true if the state also exists.

#### `bool` animationConfig.hasTransformationGroup(`String` transformationGroup)

Returns whether the animator contains the specified transformation group.

---

#### `float` animationConfig.stateCycle(`String` stateType, `Maybe<String>` state)

Returns the animation cycle of the current animation for stateType.

If **state** is valid, then it will return the animation cycle of that state instead.

#### `int` animationConfig.stateFrames(`String` stateType, `Maybe<String>` state)

Returns the number of frames of the current animation for stateType.

If **state** is valid, then it will return the number of frames of that state instead.

---

#### `void` animationConfig.translateLocalTransformationGroup(`String` transformationGroup, `Vec2F` translate)
#### `void` animationConfig.rotateLocalTransformationGroup(`String` transformationGroup, `float` rotation, [`Vec2F` rotationCenter])
#### `void` animationConfig.rotateDegreesLocalTransformationGroup(`String` transformationGroup, `float` rotation, [`Vec2F` rotationCenter])
#### `void` animationConfig.scaleLocalTransformationGroup(`String` transformationGroup, `float` scale, [`Vec2F` scaleCenter])
#### `void` animationConfig.scaleLocalTransformationGroup(`String` transformationGroup, `Vec2F` scale, [`Vec2F` scaleCenter])
#### `void` animationConfig.transformLocalTransformationGroup(`String` transformationGroup, `float` a, `float` b, `float` c, `float` d, `float` tx, `float` ty)
#### `void` animationConfig.resetLocalTransformationGroup(`String` transformationGroup)
#### `void` animationConfig.setLocalTransformationGroup(`String` transformationGroup, `Mat3F` transformation)
#### `Mat3F` animationConfig.getLocalTransformationGroup(`String` transformationGroup)

Local transformations for a group are added onto the networked counterparts, they do not replace them.

These are intended for client animation scripts. As when using the networked transformation groups, transformations often become visually desynced with the frame timing on clients.

---
#### `void` animationConfig.addPartDrawables(`String` part, `List<Drawable>` drawables)

Adds the list of drawables attached to the specified part. These drawables inherit all transformations from the part they are attached to.

These are not networked, intended for use in client animation scripts.

#### `void` animationConfig.setPartDrawables(`String` part, `List<Drawable>` drawables)

Overwrites the list of drawables attached to the specified part. These drawables inherit all transformations from the part they are attached to.

These are not networked, intended for use in client animation scripts.

---
