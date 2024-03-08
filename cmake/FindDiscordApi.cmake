# Variables defined by this module:
#
#  DISCORD_API_LIBRARY           The discord api library

find_library(DISCORD_API_LIBRARY
    NAMES discord_game_sdk libdiscord_game_sdk
)

mark_as_advanced(
    DISCORD_API_LIBRARY
)