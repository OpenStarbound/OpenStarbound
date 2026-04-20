#!/bin/bash
#
# WARNING: do not execute this file unless you are aware of what you're doing.
# This file is meant to be executed by ctest.

export LIBGL_FB=3
export LIBGL_SILENTSTUB=1
export LIBGL_NOBANNER=1
export LIBGL_NOERROR=1

tar xf ../traces/$1.tgz
apitrace dump-images --calls="$2" $1.trace >/dev/null
exit_status=$?
test $exit_status -ne 0 && echo "Test returned a non-zero status $exit_status (segmentation fault?)"
rm $1.trace

if [[ -f $1.$2.png ]]
then
	mv $1.$2.png $1.$2.$LIBGL_ES.png

	EXTRACT=""
	if [[ ! -z "$4" ]]
	then
		EXTRACT="-extract $4"
	fi
	result=$(compare -metric AE -fuzz 20% $EXTRACT ../refs/$1.$2.png $1.$2.$LIBGL_ES.png diff_$1_GLES$LIBGL_ES.png 2>&1)

	if [[ ! "$result" -lt "$3" ]]
	then
		echo -n "$result pixels of difference"
		exit 1
	fi

	[[ -e diff_$1_GLES$LIBGL_ES.png ]] && rm diff_$1_GLES$LIBGL_ES.png
	[[ -e $1.$2.$LIBGL_ES.png ]] && rm $1.$2.$LIBGL_ES.png
else
	echo -n "Failed to get snapshot"
	exit 1
fi

exit 0
