#!/bin/sh -e

cd macos_binaries

cp ../scripts/osx/sbinit.config .

./core_tests
./game_tests

