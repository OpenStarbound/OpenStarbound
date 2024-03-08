# Variables defined by this module:
#
#  STEAM_API_LIBRARY           The steam api library
#  STEAM_API_INCLUDE_DIR       The location of steam api headers

find_library(STEAM_API_LIBRARY
  NAMES libsteam_api steam_api steam_api64
)
find_path(STEAM_API_INCLUDE_DIR
  steam/steam_api.h
)

mark_as_advanced(
    STEAM_API_LIBRARY
    STEAM_API_INCLUDE_DIR
)
