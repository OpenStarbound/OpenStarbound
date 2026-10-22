# Mod compatibility

Modpacks often contain one mod that replaces a whole JSON asset and another
mod that patches that same asset. When the replacing file is missing a key the
patch expects, the patch fails and the two mods cannot be used together.

OpenStarbound handles this in two ways.

## Deferred patches

A patch file that is discovered before the asset it targets is no longer
dropped. It is attached after all asset sources and load scripts have been
processed, so a patch-only mod that loads before the mod providing the base
asset still works. Patches added at runtime through the `assets.patch` API are
not affected.

## Merging full JSON overrides

When a mod replaces a whole JSON asset and a patch from a different mod cannot
be applied to it because a key is missing, the replacing file is merged over
the lower priority version of the same asset and the patch is retried.

Objects are merged recursively, values from the replacing file win, `null`
removes a key, and arrays and scalars are replaced. Images and other binary
assets are not affected.

The merge is only attempted after a patch actually fails. Assets where every
patch applies are still replaced exactly as before, so two intentionally
different layouts are not mixed. A patch from the same mod that provided the
override is applied normally and does not trigger the merge.

In the rare case where a JSON patch only fails after a Lua patch has changed
the asset, the Lua patches are run again on the merged version. Their contexts
are reset first, so state kept by a patch script is not applied twice.

This is enabled by default and can be disabled in `sbinit.config`:

```
"autoMergePatchedOverrides": false
```

The setting is part of the asset digest, so clients and servers must use the
same value.
