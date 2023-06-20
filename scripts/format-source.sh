#!/bin/sh

cd "`dirname \"$0\"`/../source"

: ${CLANG_FORMAT:=clang-format}

for file in *; do
  if [ $file != "extern" -a -d $file ]; then
    find $file \( -name '*.cpp' -or -name '*.hpp' \) -exec $CLANG_FORMAT -fallback-style=none -i {} \;
  fi
done
