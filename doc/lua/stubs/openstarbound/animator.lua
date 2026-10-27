---@meta

--- animator API
---@class animator
animator = {}

--- True if the animator is flipped.
---@return boolean
function animator.flipped() end

--- Returns the center line the animator was flipped at. ---
---@return number
function animator.flippedRelativeCenterLine() end

--- Returns the animation rate. ---
---@return number
function animator.animationRate() end

--- Sets a global animator tag. A global tag replaces any tag <tagName> with the specified tagValue across all animation parts. If tagValue is nothing, then the tag will be removed, empty global tags default to `default`.
---@param tagName string
---@param tagValue Maybe<String>
---@return void
function animator.setGlobalTag(tagName, tagValue) end

--- Sets a local animator tag. A part tag replaces any tag <tagName> with the specified tagValue in the partType animation part only. If tagValue is nothing, then the tag will be removed, therefore reverting back to inheriting the global tag, empty global tags default to `default`.
---@param partType string
---@param tagName string
---@param tagValue Maybe<String>
---@return void
function animator.setPartTag(partType, tagName, tagValue) end

--- Sets an animator tag. A tag replaces any tag <tagName> with the specified tagValue across all animation parts. If tagValue is nothing, then the tag will be removed, inheriting from part and global tags again, empty global tags default to `default`. Local tags are not networked, they are intended for use in client animation scripts. --- ### `String` animator.applyPartTags(`String` part, `String` input) Returns the input string with the animation tags for the specified part applied to it. ---
---@param tagName string
---@param tagValue Maybe<String>
---@return void
function animator.setLocalTag(tagName, tagValue) end

--- Returns the value of the specified property for a state type. If **state** and **frame** are supplied, the value for that specific state and frame is returned, even if that state is not active.
---@param stateType string
---@param propertyName string
---@param state Maybe<String>
---@param frame Maybe<int>
---@return Json
function animator.animationStateProperty(stateType, propertyName, state, frame) end

--- Returns the value of the specified property of the next frame for a state type. ---
---@param stateType string
---@param propertyName string
---@return Json
function animator.animationStateNextProperty(stateType, propertyName) end

--- Returns an animation part property without applying any transformations. If **stateType**, **state**, and **frame** are supplied, the value for that specific state and frame is returned, even if that state is not active.
---@param partName string
---@param propertyName string
---@param stateType Maybe<String>
---@param state Maybe<String>
---@param frame Maybe<int>
---@return Json
function animator.partProperty(partName, propertyName, stateType, state, frame) end

--- Returns an animation part property of the next frame without applying any transformations. ---
---@param partName string
---@param propertyName string
---@return Json
function animator.partNextProperty(partName, propertyName) end

--- Sets an animation state. If startNew is true, restart the animation loop if it's already active. Returns whether the state was set. If reverse is true, the animation will play backwards, starting at the final frame and decrementing towards the first.
---@param stateType string
---@param State string
---@param startNew boolean
---@param reverse boolean
---@return boolean
function animator.setAnimationState(stateType, State, startNew, reverse) end

--- Sets an animation state locally. If startNew is true, restart the animation loop if it's already active. Returns whether the state was set. If reverse is true, the animation will play backwards, starting at the final frame and decrementing towards the first. Does not trigger a networking update. This is intended to be used in client animation scripts. ---
---@param stateType string
---@param State string
---@param startNew boolean
---@param reverse boolean
---@return boolean
function animator.setLocalAnimationState(stateType, State, startNew, reverse) end

--- Returns the current frame index of the sateType.
---@param stateType string
---@return number
function animator.animationStateFrame(stateType) end

--- Returns the next frame index of the sateType.
---@param stateType string
---@return number
function animator.animationStateNextFrame(stateType) end

--- Returns percentage of progress for the current frame of the stateType.
---@param stateType string
---@return number
function animator.animationStateFrameProgress(stateType) end

--- Returns timer for the current animation of the stateType.
---@param stateType string
---@return number
function animator.animationStateTimer(stateType) end

