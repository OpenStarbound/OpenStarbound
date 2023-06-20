# Variables defined by this module:
#
#  STEAM_API_FOUND             System has steam api libs/headers
#  STEAM_API_LIBRARY           The steam api library
#  STEAM_API_INCLUDE_DIR       The location of steam api headers

find_path(STEAM_API_ROOT_DIR
    NAMES include/steam/steam_api.h
)

find_library(STEAM_API_LIBRARY
    NAMES steam_api
    HINTS ${STEAM_API_ROOT_DIR}/lib
)

find_path(STEAM_API_INCLUDE_DIR
    NAMES steam/steam_api.h
    HINTS ${STEAM_API_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SteamApi DEFAULT_MSG
    STEAM_API_LIBRARY
    STEAM_API_INCLUDE_DIR
)

mark_as_advanced(
    STEAM_API_ROOT_DIR
    STEAM_API_LIBRARY
    STEAM_API_INCLUDE_DIR
)
