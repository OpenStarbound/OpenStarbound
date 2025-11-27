# Songbook

The new `songbook` table is accessible from players and NPC's.

---

#### `String` songbook.band()

Returns a string containing the instrumentalist's current band. Defaults to `""`.

---

#### `song` songbook.song()

Returns the notes of the song currently being played.

---

#### `void` songbook.keepAlive(`Float` duration)

TODO: Add description for songbook.keepAlive

---

#### `void` songbook.stop()

Causes the instrumentalist to stop playing.

---

#### `bool` songbook.active()

Returns whether the instrumentalist is playing a song.

---

#### `bool` songbook.instrumentPlaying()

Returns whether the instrumentalist is currently performing part of a song.

---

#### `void` songbook.play(`Json` song)

TODO: Add description for songbook.play

---

#### `String` songbook.instrument()

Returns the type of instrument last played, as determined by the instrument's `kind` value. Defaults to `""`.
