#!/bin/sh -e

mkdir client_win32_win64
cp -r \
  client_distribution/assets \
  client_distribution/doc \
  client_distribution/mods \
  client_distribution/tiled \
  client_distribution/win32 \
  client_distribution/win64 \
  client_win32_win64

mkdir client_linux
cp -r \
  client_distribution/assets \
  client_distribution/doc \
  client_distribution/mods \
  client_distribution/tiled \
  client_distribution/linux \
  client_linux

mkdir client_macos
cp -r \
  client_distribution/assets \
  client_distribution/doc \
  client_distribution/mods \
  client_distribution/tiled \
  client_distribution/osx \
  client_macos

mkdir server_win64
cp -r \
  server_distribution/assets \
  server_distribution/mods \
  server_distribution/win64 \
  server_win64

mkdir server_linux
cp -r \
  server_distribution/assets \
  server_distribution/mods \
  server_distribution/linux \
  server_linux
