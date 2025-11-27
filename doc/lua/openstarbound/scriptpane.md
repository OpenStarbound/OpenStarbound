# Script Pane

OpenStarbound extends the vanilla `pane` bindings described in `doc/lua/scriptpane.md` with additional helpers for interacting
with scripted panes at runtime.

---

#### `LuaCallbacks` pane.toWidget()

Returns the root widget of the pane as a table of widget callbacks, equivalent to calling `widget` helpers on the pane itself.
This is especially useful when a script pane needs to call functions such as `widget.getChildAt` on its root.

---

#### `Vec2I` pane.getPosition()

Returns the pane's current on-screen position.

---

#### `void` pane.setPosition(`Vec2I` position)

Moves the pane to the specified screen position.

---

#### `Vec2I` pane.getSize()

Returns the pane's current size in pixels.

---

#### `void` pane.setSize(`Vec2I` size)

Resizes the pane to the supplied dimensions.

---

#### `float` pane.scale()

Returns the interface scale currently applied to the pane. This mirrors `interface.scale()` but is available directly on the pane
object.

---

#### `bool` pane.isDisplayed()

Returns `true` if the pane is currently being displayed.

---

#### `bool` pane.hasFocus()

Returns `true` if the pane currently has keyboard focus.

---

#### `void` pane.show()

Shows the pane if it is hidden.

---

#### `void` pane.hide()

Hides the pane if it is currently shown.

---

#### `LuaCallbacks` pane.addWidget(`Json` config, [`String` widgetName])

In vanilla Starbound this function returns nothing. OpenStarbound now returns widget callbacks for the newly created widget,
allowing scripts to immediately operate on it without a separate `widget` lookup.

---

#### `bool` pane.removeWidget(`String` widgetName)

`pane.removeWidget` now returns whether the named widget existed and was removed.