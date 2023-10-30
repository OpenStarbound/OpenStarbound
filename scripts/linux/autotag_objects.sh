#!/bin/sh -e

cd "`dirname \"$0\"`/../.."

for OBJECT in $(find assets/ -name *.object); do

  EXISTING_TAGS=$(./dist/json_tool --opt '/tags' "$OBJECT")
  if test "x$EXISTING_TAGS" != "x"; then
    echo "Skipping $OBJECT; it already has tags..."
    continue
  fi

  echo "Automatically tagging $OBJECT"

  RACE_TAGS=$(./dist/json_tool --opt '/race' "$OBJECT" --array)
  CATEGORY_TAGS=$(./dist/json_tool --opt '/category' "$OBJECT" --array)
  TYPE_TAGS=$(./dist/json_tool --opt '/objectType' "$OBJECT" --array)

  TAGS=$(./dist/json_tool -j "$RACE_TAGS" -j "$CATEGORY_TAGS" -j "$TYPE_TAGS" --get '/*' --array-unique)
  ./dist/json_tool -i --set '/tags' "$TAGS" --after objectName "$OBJECT"
done
