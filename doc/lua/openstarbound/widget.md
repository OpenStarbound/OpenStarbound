# Widget

New `widget` callbacks introduced in OpenStarbound.

---

## Widget specific callbacks

These callbacks only work for some widget types.

---

#### `String` widget.getHint(`String` widgetName)

Gets the hint text of a TextBoxWidget.

---

#### `void` widget.setHint(`String` widgetName, `String` hint)

Sets the hint text of a TextBoxWidget.

---

#### `String` widget.getCursorPosition(`String` widgetName)

Gets the cursor position of a TextBoxWidget.

---

#### `void` widget.setCursorPosition(`String` widgetName, `int` cursorPosition)

Sets the cursor position of a TextBoxWidget.

---

#### `void` widget.setImageStretchSet(`String` widgetName, `Json` imageStretchSet)

Sets the full image set of a ImageStretchWidget.

```lua
{
  ["begin"] = "image.png",
  ["inner"] = "image.png",
  ["end"] = "image.png"
}
```
