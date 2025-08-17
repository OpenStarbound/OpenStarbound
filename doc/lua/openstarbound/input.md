# Input

Allows access to custom keybinds from most clientside Lua scripts.

---

#### `Maybe<unsigned>` input.bindDown(`String` categoryId, `String` bindId)

If this bind was pressed this frame, returns how many times it has been pressed. Returns `nil` otherwise.

---

#### `bool` input.bindHeld(`String` categoryId, `String` bindId)

Returns `true` if this bind is currently held.

---

#### `Maybe<unsigned>` input.bindDown(`String` categoryId, `String` bindId)

If this bind was released this frame, returns how many times it has been released. Returns `nil` otherwise.

---

#### `Maybe<unsigned>` input.keyDown(`String` keyName, [`StringList` modNames])

If this key was pressed this frame, returns how many times it has been pressed. Returns `nil` otherwise or if the specified modifier keys are not held.

---

#### `bool` input.keyHeld(`String` keyName,  [`StringList` modNames])

Returns true if the specified key and all specified modifier keys (like Ctrl and Shift) are held.

---

#### `bool` input.key(`String` keyName,  [`StringList` modNames])

Same as `input.keyHeld`.

---

#### `Maybe<unsigned>` input.keyUp(`String` keyName, [`StringList` modNames])

If this key was released this frame, returns how many times it has been released. Returns `nil` otherwise or if the specified modifier keys are not held.

---

#### `Maybe<unsigned>` input.mouseDown(`String` buttonName)

If this mouse button was pressed this frame, returns how many times it has been pressed. Returns `nil` otherwise.

---

#### `bool` input.mouseHeld(`String` buttonName)

Returns true if the specified mouse button is held.

---

#### `bool` input.mouse(`String` buttonName)

Same as `input.mouseHeld`.

---

#### `Maybe<unsigned>` input.mouseUp(`String` buttonName)

If this mouse button was released this frame, returns how many times it has been released. Returns `nil` otherwise.

---

#### `void` input.resetBinds(`String` categoryId, `String` bindId)

Resets this bind to its default keys.

---

#### `void` input.setBinds(`String` categoryId, `String` bindId, `Json` binds)

Sets this bind to the specified keys.

---

#### `Json` input.getDefaultBinds(`String` categoryId, `String` bindId)

Returns the default keys for this bind.

---

#### `Json` input.getBinds(`String` categoryId, `String` bindId)

Returns the keys for this bind.

---

#### `Json` input.events()

Returns all input events for this frame.

---

#### `Vec2F` input.mousePosition()

Returns the mouse position in pixels relative to the bottom left of the screen.

---

#### `unsigned` input.getTag(`String` tagName)

Returns the amount of binds currently held with the given tag.