--- Returns percentage of progress for the current frame of the stateType.
---@param stateType string
---@return number
function animator.animationStateFrameProgress(stateType) end

--- Returns timer for the current animation of the stateType.
---@param stateType string
---@return number
function animator.animationStateTimer(stateType) end

--- Returns true if the stateType is playing in reverse. ---
---@param stateType string
---@return boolean
function animator.animationStateReverse(stateType) end

--- Returns true if the animator has the stateType. If **state** is valid, then it will only return true if the state also exists. ---
---@param stateType string
---@param state Maybe<String>
---@return boolean
function animator.hasState(stateType, state) end

--- Returns the animation cycle of the current animation for stateType. If **state** is valid, then it will return the animation cycle of that state instead.
---@param stateType string
---@param state Maybe<String>
---@return number
function animator.stateCycle(stateType, state) end

--- Returns the number of frames of the current animation for stateType. If **state** is valid, then it will return the number of frames of that state instead. ---
---@param stateType string
---@param state Maybe<String>
---@return number
function animator.stateFrames(stateType, state) end

--- Rotates the specified transformation group by the specified angle in degrees, optionally around the specified center point.
---@param transformationGroup string
---@param rotation number
---@param rotationCenter Vec2F
---@return void
function animator.rotateDegreesTransformationGroup(transformationGroup, rotation, rotationCenter) end

--- Overwrites the current Mat3F transformation with the supplied one.
---@param transformationGroup string
---@param transformation Mat3F
---@return void
function animator.setTransformationGroup(transformationGroup, transformation) end

--- Returns the current Mat3F transformation. ---
---@param transformationGroup string
---@return Mat3F
function animator.getTransformationGroup(transformationGroup) end

---@param transformationGroup string
---@param translate Vec2F
---@return void
function animator.translateLocalTransformationGroup(transformationGroup, translate) end

---@param transformationGroup string
---@param rotation number
---@param rotationCenter Vec2F
---@return void
function animator.rotateLocalTransformationGroup(transformationGroup, rotation, rotationCenter) end

---@param transformationGroup string
---@param rotation number
---@param rotationCenter Vec2F
---@return void
function animator.rotateDegreesLocalTransformationGroup(transformationGroup, rotation, rotationCenter) end

---@param transformationGroup string
---@param scale number
---@param scaleCenter Vec2F
---@return void
function animator.scaleLocalTransformationGroup(transformationGroup, scale, scaleCenter) end

---@param transformationGroup string
---@param scale Vec2F
---@param scaleCenter Vec2F
---@return void
function animator.scaleLocalTransformationGroup(transformationGroup, scale, scaleCenter) end

---@param transformationGroup string
---@param a number
---@param b number
---@param c number
---@param d number
---@param tx number
---@param ty number
---@return void
function animator.transformLocalTransformationGroup(transformationGroup, a, b, c, d, tx, ty) end

---@param transformationGroup string
---@return void
function animator.resetLocalTransformationGroup(transformationGroup) end

---@param transformationGroup string
---@param transformation Mat3F
---@return void
function animator.setLocalTransformationGroup(transformationGroup, transformation) end

--- Local transformations for a group are added onto the networked counterparts, they do not replace them. These are intended for client animation scripts. As when using the networked transformation groups, transformations often become visually desynced with the frame timing on clients. ---
---@param transformationGroup string
---@return Mat3F
function animator.getLocalTransformationGroup(transformationGroup) end

--- Adds the list of drawables attached to the specified part. These drawables inherit all transformations from the part they are attached to. These are not networked, intended for use in client animation scripts.
---@param part string
---@param drawables List<Drawable>
---@return void
function animator.addPartDrawables(part, drawables) end

--- Overwrites the list of drawables attached to the specified part. These drawables inherit all transformations from the part they are attached to. These are not networked, intended for use in client animation scripts. ---
---@param part string
---@param drawables List<Drawable>
---@return void
function animator.setPartDrawables(part, drawables) end
