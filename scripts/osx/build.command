#!/bin/sh

cd "$(dirname $0)/../.."

cd build
make -j3
