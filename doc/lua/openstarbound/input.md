# Input

Allows access to custom keybinds from most clientside Lua scripts.

---

#### `Maybe<unsigned>` input.bindDown(`String` categoryId, `String` bindId)

TODO

---

#### `bool` input.bindHeld(`String` categoryId, `String` bindId)

Returns `true` if this bind is currently held.

---

#### `Maybe<unsigned>` input.bindDown(`String` categoryId, `String` bindId)

TODO

---

#### `Maybe<unsigned>` input.keyDown(`String` keyName, [`StringList` modNames])

TODO

---

#### `bool` input.keyHeld(`String` keyName,  [`StringList` modNames])

Returns true if the specified key and all specified modifier keys (like Ctrl and Shift) are held.

---

#### `bool` input.key(`String` keyName,  [`StringList` modNames])

Same as `input.keyHeld`.

---

#### `Maybe<unsigned>` input.keyUp(`String` keyName, [`StringList` modNames])

TODO

---

#### `Maybe<unsigned>` input.mouseDown(`String` buttonName)

TODO

---

#### `bool` input.mouseHeld(`String` buttonName)

Returns true if the specified mouse button is held.

---

#### `bool` input.mouse(`String` buttonName)

Same as `input.mouseHeld`.

---

#### `Maybe<unsigned>` input.mouseUp(`String` buttonName)

TODO

---

#### `void` input.resetBinds(`String` categoryId, `String` bindId)

TODO

---

#### `void` input.setBinds(`String` categoryId, `String` bindId)

TODO

---

#### `Json` input.getDefaultBinds(`String` categoryId, `String` bindId)

TODO

---

#### `Json` input.getBinds(`String` categoryId, `String` bindId)

TODO

---

#### `Json` input.events()

TODO

---

#### `Vec2F` input.mousePosition()

Returns the mouse position in pixels relative to the bottom left of the screen.

---

#### `unsigned` input.getTag(`String` tagName)

TODO









