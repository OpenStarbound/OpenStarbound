The activeItem table contains bindings which provide functionality for the ActiveItem and for the item's 'owner' (a ToolUser entity currently holding the item).

---

#### `EntityId` activeItem.ownerEntityId()

Returns the entity id of the owner entity.

---

#### `DamageTeam` activeItem.ownerTeam()

Returns the damage team of the owner entity.

---

#### `Vec2F` activeItem.ownerAimPosition()

Returns the world aim position of the owner entity.

---

#### `float` activeItem.ownerPowerMultiplier()

Returns the power multiplier of the owner entity.

---

#### `String` activeItem.fireMode()

Returns the current fire mode of the item, which can be "none", "primary" or "alt". Single-handed items held in the off hand will receive right click as "primary" rather than "alt".

---

#### `String` activeItem.hand()

Returns the name of the hand that the item is currently held in, which can be "primary" or "alt".

---

#### `Vec2F` activeItem.handPosition([`Vec2F` offset])

Takes an input position (defaults to [0, 0]) relative to the item and returns a position relative to the owner entity.

---

#### `LuaTable` activeItem.aimAngleAndDirection(`float` aimVerticalOffset, `Vec2F` targetPosition)

Returns a table containing the `float` aim angle and `int` facing direction that would be used for the item to aim at the specified target position with the specified vertical offset. This takes into account the position of the shoulder, distance of the hand from the body, and a lot of other complex factors and should be used to control aimable weapons or tools based on the owner's aim position.

---

#### `float` activeItem.aimAngle(`float` aimVerticalOffset, `Vec2F` targetPosition)

Similar to activeItem.aimAngleAndDirection but only returns the aim angle that would be calculated with the entity's current facing direction. Necessary if, for example, an item needs to aim behind the owner.

---

#### `void` activeItem.setHoldingItem(`bool` holdingItem)

Sets whether the owner is visually holding the item.

---

#### `void` activeItem.setBackArmFrame([`String` armFrame])

Sets the arm image frame that the item should use when held behind the player, or clears it to the default rotation arm frame if no frame is specified.

---

#### `void` activeItem.setFrontArmFrame([`String` armFrame])

Sets the arm image frame that the item should use when held in front of the player, or clears it to the default rotation arm frame if no frame is specified.

---

#### `void` activeItem.setTwoHandedGrip(`bool` twoHandedGrip)

Sets whether the item should be visually held with both hands. Does not alter the functional handedness requirement of the item.

---

#### `void` activeItem.setRecoil(`bool` recoil)

Sets whether the item is in a recoil state, which will translate both the item and the arm holding it slightly toward the back of the character.

---

#### `void` activeItem.setOutsideOfHand(`bool` outsideOfHand)

Sets whether the item should be visually rendered outside the owner's hand. Items outside of the hand will be rendered in front of the arm when held in front and behind the arm when held behind.

---

#### `void` activeItem.setArmAngle(`float` angle)

Sets the angle to which the owner's arm holding the item should be rotated.

---

#### `void` activeItem.setFacingDirection(`float` direction)

Sets the item's requested facing direction, which controls the owner's facing. Positive direction values will face right while negative values will face left. If the owner holds two items which request opposing facing directions, the direction requested by the item in the primary hand will take precedence.

---

#### `void` activeItem.setDamageSources([`List<DamageSource>` damageSources])

Sets a list of active damage sources with coordinates relative to the owner's position or clears them if unspecified.

---

#### `void` activeItem.setItemDamageSources([`List<DamageSource>` damageSources])

Sets a list of active damage sources with coordinates relative to the item's hand position or clears them if unspecified.

---

#### `void` activeItem.setShieldPolys([`List<PolyF>` shieldPolys])

Sets a list of active shield polygons with coordinates relative to the owner's position or clears them if unspecified.

---

#### `void` activeItem.setItemShieldPolys([`List<PolyF>` shieldPolys])

Sets a list of active shield polygons with coordinates relative to the item's hand position or clears them if unspecified.

---

#### `void` activeItem.setForceRegions([`List<PhysicsForceRegion>` forceRegions])

Sets a list of active physics force regions with coordinates relative to the owner's position or clears them if unspecified.

---

#### `void` activeItem.setItemForceRegions([`List<PhysicsForceRegion>` forceRegions])

Sets a list of active physics force regions with coordinates relative to the item's hand position or clears them if unspecified.

---

#### `void` activeItem.setCursor([`String` cursor])

Sets the item's overriding cursor image or clears it if unspecified.

---

#### `void` activeItem.setScriptedAnimationParameter(`String` parameter, `Json` value)

Sets a parameter to be used by the item's scripted animator.

---

#### `void` activeItem.setInventoryIcon(`String` image)

Sets the inventory icon of the item.

---

#### `void` activeItem.setInstanceValue(`String` parameter, `Json` value)

Sets an instance value (parameter) of the item.

---

#### `LuaValue` activeItem.callOtherHandScript(`String` functionName, [`LuaValue` args ...])

Attempts to call the specified function name with the specified argument values in the context of an ActiveItem held in the opposing hand and synchronously returns the result if successful.

---

#### `void` activeItem.interact(`String` interactionType, `Json` config, [`EntityId` sourceEntityId])

Triggers an interact action on the owner as if they had initiated an interaction and the result had returned the specified interaction type and configuration. Can be used to e.g. open GUI windows normally triggered by player interaction with entities.

---

#### `void` activeItem.emote(`String` emote)

Triggers the owner to perform the specified emote.

---

#### `void` activeItem.setCameraFocusEntity([`EntityId` entity])

If the owner is a player, sets that player's camera to be centered on the position of the specified entity, or recenters the camera on the player's position if no entity id is specified.
