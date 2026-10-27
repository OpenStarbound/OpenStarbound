#!/bin/sh
# Launches OpenStarbound with a writable data directory.
# Put the vanilla packed.pak in "$data/assets" (and optional mods in "$data/mods").
set -e

data="${OPENSTARBOUND_DATA:-${XDG_DATA_HOME:-$HOME/.local/share}/OpenStarbound}"
mkdir -p "$data/assets" "$data/mods" "$data/storage" "$data/logs"
ln -sf @out@/share/openstarbound/assets/opensb.pak "$data/assets/opensb.pak"

cat > "$data/sbinit.config" <<CFG
{
  "assetDirectories" : [ "$data/assets/", "$data/mods/" ],
  "storageDirectory" : "$data/storage/",
  "logDirectory" : "$data/logs/"
}
CFG

if [ ! -e "$data/assets/packed.pak" ]; then
  echo "openstarbound: copy your Starbound packed.pak to $data/assets/" >&2
fi

exec @out@/libexec/openstarbound/@binary@ -bootconfig "$data/sbinit.config" "$@"
