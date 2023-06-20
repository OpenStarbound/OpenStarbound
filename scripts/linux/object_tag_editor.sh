#!/bin/sh -e

cd "`dirname \"$0\"`/../.."

./dist/json_tool --find ./assets .object --edit '/tags' --input csv --after objectName --editor-image '/orientations/0/imageLayers/0/image' --editor-image '/orientations/0/dualImage' --editor-image '/orientations/0/image' --editor-image '/orientations/0/leftImage'
