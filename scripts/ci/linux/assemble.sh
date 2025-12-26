#!/bin/sh -e

mkdir client_distribution
mkdir client_distribution/assets
mkdir client_distribution/assets/user

./dist/asset_packer -c scripts/packing.config assets/opensb client_distribution/assets/opensb.pak

mkdir client_distribution/mods
touch client_distribution/mods/mods_go_here

mkdir client_distribution/linux
cp \
  dist/starbound \
  dist/btree_repacker \
  dist/asset_packer \
  dist/asset_unpacker \
  dist/dump_versioned_json \
  dist/make_versioned_json \
  lib/linux/libdiscord_game_sdk.so \
  lib/linux/libsteam_api.so \
  scripts/ci/linux/sbinit.config \
  scripts/ci/linux/run-client.sh \
  scripts/steam_appid.txt \
  client_distribution/linux/
  
mkdir client_distribution/linux/.icon
cp \
  source/client/openstarbound.png \
  client_distribution/linux/.icon/

mkdir server_distribution
mkdir server_distribution/assets

mkdir server_distribution/mods
touch server_distribution/mods/mods_go_here

./dist/asset_packer -c scripts/packing.config -s assets/opensb server_distribution/assets/opensb.pak

mkdir server_distribution/linux

# makes the server function on older Linux versions (this is so stupid)
nm --dynamic --undefined-only --with-symbol-versions dist/starbound_server | grep GLIBC_2.29
./scripts/ci/linux/patchelf dist/starbound_server \
  --clear-symbol-version exp \
  --clear-symbol-version exp2 \
  --clear-symbol-version log \
  --clear-symbol-version log2 \
  --clear-symbol-version pow

cp \
  dist/starbound_server \
  dist/btree_repacker \
  scripts/ci/linux/run-server.sh \
  scripts/ci/linux/sbinit.config \
  scripts/steam_appid.txt \
  server_distribution/linux/

tar -cvf client.tar client_distribution
tar -cvf server.tar server_distribution
