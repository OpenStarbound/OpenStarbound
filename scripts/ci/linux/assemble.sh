#!/bin/sh -e

mkdir client_distribution
mkdir client_distribution/assets

./dist/asset_packer -c scripts/packing.config assets/opensb client_distribution/assets/opensb.pak
cp -r assets/user client_distribution/assets/

mkdir client_distribution/mods
touch client_distribution/mods/mods_go_here

mkdir client_distribution/linux
cp \
  dist/starbound \
  dist/starbound_server \
  dist/btree_repacker \
  dist/asset_packer \
  dist/asset_unpacker \
  dist/dump_versioned_json \
  dist/make_versioned_json \
  dist/*.so \
  scripts/ci/linux/sbinit.config \
  scripts/ci/linux/run-client.sh \
  scripts/ci/linux/run-server.sh \
  client_distribution/linux/

mkdir server_distribution
mkdir server_distribution/assets

mkdir server_distribution/mods
touch server_distribution/mods/mods_go_here

./dist/asset_packer -c scripts/packing.config -s assets/opensb server_distribution/assets/opensb.pak

mkdir server_distribution/linux

cp \
  dist/starbound_server \
  dist/btree_repacker \
  dist/*.so \
  scripts/ci/linux/run-server.sh \
  scripts/ci/linux/sbinit.config \
  server_distribution/linux/

tar -cvf dist.tar dist
tar -cvf client.tar client_distribution
tar -cvf server.tar server_distribution
