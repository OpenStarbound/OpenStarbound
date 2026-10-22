---@meta

--- songbook API
---@class songbook
songbook = {}

--- Returns a string containing the instrumentalist's current band. Defaults to `""`. ---
---@return string
function songbook.band() end

--- Returns the notes of the song currently being played. ---
---@return song
function songbook.song() end

--- Must be called repeatedly while the instrument is equipped to signal that it is still in use. Sets the instrument type and the world position the sound plays from. If a song is active, this also resets the idle cooldown that would otherwise stop playback. ---
---@param instrument string
---@param position Vec2F
---@return void
function songbook.keepAlive(instrument, position) end

--- Causes the instrumentalist to stop playing. ---
---@return void
function songbook.stop() end

--- Returns whether the instrumentalist is playing a song. ---
---@return boolean
function songbook.active() end

--- Returns whether the instrumentalist is currently performing part of a song. ---
---@return boolean
function songbook.instrumentPlaying() end

--- Starts playing `song`, stopping any already playing. `song` is a JSON object whose `abc` field holds the song in ABC notation. `timeSource` is an identifier for synchronising playback, so instrumentalists that use the same `timeSource` share a clock to play in sync. If it is empty, a random one is generated. ---
---@param song Json
---@param timeSource string
---@return void
function songbook.play(song, timeSource) end

--- Returns the type of instrument last played, as determined by the instrument's `kind` value. Defaults to `""`.
---@return string
function songbook.instrument() end
