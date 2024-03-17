#!/bin/sh

cd "`dirname \"$0\"`/../source"

for file in *; do
  if [ $file != "extern" -a -d $file ]; then
    #  This is not bulletproof, this will break if the last character on a line
    #  is an *escaped* quote.
    find $file \( -name '*.cpp' -or -name '*.hpp' \) -exec perl -0777 -i -pe 's/\"\s*\n\s*\"//igs' {} \;
  fi
done
