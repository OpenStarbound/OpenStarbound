Most entity script contexts include the *script* table, which provides bindings for getting and setting the script's update rate. Update deltas are specified in numbers of frames, so a script with an update delta of 1 would run every frame, or a script with an update delta of 60 would run once per second. An update delta of 0 means that the script's periodic update will never be called, but it can still perform actions through script calls, messaging, or event hooks.

---

#### `void` script.setUpdateDelta(`unsigned` dt)

Sets the script's update delta.

---

#### `float` script.updateDt()

Returns the duration in seconds between periodic updates to the script.
