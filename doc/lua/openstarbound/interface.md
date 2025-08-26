# Interface

The `interface` table contains bindings which allow scripts to display a message at the bottom of the screen, among other things.

---

#### `void` interface.queueMessage(`String` message, [`float` cooldown, [`float` springState]])

Queues a message popup at the bottom of the screen with an optional **cooldown** and **springState**.

---

#### `void` interface.setHudVisible(`bool` visible)

Sets the HUD's visibility.

---

#### `bool` interface.hudVisible()

Returns the HUD's visibility.

---

#### `PaneId` interface.bindRegisteredPane(`String` paneName)
Binds a registered pane (defined in `/source/frontend/StarMainInterfaceTypes`) to a Lua value, which can then call widget functions on that pane.
<details><summary><b>Panes</b></summary>
EscapeDialog<br>
Inventory<br>
Codex<br>
Cockpit<br>
Tech<br>
Songbook<br>
Ai<br>
Popup<br>
Confirmation<br>
JoinRequest<br>
Options<br>
QuestLog<br>
ActionBar<br>
TeamBar<br>
StatusPane<br>
Chat<br>
WireInterface<br>
PlanetText<br>
RadioMessagePopup<br>
CraftingPlain<br>
QuestTracker<br>
MmUpgrade<br>
Collections<br>
</details>

---

#### `void` interface.displayRegisteredPane(`String` paneName)

Displays a registered pane.

---

#### `CanvasWidget` interface.bindCanvas(`String` name, [`bool` ignoreInterfaceScale = false])

Binds the canvas widget on the main interface with the specified name as userdata for easy access. The `CanvasWidget` has the same methods as described in `widget.md`.

- **ignoreInterfaceScale** is used to ignore the current interface scaling and bind the canvas with the screen size.

---

#### `int` interface.scale()

Returns the scale used for interfaces.
