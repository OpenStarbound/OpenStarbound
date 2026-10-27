# activeitem

The `activeItem` table contains bindings which provide functionality for the ActiveItem and for the item's 'owner' (a ToolUser entity currently holding the item).

---

#### `void` activeItem.setInventoryIcon(`Variant<String, List<Drawable>>` icon)

If **icon** is a `String` it will set the item's icon to the image at it's path, this path can be local to the item's directory, or a global path.

If **icon** is a `List<Drawable>` it will use those drawbles as the item's icon. Any image drawables can have their path be local to the item's directory or global as well.

This behavior now matches that of an item's build parameters.

---

#### `void` activeItem.setSecondaryIcon(`Variant<String, List<Drawable>>` icon)

If **icon** is a `String` it will set the item's secondary icon to the image at it's path, this path can be local to the item's directory, or a global path.

If **icon** is a `List<Drawable>` it will use those drawbles as the item's secondary icon. Any image drawables can have their path be local to the item's directory or global as well.

Secondary icons are shown in the right hand slot of two handed items.

---

#### `void` activeItem.setDescription(`String` description)

Sets the item's description

---

#### `void` activeItem.setShortDescription(`String` shortDescription)

Sets the item's short description

---


#### `void` activeItem.fire([`String` fireMode])

Triggers the item to fire if it is currently ready. If **fireMode** is specified it must be either `"Primary"` or `"Alt"`,
otherwise the primary fire mode is used.

---

#### `void` activeItem.triggerCooldown()

Immediately starts the item's cooldown using its current fire time.

---

#### `void` activeItem.setCooldown(`float` cooldownTime)

Sets the current cooldown timer to **cooldownTime** seconds without modifying the item's default cooldown length.

---

#### `void` activeItem.endCooldown()

Clears the current cooldown timer, making the item ready to fire again.

---

#### `float` activeItem.cooldownTime()

Returns the item's configured default cooldown duration in seconds.

---

#### `Json` activeItem.fireableParam(`String` name, `Json` default)

Returns the value of the fireable parameter **name** from the item's configuration, or **default** when the parameter is not
defined.

---

#### `bool` activeItem.ready()

Returns whether the item can currently begin firing.

---

#### `bool` activeItem.firing()

Returns whether the item is currently firing.

---

#### `bool` activeItem.windingUp()

Returns whether the item is winding up in preparation to fire.

---

#### `bool` activeItem.coolingDown()

Returns whether the item is cooling down after firing.

---

#### `bool` activeItem.ownerFullEnergy()

Returns whether the item's owner currently has a full energy bar.

---

#### `bool` activeItem.ownerEnergy()

Returns whether the item's owner has any energy available.

---

#### `bool` activeItem.ownerEnergyLocked()

Returns whether the item's owner currently has their energy locked.

---

#### `bool` activeItem.ownerConsumeEnergy(`float` amount)

Attempts to consume **amount** energy from the item's owner and returns whether the energy was successfully consumed.

---