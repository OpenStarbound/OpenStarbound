#!/bin/sh -e

cd "`dirname \"$0\"`/../../dist"

./map_grep "invalid=true" ../assets/packed/dungeons/
./map_grep "invalid=true" ../assets/devel/dungeons/
