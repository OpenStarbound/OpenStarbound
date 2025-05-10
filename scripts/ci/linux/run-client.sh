#!/bin/sh

# Check if environment variables are already set
if [ -n "${__GLX_VENDOR_LIBRARY_NAME}" ] || [ -n "${MESA_LOADER_DRIVER_OVERRIDE}" ] || [ -n "${GALLIUM_DRIVER}" ]; then :
else
    # Check for Vulkan support
    if command -v vulkaninfo &> /dev/null; then
        # Check for Zink support
        if __GLX_VENDOR_LIBRARY_NAME=mesa MESA_LOADER_DRIVER_OVERRIDE=zink GALLIUM_DRIVER=zink glxinfo | grep -q 'renderer string: zink'; then
            # Set environment variables for Zink
            export __GLX_VENDOR_LIBRARY_NAME=mesa MESA_LOADER_DRIVER_OVERRIDE=zink GALLIUM_DRIVER=zink
        fi
    fi
fi

cd "`dirname \"$0\"`"

LD_LIBRARY_PATH=".:$LD_LIBRARY_PATH" ./starbound "$@"
