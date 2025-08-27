# Assets

Only accessible on onLoad scripts and postLoad scripts.

onLoad scripts and postLoad scripts are defined under the `scripts` key of a mod's metadata, like so:
```json
"scripts":{
  "onLoad": ["/scripts/assets/thisonloadscript.lua"],
  "postLoad": ["/scripts/assets/somepostloadscript.lua"]
}
```

onLoad and postLoad scripts also have access to `sb`. They do not have access to any other API tables, not even `root`.

onLoad and postLoad scripts work differently from other scripts. 
They do not use `init` or `update`, instead functioning like a traditional Lua script with full API access outside of functions.

---

#### `StringSet` assets.byExtension(`String` extension)

Returns a set of every asset path with this extension.

---

#### `StringList` assets.scan([`String` a], [`String` b])

If `b` is not specified, `a` is a suffix.
If both are specified, `a` is a prefix and `b` is a suffix.

Returns a list of all asset paths with with the prefix, if specified, and the suffix, if specified. This is case insensitive.
The prefix and suffix are checked against the entire path, including directories and extensions.
If neither are specified, returns ALL asset paths.

---

#### `Json` assets.json(`String` path)

Returns the JSON contents of a JSON asset file. Similar to `root.assetJson`.

---

#### Maybe<`String`> assets.origin(`String` path)

Returns the sourcePath for the specified asset or `nil` if the asset doesn't exist. Similar to `root.assetOrigin`.

---

#### `String` assets.bytes(`String` path)

Returns the raw contents of the specified asset file as a String. Similar to `root.assetData`.

---

#### `Image` assets.image(`String` path)

Returns the specified image asset as an `Image`. Similar to `root.assetImage`.

---

#### `Json` assets.frames(`String` path)

Returns the frame data of the specified image asset. Similar to `root.assetFrames`.

---

#### `bool` assets.exists(`String` path)

Returns true if the asset under the specified path exists.

---

#### `void` assets.add(`String` path, `LuaValue` data)

Adds a new asset at the specified path with the specified data. If an asset already exists here, it is overwritten.

---

#### `bool` assets.patch(`String` path, `String` patchPath)

Patches the specified asset with the specified patch file. Returns true if successful.

---

#### `bool` assets.erase(`String` path)

Removes the specified asset. Returns true if successful.

---

#### `LuaTable` assets.sourcePaths([`bool` withMetadata])

If withMetadata is false or unspecified, returns a list of the paths of all asset sources relative to the game executables.
If withMetadata is true, returns a map of all asset source paths to the contents of their metadatas or an empty table if the source has no metadata.

---

#### Maybe<`JsonObject`> assets.sourceMetadata(`String` sourcePath)

Returns the metadata of the specified sourcePath, an empty table if the sourcePath lacks a metadata, or `nil` if it doesn't exist.
