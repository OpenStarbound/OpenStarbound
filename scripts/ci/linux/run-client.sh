#!/bin/sh

# Check for Zink support
if MESA_LOADER_DRIVER_OVERRIDE=zink glxinfo | grep -q 'renderer string.*zink'; then
    # Set environment variables for Zink
    export __GLX_VENDOR_LIBRARY_NAME=mesa
    export MESA_LOADER_DRIVER_OVERRIDE=zink
    export GALLIUM_DRIVER=zink
fi

cd "`dirname \"$0\"`"

LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./starbound "$@"
