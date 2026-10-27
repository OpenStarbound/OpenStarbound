# Clipboard

The `clipboard` table is available to client-side scripts (such as script panes and interface scripts) and exposes access to the
system clipboard while the game window is focused.

---

#### `bool` clipboard.available()

Returns `true` if clipboard access is currently allowed. Clipboard operations require the game window to be focused and, unless
overridden, for the player to have enabled clipboard integration.

---

#### `bool` clipboard.hasText()

Returns `true` if textual data is available on the clipboard and the script may read it.

---

#### `Maybe<String>` clipboard.getText()

Returns the clipboard text when access is available, or `nil` otherwise.

---

#### `bool` clipboard.setText(`String` text)

Replaces the clipboard contents with the supplied UTF-8 text. Returns `false` if the game window is unfocused or the operation is
blocked.

---

#### `bool` clipboard.setData(`Table<String, String>` data)

Replaces the clipboard contents with arbitrary data payloads. Keys are MIME type strings (for example `"text/plain"` or
`"image/png"`) and values are raw byte strings containing the payload for that type. Returns `false` if the game window is
unfocused or the operation is blocked.

---

#### `bool` clipboard.setImage(`Variant<Image, String>` imageOrPath)

Copies an image to the clipboard. When an `Image` userdata is provided it is exported as-is; when a string path is supplied the
image asset is loaded before exporting. The image is written as PNG data and `false` is returned if the clipboard cannot be
modified.