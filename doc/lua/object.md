The object table contains bindings specific to objects which are available in addition to their common tables.

---

#### `String` object.name()

Returns the object's type name.

---

#### `int` object.direction()

Returns the object's facing direction. This will be 1 for right or -1 for left.

---

#### `Vec2F` object.position()

Returns the object's tile position. This is identical to entity.position(), so use that instead.

---

#### `void` object.setInteractive(`bool` interactive)

Sets whether the object is currently interactive.

---

#### `String` object.uniqueId()

Returns the object's unique entity id, or `nil` if no unique id is set. This should be identical to entity.uniqueId(), so use that instead.

---

#### `void` object.setUniqueId([`String` uniqueId])

Sets the objects unique entity id, or clears it if unspecified.

---

#### `RectF` object.boundBox()

Returns the object's metaBoundBox in world space.

---

#### `List<Vec2I>` object.spaces()

Returns a list of the tile spaces that the object occupies.

---

#### `void` object.setProcessingDirectives(`String` directives)

Sets the image processing directives that should be applied to the object's animation.

---

#### `void` object.setSoundEffectEnabled(`bool` enabled)

Enables or disables the object's persistent sound effect, if one is configured.

---

#### `void` object.smash([`bool` smash])

Breaks the object. If smash is `true` then it will be smashed, causing it to (by default) drop no items.

---

#### `float` object.level()

Returns the "level" parameter if set, otherwise returns the current world's threat level.

---

#### `Vec2F` object.toAbsolutePosition(`Vec2F` relativePosition)

Returns an absolute world position calculated from the given relative position.

---

#### `bool` object.say(`String` line, [`Map<String, String>` tags], [`Json` config])

Causes the object to say the line, optionally replacing any specified tags in the text, and using the provided additional chat configuration. Returns `true` if anything is said (i.e. the line is not empty) and `false` otherwise.

---

#### `bool` object.sayPortrait(`String` line, `String` portrait, [`Map<String, String>` tags], [`Json` config])

Similar to object.say, but uses a portrait chat bubble with the specified portrait image.

---

#### `bool` object.isTouching(`EntityId` entityId)

Returns `true` if the specified entity's collision area overlaps the object's bound box and `false` otherwise.

---

#### `void` object.setLightColor(`Color` color)

Sets the color of light for the object to emit. This is not the same as animator.setLightColor and the animator light configuration should be used for more featureful light sources.

---

#### `Color` object.getLightColor()

Returns the object's currently configured light color.

---

#### `unsigned` object.inputNodeCount()

Returns the number of wire input nodes the object has.

---

#### `unsigned` object.outputNodeCount()

Returns the number of wire output nodes the object has.

---

#### `Vec2I` object.getInputNodePosition(`unsigned` nodeIndex)

Returns the relative position of the specified wire input node.

---

#### `Vec2I` object.getOutputNodePosition(`unsigned` nodeIndex)

Returns the relative position of the specified wire output node.

---

#### `bool` object.getInputNodeLevel(`unsigned` nodeIndex)

Returns the current level of the specified wire input node.

---

#### `bool` object.getOutputNodeLevel(`unsigned` nodeIndex)

Returns the current level of the specified wire output node.

---

#### `bool` object.isInputNodeConnected(`unsigned` nodeIndex)

Returns `true` if any wires are currently connected to the specified wire input node and `false` otherwise.

---

#### `bool` object.isOutputNodeConnected(`unsigned` nodeIndex)

Returns `true` if any wires are currently connected to the specified wire output node and `false` otherwise

---

#### `Map<EntityId, unsigned>` object.getInputNodeIds(`unsigned` nodeIndex)

Returns a map of the entity id of each wire entity connected to the given wire input node and the index of that entity's output node to which the input node is connected.

---

#### `Map<EntityId, unsigned>` object.getOutputNodeIds(`unsigned` nodeIndex)

Returns a map of the entity id of each wire entity connected to the given wire output node and the index of that entity's input node to which the output node is connected.

---

#### `void` object.setOutputNodeLevel(`unsigned` nodeIndex, `bool` level)

Sets the level of the specified wire output node.

---

#### `void` object.setAllOutputNodes(`bool` level)

Sets the level of all wire output nodes.

---

#### `void` object.setOfferedQuests([`JsonArray` quests])

Sets the list of quests that the object will offer to start, or clears them if unspecified.

---

#### `void` object.setTurnInQuests([`JsonArray` quests])

Sets the list of quests that the object will accept turn-in for, or clears them if unspecified.

---

#### `void` object.setConfigParameter(`String` key, `Json` value)

Sets the specified override configuration parameter for the object.

---

#### `void` object.setAnimationParameter(`String` key, `Json` value)

Sets the specified animation parameter for the object's scripted animator.

---

#### `void` object.setMaterialSpaces([`JsonArray` spaces])

Sets the object's material spaces to the specified list, or clears them if unspecified. List entries should be in the form of `pair<Vec2I, String>` specifying the relative position and material name of materials to be set. __Objects should only set material spaces within their occupied tile spaces to prevent Bad Things TM from happening.__

---

#### `void` object.setDamageSources([`List<DamageSource>` damageSources])

Sets the object's active damage sources (or clears them if unspecified).

---

#### `float` object.health()

Returns the object's current health.

---

#### `void` object.setHealth(`float` health)

Sets the object's current health.
