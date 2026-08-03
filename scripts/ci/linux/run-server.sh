#!/bin/sh

cd "`dirname \"$0\"`"

terms="
xdg-terminal-exec
x-terminal-emulator
konsole
gnome-terminal.wrapper
xfce4-terminal.wrapper
koi8rxterm
lxterm
uxterm
xterm"

for term in $terms; do
  command -v "$term" > /dev/null 2>&1 || continue
  "$term" -- ./starbound_server "$@" && exit 0
done

exit 1
