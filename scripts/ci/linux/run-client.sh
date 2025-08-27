#!/bin/sh

cd "`dirname \"$0\"`"

LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./starbound "$@" & exit
