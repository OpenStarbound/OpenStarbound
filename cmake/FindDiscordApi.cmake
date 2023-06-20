# Variables defined by this module:
#
#  DISCORD_API_FOUND             System has discord api libs/headers
#  DISCORD_API_LIBRARY           The discord api library
#  DISCORD_API_INCLUDE_DIR       The location of discord api headers

find_path(DISCORD_API_ROOT_DIR
    NAMES include/discord_game_sdk.h
)

find_library(DISCORD_API_LIBRARY
    NAMES discord_game_sdk
    HINTS ${DISCORD_API_ROOT_DIR}/lib
)

find_path(DISCORD_API_INCLUDE_DIR
    NAMES discord_game_sdk.h
    HINTS ${DISCORD_API_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DiscordApi DEFAULT_MSG
    DISCORD_API_LIBRARY
    DISCORD_API_INCLUDE_DIR
)

mark_as_advanced(
    DISCORD_API_ROOT_DIR
    DISCORD_API_LIBRARY
    DISCORD_API_INCLUDE_DIR
)
