# - Try to find jemalloc headers and libraries.
#
# Usage of this module as follows:
#
#     find_package(JeMalloc)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  JEMALLOC_ROOT_DIR Set this variable to the root installation of
#                    jemalloc if the module has problems finding
#                    the proper installation path.
#
# Variables defined by this module:
#
#  JEMALLOC_FOUND             System has jemalloc libs/headers
#  JEMALLOC_LIBRARIES         The jemalloc library/libraries
#  JEMALLOC_INCLUDE_DIR       The location of jemalloc headers

find_path(JEMALLOC_ROOT_DIR
    NAMES include/jemalloc/jemalloc.h
)

find_library(JEMALLOC_LIBRARY
    NAMES jemalloc
    HINTS ${JEMALLOC_ROOT_DIR}/lib
)

find_path(JEMALLOC_INCLUDE_DIR
    NAMES jemalloc/jemalloc.h
    HINTS ${JEMALLOC_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JeMalloc DEFAULT_MSG
    JEMALLOC_LIBRARY
    JEMALLOC_INCLUDE_DIR
)

# Check if the JeMalloc library was compiled with the "je_" prefix.
include(CheckSymbolExists)
set(CMAKE_REQUIRED_INCLUDES ${JEMALLOC_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${JEMALLOC_LIBRARY})
check_symbol_exists("je_malloc" "jemalloc/jemalloc.h" _jemalloc_is_prefixed)
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_LIBRARIES)

if(_jemalloc_is_prefixed)
    set(JEMALLOC_IS_PREFIXED TRUE)
endif()

unset(_jemalloc_is_prefixed)

mark_as_advanced(
    JEMALLOC_ROOT_DIR
    JEMALLOC_LIBRARY
    JEMALLOC_INCLUDE_DIR
    JEMALLOC_IS_PREFIXED
)
