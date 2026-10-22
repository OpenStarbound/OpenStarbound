# Boot configuration (sbinit.config)

`sbinit.config` controls which asset sources are loaded and how they are
ordered. It is read once at startup, before the asset database is built.

`ignoreAssetSources` and `assetSourcePriorities` accept asset source selectors:

- the source name from its metadata (`"FrackinUniverse"`),
- the containing folder name, e.g. a Steam Workshop item id (`"729480149"`),
- the full path of the source or pak file.

Matching is case-insensitive. Empty selectors are ignored.

## ignoreAssetSources

A list of selectors. Matching asset sources are not loaded at all.

```
"ignoreAssetSources": ["2865755682", "Some Broken Mod"]
```

The base game (`base`) and OpenStarbound's own assets (`opensb_base`) cannot
be ignored. If another mod requires an ignored source, the requirement is
treated as satisfied and a warning is logged instead of aborting startup.

## assetSourcePriorities

A map of selector to priority. The priority replaces the one from the mod's
metadata. Sources are loaded in ascending priority order, so higher priority
sources are loaded later and win file conflicts.

```
"assetSourcePriorities": {
  "Extended GUI": 10,
  "many_tabs2": 5
}
```

If more than one selector matches the same source, the highest value is used
and a warning is logged.

## protectedAssetPaths

A list of asset path prefixes. Mods may not replace, patch, or erase assets
under these prefixes; only the base game and OpenStarbound's own assets may.
A mod can still add a new asset under a protected prefix, but that asset is
protected as well.

```
"protectedAssetPaths": ["/interface/modsmenu/"]
```

The default is `["/interface/modsmenu/"]`, because the mods menu is
OpenStarbound's own UI and mods made for vanilla Starbound replace it with an
incompatible layout. Include a trailing `/` to protect a directory prefix;
otherwise the prefix also matches longer names. Prefixes are matched
case-insensitively; an explicit empty array disables the protection.

## Client / server consistency

`protectedAssetPaths` is part of the asset digest. Clients and servers must use
the same value, or they will report different asset digests.
