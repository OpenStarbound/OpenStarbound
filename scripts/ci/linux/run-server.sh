#!/bin/sh

cd "`dirname \"$0\"`"

# Define hardcoded fallback terminals
FALLBACK_TERMS="
x-terminal-emulator
konsole
gnome-terminal.wrapper
xfce4-terminal.wrapper
koi8rxterm
lxterm
uxterm
xterm"

# Function to attempt launching server with a terminal
launch_with_terminal() {
  local term="$1"
  shift
  "$term" -e ./starbound_server "$@"
  return $?
}

# Check for config file
CONFIG_PATH=""
if [ -f "$XDG_CONFIG_HOME/xdg-terminals.list" ]; then
  CONFIG_PATH="$XDG_CONFIG_HOME/xdg-terminals.list"
elif [ -f "$HOME/.config/xdg-terminals.list" ]; then
  CONFIG_PATH="$HOME/.config/xdg-terminals.list"
fi

# Try config file terminal first (if it exists)
if [ -n "$CONFIG_PATH" ]; then
  CONFIG_TERM=$(grep -v '^#' "$CONFIG_PATH" | grep -v '^$' | tr '[:upper:]' '[:lower:]' | head -n 1 | sed 's/\.desktop$//' | awk -F. '{print $NF}')

  if [ -n "$CONFIG_TERM" ]; then
    launch_with_terminal "$CONFIG_TERM" "$@"
    if [ $? -eq 0 ]; then
      exit 0
    fi
  fi
fi

# Fall back to hardcoded terminal list
for term in $FALLBACK_TERMS; do
  launch_with_terminal "$term" "$@"
  if [ $? -eq 0 ]; then
    exit 0
  fi
done

exit 1
