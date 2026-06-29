set(CMAKE_C_COMPILER "clang-cl.exe")
set(CMAKE_CXX_COMPILER "clang-cl.exe")
# stupid but windows.cmake inserts /MP if it doesn't match 'clang-cl.exe'
include($ENV{VCPKG_ROOT}/scripts/toolchains/windows.cmake)
# and some ports do not work unless we match 'clang-cl'
set(CMAKE_C_COMPILER "clang-cl")
set(CMAKE_CXX_COMPILER "clang-cl")

set(ignore_werror "/WX-")
cmake_language(DEFER CALL add_compile_options "${ignore_werror}")

set(arch_flags "-mcrc32 -msse4.2 -maes -mpclmul")
string(APPEND CMAKE_C_FLAGS " ${arch_flags} ${ignore_werror}")
string(APPEND CMAKE_CXX_FLAGS " ${arch_flags} ${ignore_werror}")
unset(arch_flags)