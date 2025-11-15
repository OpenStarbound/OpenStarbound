# Camera

The `camera` table provides several camera-related functions.

---

#### `Vec2F` camera.position()

Returns the current camera position in world coordinates.

---

#### `float` camera.pixelRatio()

Returns the camera's current pixel ratio.

---

#### `void` camera.setPixelRatio(`float` pixelRatio, [`bool` smooth])

Sets the camera's pixel ratio. If `smooth` is true, the change will be interpolated smoothly using linear interpolation (lerp).

---

#### `Vec2U` camera.screenSize()

Returns the screen size in pixels.

---

#### `RectF` camera.worldScreenRect()

Returns the screen rectangle in world coordinates.

---

#### `RectI` camera.worldTileRect()

Returns the tile rectangle covering the tiles that overlap the screen.

---

#### `Vec2F` camera.tileMinScreen()

Returns the screen-space position of the lower-left corner of the tile at the minimum (lower-left) corner of `camera.worldTileRect()`.

---

#### `Vec2F` camera.screenToWorld(`Vec2F` screenCoordinates)

Converts screen coordinates to world coordinates. Assumes the top-left corner of the screen is `(0, 0)` in screen coordinates.

---

#### `Vec2F` camera.worldToScreen(`Vec2F` worldCoordinates)

Converts world coordinates to screen coordinates. Because the world is non-Euclidean, a single world coordinate may map to multiple screen coordinates; this function returns the screen coordinate closest to the center of the screen.
