The activeItemAnimation table contains bindings available to client-side animation scripts for active items.

---

#### `Vec2F` activeItemAnimation.ownerPosition()

Returns the current entity position of the item's owner.

---

#### `Vec2F` activeItemAnimation.ownerAimPosition()

Returns the current world aim position of the item's owner.

---

#### `float` activeItemAnimation.ownerArmAngle()

Returns the current angle of the arm holding the item.

---

#### `int` activeItemAnimation.ownerFacingDirection()

Returns the current facing direction of the item's owner. Will return 1 for right or -1 for left.

---

#### `Vec2F` activeItemAnimation.handPosition([`Vec2F` offset])

Takes an input position (defaults to [0, 0]) relative to the item and returns a position relative to the owner entity.

---

#### `Vec2F` activeItemAnimation.partPoint(`String` partName, `String` propertyName)

Returns a transformation of the specified `Vec2F` parameter configured on the specified animation part, relative to the owner's position.

---

#### `PolyF` activeItemAnimation.partPoly(`String` partName, `String` propertyName)

Returns a transformation of the specified `PolyF` parameter configured on the specified animation part, relative to the owner's position.
