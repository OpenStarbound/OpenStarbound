# item

The `item` table is available in all scripted items and contains functions relating to the item itself.

---

#### `String` item.name()

Returns the name of the item.

---

#### `size_t` item.count()

Returns the stack count of the item.

---

#### `size_t` item.setCount(`size_t` count)

Sets the item count. Returns any overflow.

---

#### `size_t` item.maxStack()

Returns the max number of this item that will fit in a stack.

---

#### `bool` item.matches(`ItemDescriptor` desc, [`bool` exactMatch])

Returns whether the item matches the specified item. If exactMatch is `true` then both the items' names and parameters are compared, otherwise only the items' names.

---

#### `bool` item.consume(`size_t` count)

Consumes items from the stack. Returns whether the full count was successfuly consumed.

---

#### `bool` item.empty()

Returns whether the item stack is empty.

---

#### `ItemDescriptor` item.descriptor()

Returns an item descriptor for the item.

---

#### `String` item.description()

Returns the description for the item.

---

#### `String` item.friendlyName()

Returns the short description for the item.

---

#### `int` item.rarity()

Returns the rarity for the item.

* 0 = common
* 1 = uncommon
* 2 = rare
* 3 = legendary
* 4 = essential

---

#### `String` item.rarityString()

Returns the rarity as a string.

---

#### `size_t` item.price()

Returns the item price.

---

#### `unsigned` item.fuelAmount()

Returns the item fuel amount.

---

#### `Json` item.iconDrawables()

Returns a list of the item's icon drawables.

---

#### `Json` item.dropDrawables()

Returns a list of the item's itemdrop drawables.

---

#### `String` item.largeImage()

Returns the item's configured large image, if any.

---

#### `String` item.tooltipKind()

Returns the item's tooltip kind.

---

#### `String` item.category()

Returns the item's category

---

#### `String` item.pickupSound()

Returns the item's pickup sound.

---

#### `bool` item.twoHanded()

Returns whether the item is two handed.

---

#### `float` item.timeToLive()

Returns the items's time to live.

---

#### `Json` item.learnBlueprintsOnPickup()

Returns a list of the blueprints learned on picking up this item.

---

#### `bool` item.hasItemTag(`String` itemTag)

Returns whether the set of item tags for this item contains the specified tag.

---

#### `Json` item.pickupQuestTemplates()

Returns a list of quests acquired on picking up this item.
