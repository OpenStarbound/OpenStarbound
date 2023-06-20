# animationConfig

The `animationConfig` table contains functions for getting configuration options from the base entity and its networked animator.

It is available only in client side rendering scripts.

---

#### `Json` animationConfig.animationParameter(`String` key)

Returns a networked value set by the parent entity's master script.

---

#### `Vec2F` animationConfig.partPoint(`String` partName, `String` propertyName)

Returns a `Vec2F` configured in a part's properties with all of the part's transformations applied to it.

---

#### `PolyF` animationConfig.partPoly(`String` partName, `String` propertyName)

Returns a `PolyF` configured in a part's properties with all the part's transformations applied to it.
