# tech

The `tech` table contains functions exclusively available in tech scripts.

---

#### `Vec2F` tech.aimPosition()

Returns the current cursor aim position.

---

#### `void` tech.setVisible(`bool` visible)

Sets whether the tech should be visible.

---

#### `void` tech.setParentState(`String` state)

Set the animation state of the player.

Valid states:
* "Stand"
* "Fly"
* "Fall"
* "Sit"
* "Lay"
* "Duck"
* "Walk"
* "Run"
* "Swim"

---

#### `void` tech.setParentDirectives(`String` directives)

Sets the image processing directives for the player.

---

#### `void` tech.setParentHidden(`bool` hidden)

Sets whether to make the player invisible. Will still show the tech.

---

#### `void` tech.setParentOffset(`Vec2F` offset)

Sets the position of the player relative to the tech.

---

#### `bool` tech.parentLounging()

Returns whether the player is lounging.

---

#### `void` tech.setToolUsageSuppressed(`bool` suppressed)

Sets whether to suppress tool usage on the player. When tool usage is suppressed no items can be used.

