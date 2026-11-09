#!/bin/sh -e
# Packs every folder of assets/mods into <output dir>/<name>.pak (run from the repo root).
# Usage: scripts/pack-mods.sh <asset_packer> <output dir> [asset_packer flags...]
packer="$1"; out="$2"; shift 2
mkdir -p "$out"
for dir in assets/mods/*/; do
  name="$(basename "$dir")"
  "$packer" -c scripts/packing.config "$@" "$dir" "$out/$name.pak"
done
