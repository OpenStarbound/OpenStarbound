# Utility

A few additional utiliy callbacks are added by OpenStarbound.

---

#### `Json` sb.parseJson(`String` json)

Parses a JSON string.
Equivalent to StarExtensions' `sb.jsonFromString`, which is also present for backwards compatibility.

---

#### `Json` sb.parseJsonSequence(`String` json)

Parses a space-separated sequence of JSON values in a string into an array of values. Useful for parsing command arguments.
Similar to `chat.parseArguments`.

---

#### `String` sb.stripEscapeCodes(`String` text)

Removes escape codes from the string.
If given `^blue;fish^reset;`, outputs `fish`.

---

#### `bool` sb.jsonEqual(`Json` a, `Json` b)

Checks if the two json values match.

---

#### `int` sb.millisecondsSinceEpoch()

Returns how many milliseconds have passed since the Unix epoch. (1970-01-01)
This can be used like other time values, but note that it is in milliseconds.
It is also useful for working with sound setClockStart and setClockStop, which have time values in milliseconds.

---

#### `pair<RpcPromise<Json>,RpcPromiseKeeper<Json>>` sb.makePromise()

Returns a promise and its keeper.
RpcPromises can be returned in message handlers to delay the fulfilling of the message promise until that promise is fulfilled.

The `RpcPromiseKeeper` has the following methods:

##### `void` fulfill(`Json` result)

Fulfills the promise as succeeded with the provided result.

##### `void` fail(`String` error)

Fails the promise with the provided error.

##### `void` chain(`RpcPromise<Json>` promise)

Fulfills or fails the promise depending on the provided promise.

---

#### `String` sb.embedData(`String` data, `String` compression)

Encodes the provided data, whether actual plaintext or raw such as from `root.assetData` or `assets.bytes`, into an embedded asset path.
It is optionally compressed using the given mode, which is either `none`, `zlib`, or `zstd`.

---

#### `String` sb.embedJson(`Json` data, `String` compression)

Encodes the provided Json data into an embedded asset path using Starbound's internal binary format for Json data.
It is optionally compressed using the given mode, which is either `none`, `zlib`, or `zstd`.

---

#### `String` sb.embedText(`String` data)

Creates an embedded asset path with the provided plaintext data, adding escape codes as necessary.

