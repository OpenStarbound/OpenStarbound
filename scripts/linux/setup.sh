#!/bin/sh

cd "`dirname \"$0\"`/../.."

mkdir -p dist
cp scripts/linux/sbinit.config dist/

mkdir -p build
cd build

if [ -d /usr/lib/ccache ]; then
  export PATH=/usr/lib/ccache/:$PATH
fi

export VCPKG_ROOT=/home/bottinator22/vcpkg/vcpkg

LINUX_LIB_DIR=../lib/linux

cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_INCLUDE_PATH=$LINUX_LIB_DIR/include \
  -DCMAKE_LIBRARY_PATH=$LINUX_LIB_DIR/ \
  --preset linux-release \
  ../source

cd linux-release
ninja
