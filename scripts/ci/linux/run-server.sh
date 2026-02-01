#!/bin/sh

cd "`dirname \"$0\"`"

if [ -f "$XDG_CONFIG_HOME/xdg-terminals.list" ]; then
    terminals_config_path="$XDG_CONFIG_HOME/xdg-terminals.list"
elif [ -f "$HOME/.config/xdg-terminals.list" ]; then
    terminals_config_path="$HOME/.config/xdg-terminals.list"
else
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
            exit 0
        fi
    done
    exit 1
fi

term=$(grep -v '^#' "$terminals_config_path" | grep -v '^$' | tr '[:upper:]' '[:lower:]' | head -n 1 | sed 's/\.desktop$//' | awk -F. '{print $NF}')

if [ -n "$term" ]; then
    $term -e ./starbound_server $@
    if [ $? -eq 0 ]; then
        exit 0
    fi
fi

exit 1
