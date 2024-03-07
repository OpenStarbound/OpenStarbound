#!/bin/sh

cd "`dirname \"$0\"`"

terms="
x-terminal-emulator
konsole
gnome-terminal.wrapper
xfce4-terminal.wrapper
koi8rxterm
lxterm
uxterm
xterm"

for term in $terms; do
  $term -e ./starbound_server $@
  if [ $? -eq 0 ]; then
    exit 0;
  fi
done

exit 1
