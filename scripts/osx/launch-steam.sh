#!/bin/sh -e

cd "`dirname \"$0\"`/../../dist"

cp ../scripts/steam_appid.txt .

DYLD_INSERT_LIBRARIES=~/Library/Application\ Support/Steam/Steam.AppBundle/Steam/Contents/MacOS/gameoverlayrenderer.dylib DYLD_LIBRARY_PATH=../lib/osx/ $@
