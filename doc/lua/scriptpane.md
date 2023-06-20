These pane bindings are available to scripted interface panes and include functions not specifically related to widgets within the pane.

---

#### `EntityId` pane.sourceEntity()

Returns the entity id of the pane's source entity.

---

#### `void` pane.dismiss()

Closes the pane.

---

#### `void` pane.playSound(`String` sound, [`int` loops], [`float` volume])

Plays the specified sound asset, optionally looping the specified number of times or at the specified volume.

---

#### `bool` pane.stopAllSounds(`String` sound)

Stops all instances of the given sound asset, and returns `true` if any sounds were stopped and `false` otherwise.

---

#### `void` pane.setTitle(`String` title, `String` subtitle)

Sets the window title and subtitle.

---

#### `void` pane.setTitleIcon(`String` image)

Sets the window icon.

---

#### `void` pane.addWidget(`Json` widgetConfig, [`String` widgetName])

Creates a new widget with the specified config and adds it to the pane, optionally with the specified name.

---

#### `void` pane.removeWidget(`String` widgetName)

Removes the specified widget from the pane.
