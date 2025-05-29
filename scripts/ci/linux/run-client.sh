#!/bin/sh

cd "`dirname \"$0\"`"

export SDL_VIDEODRIVER=x11

LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./starbound "$@" & exit
