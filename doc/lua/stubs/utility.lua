---@meta

--- sb API
---@class sb
sb = {}

--- Returns a randomized value with a normal distribution using the specified standard deviation (default is 1.0) and mean (default is 0). ---
---@param standardDeviation number
---@param mean number
---@return number
function sb.nrand(standardDeviation, mean) end

--- Returns a `String` representation of a new, randomly-created `Uuid`. ---
---@return string
function sb.makeUuid() end

--- Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Info log level. ---
---@param formatString string
---@param formatValues LuaValue
---@return void
function sb.logInfo(formatString, formatValues) end

--- Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Warn log level. ---
---@param formatString string
---@param formatValues LuaValue
---@return void
function sb.logWarn(formatString, formatValues) end

--- Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Error log level. ---
---@param formatString string
---@param formatValues LuaValue
---@return void
function sb.logError(formatString, formatValues) end

--- Sets an entry in the debug log map (visible while in debug mode) using the specified format string and optional formatted replacement values. ---
---@param key string
---@param formatString string
---@param formatValues LuaValue
---@return void
function sb.setLogMap(key, formatString, formatValues) end

--- Returns a human-readable string representation of the specified JSON value. If pretty is `true`, objects and arrays will have whitespace added for readability. ---
---@param value Json
---@param pretty boolean
---@return string
function sb.printJson(value, pretty) end

--- Returns a human-readable string representation of the specified `LuaValue`. ---
---@param value LuaValue
---@return string
function sb.print(value) end

--- Returns an interpolated `Vec2F` or `double` between the two specified values using a sin ease function. ---
---@param offset number
---@return Variant<Vec2F, double>
function sb.interpolateSinEase(offset) end

--- Replaces all tags in the specified string with the specified tag replacement values. ---
---@param string string
---@return string
function sb.replaceTags(string) end

--- Returns the result of merging the contents of b on top of a. ---
---@param a Json
---@param b Json
---@return Json
function sb.jsonMerge(a, b) end

--- Attempts to extract the value in the specified content at the specified path, and returns the found value or the specified default if no such value exists. ---
---@param content Json
---@param path string
---@param default Json
---@return Json
function sb.jsonQuery(content, path, default) end

--- Returns a statically randomized 32-bit signed integer based on the given list of seed values. ---
---@param hashValues LuaValue
---@return number
function sb.staticRandomI32(hashValues) end

--- Returns a statically randomized 32-bit signed integer within the specified range based on the given list of seed values. ---
---@param min number
---@param max number
---@param hashValues LuaValue
---@return number
function sb.staticRandomI32Range(min, max, hashValues) end

--- Returns a statically randomized `double` based on the given list of seed values. ---
---@param hashValues LuaValue
---@return number
function sb.staticRandomDouble(hashValues) end

--- Returns a statically randomized `double` within the specified range based on the given list of seed values. ---
---@param min number
---@param max number
---@param hashValues LuaValue
---@return number
function sb.staticRandomDoubleRange(min, max, hashValues) end

--- Creates and returns a Lua UserData value which can be used as a random source, initialized with the specified seed. The `RandomSource` has the following methods:
---@param seed unsigned
---@return RandomSource
function sb.makeRandomSource(seed) end

--- Creates and returns a Lua UserData value which can be used as a Perlin noise source. The configuration for the `PerlinSource` should be a JSON object and can include the following keys: * `unsigned` __seed__ - Seed value used to initialize the source. * `String` __type__ - Type of noise to use. Valid types are "perlin", "billow" or "ridgedMulti". * `int` __octaves__ - Number of octaves of noise to use. Defaults to 1. * `double` __frequency__ - Defaults to 1.0. * `double` __amplitude__ - Defaults to 1.0. * `double` __bias__ - Defaults to 0.0. * `double` __alpha__ - Defaults to 2.0. * `double` __beta__ - Defaults to 2.0. * `double` __offset__ - Defaults to 1.0. * `double` __gain__ - Defaults to 2.0. The `PerlinSource` has only one method:
---@param config Json
---@return PerlinSource
function sb.makePerlinSource(config) end

--- Global functions

--- Reinitializes the random source, optionally using the specified seed.
---@param seed unsigned
---@return void
function init(seed) end

--- Adds entropy to the random source, optionally using the specified seed.
---@param seed unsigned
---@return void
function addEntropy(seed) end

--- Returns a random 32-bit unsigned integer value.
---@return unsigned
function randu32() end

--- Returns a random 64-bit unsigned integer value.
---@return unsigned
function randu64() end

--- Returns a random 32-bit signed integer value.
---@return number
function randi32() end

--- Returns a random 64-bit signed integer value.
---@return number
function randi64() end

--- Returns a random `float` value within the specified range, or between 0 and 1 if no range is specified.
---@param min number
---@param max number
---@return number
function randf(min, max) end

--- Returns a random `double` value within the specified range, or between 0 and 1 if no range is specified.
---@param min number
---@param max number
---@return number
function randf(min, max) end

--- Returns a random unsigned integer value between minOrMax and max, or between 0 and minOrMax if no max is specified.
---@param minOrMax unsigned
---@param max unsigned
---@return unsigned
function randf(minOrMax, max) end

--- Returns a random signed integer value between minOrMax and max, or between 0 and minOrMax if no max is specified.
---@param min number
---@param max number
---@return number
function randf(min, max) end

--- Returns a random `bool` value. ---
---@return boolean
function randb() end

--- Returns a `float` value from the Perlin source using 1, 2, or 3 dimensions of input.
---@param x number
---@param y number
---@param z number
---@return number
function get(x, y, z) end
