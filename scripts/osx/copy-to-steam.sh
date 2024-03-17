#!/bin/sh -e

cd "`dirname \"$0\"`/../../dist"

STEAM_INSTALL_DIR="$HOME/Library/Application Support/Steam/steamapps/common/Starbound - Unstable"

./asset_packer -c ../scripts/packing.config ../assets/packed ./packed.pak
mv packed.pak "$STEAM_INSTALL_DIR/assets/packed.pak"
cp starbound "$STEAM_INSTALL_DIR/osx/Starbound.app/Contents/MacOS/"
