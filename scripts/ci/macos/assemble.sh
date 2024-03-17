#!/bin/sh -e

mkdir client_distribution
mkdir client_distribution/assets
mkdir client_distribution/assets/user

./dist/asset_packer -c scripts/packing.config assets/opensb client_distribution/assets/opensb.pak

mkdir client_distribution/mods
touch client_distribution/mods/mods_go_here

mkdir client_distribution/osx
cp -LR scripts/ci/macos/Starbound.app client_distribution/osx/
mkdir client_distribution/osx/Starbound.app/Contents/MacOS
cp dist/starbound client_distribution/osx/Starbound.app/Contents/MacOS/
cp dist/*.dylib client_distribution/osx/Starbound.app/Contents/MacOS/
cp \
  dist/starbound_server \
  dist/btree_repacker \
  dist/asset_packer \
  dist/asset_unpacker \
  dist/dump_versioned_json \
  dist/make_versioned_json \
  dist/planet_mapgen \
  scripts/ci/macos/sbinit.config \
  scripts/ci/macos/run-server.sh \
  client_distribution/osx/