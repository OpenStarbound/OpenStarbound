# Songbook

The new `songbook` table is accessible from players and NPC's.

---

#### `String` songbook.band()

Returns a string containing the instrumentalist's current band. Defaults to `""`.

---

#### `song` songbook.song()

Returns the notes of the song currently being played.

---

#### `void` songbook.keepAlive(`String` instrument, `Vec2F` position)

Must be called repeatedly while the instrument is equipped to signal that it is still in use. Sets the instrument type and the world position the sound plays from. If a song is active, this also resets the idle cooldown that would otherwise stop playback.

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

#### `void` songbook.play(`Json` song, `String` timeSource)

Starts playing `song`, stopping any already playing. `song` is a JSON object whose `abc` field holds the song in ABC notation. `timeSource` is an identifier for synchronising playback, so instrumentalists that use the same `timeSource` share a clock to play in sync. If it is empty, a random one is generated.

---

#### `String` songbook.instrument()

Returns the type of instrument last played, as determined by the instrument's `kind` value. Defaults to `""`.
