#!/bin/sh

DIR="$(cd "$(dirname "$0")" && pwd)"
APPDIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"

if [ ! -d "$APPDIR" ]; then
  echo "Applications directory not found: $APPDIR"
  exit 1
fi

cat > "$APPDIR/io.github.openstarbound.openstarbound.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=OpenStarbound
Comment=Community-maintained fork of Starbound 1.4.4
Icon=$DIR/.icon/openstarbound.png
Exec=$DIR/run-client.sh
Terminal=false
Categories=Game;
Keywords=sandbox;space;survival;osb;
StartupNotify=true
StartupWMClass=io.github.openstarbound.openstarbound
PrefersNonDefaultGPU=true
EOF

echo "Created: $APPDIR/io.github.openstarbound.openstarbound.desktop"