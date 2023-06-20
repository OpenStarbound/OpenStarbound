#!/bin/bash

cd "`dirname \"$0\"`"

cd ../assets/packed/tiles/;

THISMAT=0;
for i in $(grep materialId */*.material | awk '{print $4}' | sort -n ); 
  do
  THISMAT=$(($THISMAT+1)); 
  if [ ${i%?} -ne $THISMAT ]; 
    then 
    echo "Skipped $THISMAT to $((${i%?}-1))"; 
    THISMAT=${i%?};
  fi;
done;
