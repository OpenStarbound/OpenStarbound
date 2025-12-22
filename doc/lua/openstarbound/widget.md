# Widget

The `widget` table now contains extra bindings.

---

## Widget specific callbacks

These callbacks only work for some widget types.

---

### TextBoxWidget helpers

#### `String` widget.getHint(`String` widgetName)

Gets the hint text of a TextBoxWidget.

---

#### `void` widget.setHint(`String` widgetName, `String` hint)

Sets the hint text of a TextBoxWidget.

---

#### `Int` widget.getCursorPosition(`String` widgetName)

Gets the cursor position of a TextBoxWidget.

---

#### `void` widget.setCursorPosition(`String` widgetName, `int` cursorPosition)

Sets the cursor position of a TextBoxWidget.

---

### SliderBarWidget helpers

#### `void` widget.setSliderRange(`String` widgetName, `int` minimum, `int` maximum, [`int` step])

Sets the allowed range for a slider, optionally overriding the step used when the slider value is changed. The step defaults to
`1` when omitted.

---

### ImageStretchWidget helpers

#### `void` widget.setImageStretchSet(`String` widgetName, `Json` imageStretchSet)

Sets the full image set of a ImageStretchWidget.
```lua
{
  ["begin"] = "image.png",
  ["inner"] = "image.png",
  ["end"] = "image.png"
}
```

---

### FlowLayout helpers

#### `void` widget.addFlowImage(`String` widgetName, `String` childName, `String` imagePath)

Appends a new ImageWidget to the specified FlowLayout widget, using `childName` as the new widget's identifier within the
layout.

---

### ScrollArea helpers

#### `Maybe<Vec2I>` widget.getScrollOffset(`String` widgetName)

Gets the current scroll offset of a ScrollArea widget. Returns `nil` if the widget is not a ScrollArea.

---

#### `void` widget.setScrollOffset(`String` widgetName, `Vec2I` offset)

Sets the current scroll offset of a ScrollArea widget.

---

#### `Vec2I` widget.getMaxScrollPosition(`String` widgetName)

Gets the maximum scroll position of a ScrollArea widget. This is the maximum offset that can be scrolled to.