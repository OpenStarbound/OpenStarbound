#!/bin/sh -e

cd linux_binaries

cp ../scripts/linux/sbinit.config .

./core_tests
./game_tests
