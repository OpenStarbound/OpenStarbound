#!/bin/sh -e

mkdir client_distribution
mkdir client_distribution/assets
mkdir client_distribution/tiled

./linux_binaries/asset_packer -c scripts/packing.config assets/packed client_distribution/assets/packed.pak
cp -r assets/user client_distribution/assets/

cp -r tiled/packed client_distribution/tiled/

cp -r doc client_distribution/doc

mkdir client_distribution/mods
touch client_distribution/mods/mods_go_here

mkdir client_distribution/win64
cp -r \
  windows_binaries/starbound.exe \
  windows_binaries/starbound.pdb \
  windows_binaries/starbound_server.exe \
  windows_binaries/mod_uploader.exe \
  windows_binaries/*.dll \
  windows_binaries/iconengines \
  windows_binaries/imageformats \
  windows_binaries/platforms \
  windows_binaries/translations \
  scripts/gitlab-ci/windows/sbinit.config \
  client_distribution/win64/

mkdir client_distribution/osx
cp -LR scripts/gitlab-ci/macos/Starbound.app client_distribution/osx/
mkdir client_distribution/osx/Starbound.app/Contents/MacOS
cp macos_binaries/starbound client_distribution/osx/Starbound.app/Contents/MacOS/
cp macos_binaries/*.dylib client_distribution/osx/Starbound.app/Contents/MacOS/
cp \
  macos_binaries/starbound_server \
  macos_binaries/asset_packer \
  macos_binaries/asset_unpacker \
  macos_binaries/dump_versioned_json \
  macos_binaries/make_versioned_json \
  macos_binaries/planet_mapgen \
  scripts/gitlab-ci/macos/sbinit.config \
  scripts/gitlab-ci/macos/run-server.sh \
  client_distribution/osx/

mkdir client_distribution/linux
cp \
  linux_binaries/starbound \
  linux_binaries/starbound_server \
  linux_binaries/asset_packer \
  linux_binaries/asset_unpacker \
  linux_binaries/dump_versioned_json \
  linux_binaries/make_versioned_json \
  linux_binaries/planet_mapgen \
  linux_binaries/*.so \
  scripts/gitlab-ci/linux/sbinit.config \
  scripts/gitlab-ci/linux/run-client.sh \
  scripts/gitlab-ci/linux/run-server.sh \
  client_distribution/linux/

mkdir server_distribution
mkdir server_distribution/assets

mkdir server_distribution/mods
touch server_distribution/mods/mods_go_here

./linux_binaries/asset_packer -c scripts/packing.config -s assets/packed server_distribution/assets/packed.pak

mkdir server_distribution/win64
mkdir server_distribution/linux

cp \
  linux_binaries/starbound_server \
  linux_binaries/*.so \
  scripts/gitlab-ci/linux/run-server.sh \
  scripts/gitlab-ci/linux/sbinit.config \
  server_distribution/linux/

cp \
  windows_binaries/starbound_server.exe \
  windows_binaries/*.dll \
  scripts/gitlab-ci/windows/sbinit.config \
  server_distribution/win64/
