# activeitem

The activeItem table contains bindings which provide functionality for the ActiveItem and for the item's 'owner' (a ToolUser entity currently holding the item).

---

#### `void` activeItem.setInventoryIcon(`Variant<String, List<Drawable>>` icon)

If **icon** is a `String` it will set the item's icon to the image at it's path, this path can be local to the item's directory, or a global path.

If **icon** is a `List<Drawable>` it will use those drawbles as the item's icon. Any image drawables can have their path be local to the item's directory or global as well.

This behavior now matches that of an item's build parameters.

#### `void` activeItem.setSecondaryIcon(`Variant<String, List<Drawable>>` icon)

If **icon** is a `String` it will set the item's secondary icon to the image at it's path, this path can be local to the item's directory, or a global path.

If **icon** is a `List<Drawable>` it will use those drawbles as the item's secondary icon. Any image drawables can have their path be local to the item's directory or global as well.

Secondary icons are shown in the right hand slot of two handed items.

---

#### `void` activeItem.setDescription(`String` description)

Sets the item's description

#### `void` activeItem.setShortDescription(`String` shortDescription)

Sets the item's short description

---
