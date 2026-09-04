
OpenStarbound includes some additional datatypes.

--- 
#### AudioInstance

An instance of currently playing audio.

Has the following methods:

##### `Maybe<Vec2F>` position()

Returns the position of the sound.

##### `void` setPosition(`Maybe<Vec2F>` position)

Sets the position. If no position is specified, sound will have no position (heard the same globally).

##### `void` translate(`Vec2F` distance)

Moves the physical position of the sound by the specified distance.

##### `float` rangeMultiplier()

Returns the sound's range multiplier.

##### `void` setRangeMultiplier(`float` rm)

Sets the sound's range multiplier.

##### `void` setVolume(`float` targetValue, `Maybe<float>` rampTime)

Sets the sound's volume, ramping over time if specified.

##### `void` setPitchMultiplier(`float` targetValue, `Maybe<float>` rampTime)

Sets the sound's pitch multiplier, ramping over time if specified.

##### `int` loops()

Returns the amount of times the sound will loop, or `-1` if it loops forever.

##### `void` setLoops(`int` loops)

Sets the amount of times the sound will loop. `-1` loops forever.

##### `double` currentTime()

Returns the sound's current time.

##### `double` totalTime()

Returns the sound's length.

##### `void` seekTime(`double` time)

Plays the sound from the given timeframe.

##### `String` mixerGroup()

Returns the sound's mixer group.

##### `void` setMixerGroup(`String` mixerGroup)

Sets the sound's mixer group.
The following mixer groups exist, defining which option volume sliders the sound is controlled by:
  `Effects`
  `Music`
  `Cinematic`
  `Instruments`

##### `void` setClockStart(`Maybe<int64_t>` time)

Delays the sound from playing until the given time.
These clock values are in milliseconds, see `sb.millisecondsSinceEpoch`.

##### `void` setClockStop(`Maybe<int64_t>` time, `Maybe<float>` rampTime)

Sets the time the sound will stop at, or resets it if no time is specified.
These clock values are in milliseconds, see `sb.millisecondsSinceEpoch`.

##### `void` stop(`Maybe<float>` rampTime)

Stops the sound.

##### `bool` finished()

Returns whether the sound has finished.

---
