The sb table contains miscellaneous utility functions that don't directly relate to any assets or content of the game.

---

#### `double` sb.nrand([`double` standardDeviation], [`double` mean])

Returns a randomized value with a normal distribution using the specified standard deviation (default is 1.0) and mean (default is 0).

---

#### `String` sb.makeUuid()

Returns a `String` representation of a new, randomly-created `Uuid`.

---

#### `void` sb.logInfo(`String` formatString, [`LuaValue` formatValues ...])

Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Info log level.

---

#### `void` sb.logWarn(`String` formatString, [`LuaValue` formatValues ...])

Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Warn log level.

---

#### `void` sb.logError(`String` formatString, [`LuaValue` formatValues ...])

Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Error log level.

---

#### `void` sb.setLogMap(`String` key, `String` formatString, [`LuaValue` formatValues ...])

Sets an entry in the debug log map (visible while in debug mode) using the specified format string and optional formatted replacement values.

---

#### `String` sb.printJson(`Json` value, [`bool` pretty])

Returns a human-readable string representation of the specified JSON value. If pretty is `true`, objects and arrays will have whitespace added for readability.

---

#### `String` sb.print(`LuaValue` value)

Returns a human-readable string representation of the specified `LuaValue`.

---

#### `Variant<Vec2F, double>` sb.interpolateSinEase(`double` offset, `Variant<Vec2F, double>` value1, `Variant<Vec2F, double>` value2)

Returns an interpolated `Vec2F` or `double` between the two specified values using a sin ease function.

---

#### `String` sb.replaceTags(`String` string, `Map<String, String>` tags)

Replaces all tags in the specified string with the specified tag replacement values.

---

#### `Json` sb.jsonMerge(`Json` a, `Json` b)

Returns the result of merging the contents of b on top of a.

---

#### `Json` sb.jsonQuery(`Json` content, `String` path, `Json` default)

Attempts to extract the value in the specified content at the specified path, and returns the found value or the specified default if no such value exists.

---

#### `int` sb.staticRandomI32([`LuaValue` hashValues ...])

Returns a statically randomized 32-bit signed integer based on the given list of seed values.

---

#### `int` sb.staticRandomI32Range(`int` min, `int` max, [`LuaValue` hashValues ...])

Returns a statically randomized 32-bit signed integer within the specified range based on the given list of seed values.

---

#### `double` sb.staticRandomDouble([`LuaValue` hashValues ...])

Returns a statically randomized `double` based on the given list of seed values.

---

#### `double` sb.staticRandomDoubleRange(`double` min, `double` max, [`LuaValue` hashValues ...])

Returns a statically randomized `double` within the specified range based on the given list of seed values.

---

#### `RandomSource` sb.makeRandomSource([`unsigned` seed])

Creates and returns a Lua UserData value which can be used as a random source, initialized with the specified seed. The `RandomSource` has the following methods:

##### `void` init([`unsigned` seed])

Reinitializes the random source, optionally using the specified seed.

##### `void` addEntropy([`unsigned` seed])

Adds entropy to the random source, optionally using the specified seed.

##### `unsigned` randu32()

Returns a random 32-bit unsigned integer value.

##### `unsigned` randu64()

Returns a random 64-bit unsigned integer value.

##### `int` randi32()

Returns a random 32-bit signed integer value.

##### `int` randi64()

Returns a random 64-bit signed integer value.

##### `float` randf([`float` min], [`float` max])

Returns a random `float` value within the specified range, or between 0 and 1 if no range is specified.

##### `double` randf([`double` min], [`double` max])

Returns a random `double` value within the specified range, or between 0 and 1 if no range is specified.

##### `unsigned` randf(`unsigned` minOrMax, [`unsigned` max])

Returns a random unsigned integer value between minOrMax and max, or between 0 and minOrMax if no max is specified.

##### `int` randf([`int` min], [`int` max])

Returns a random signed integer value between minOrMax and max, or between 0 and minOrMax if no max is specified.

##### `bool` randb()

Returns a random `bool` value.

---

#### `PerlinSource` sb.makePerlinSource(`Json` config)

Creates and returns a Lua UserData value which can be used as a Perlin noise source. The configuration for the `PerlinSource` should be a JSON object and can include the following keys:

* `unsigned` __seed__ - Seed value used to initialize the source.
* `String` __type__ - Type of noise to use. Valid types are "perlin", "billow" or "ridgedMulti".
* `int` __octaves__ - Number of octaves of noise to use. Defaults to 1.
* `double` __frequency__ - Defaults to 1.0.
* `double` __amplitude__ - Defaults to 1.0.
* `double` __bias__ - Defaults to 0.0.
* `double` __alpha__ - Defaults to 2.0.
* `double` __beta__ - Defaults to 2.0.
* `double` __offset__ - Defaults to 1.0.
* `double` __gain__ - Defaults to 2.0.

The `PerlinSource` has only one method:

##### `float` get(`float` x, [`float` y], [`float` z])

Returns a `float` value from the Perlin source using 1, 2, or 3 dimensions of input.
