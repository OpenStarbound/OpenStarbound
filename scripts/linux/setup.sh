#!/bin/sh

cd "`dirname \"$0\"`/../.."

mkdir -p dist
cp scripts/linux/sbinit.config dist/

mkdir -p build
cd build

if [ -d /usr/lib/ccache ]; then
  export PATH=/usr/lib/ccache/:$PATH
fi

LINUX_LIB_DIR=../lib/linux

cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DCMAKE_BUILD_TYPE=RelWithAsserts \
  -DSTAR_USE_JEMALLOC=ON \
  -DCMAKE_INCLUDE_PATH=$LINUX_LIB_DIR/include \
  -DCMAKE_LIBRARY_PATH=$LINUX_LIB_DIR/ \
  ../source

if [ $# -ne 0 ]; then 
  make -j$*
fi
