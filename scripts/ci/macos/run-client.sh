#!/bin/sh

cd "`dirname \"$0\"`"

DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:./" ./starbound "$@"
