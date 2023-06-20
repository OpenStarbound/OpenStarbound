#!/bin/sh

cd "$(dirname $0)/../.."

mkdir -p dist
cp scripts/osx/sbinit.config dist/

mkdir -p build
cd build

CC=clang CXX=clang++ /Applications/CMake.app/Contents/bin/cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=true \
  -DCMAKE_BUILD_TYPE=RelWithAsserts \
  -DSTAR_USE_JEMALLOC=ON \
  -DCMAKE_INCLUDE_PATH=../lib/osx/include \
  -DCMAKE_LIBRARY_PATH=../lib/osx/ \
  ../source
