set(CMAKE_C_COMPILER "clang-cl")
set(CMAKE_CXX_COMPILER "clang-cl")
include($ENV{VCPKG_ROOT}/scripts/toolchains/windows.cmake)

set(arch_flags "-mcrc32 -msse4.2 -maes -mpclmul")
string(APPEND CMAKE_C_FLAGS " ${arch_flags}")
string(APPEND CMAKE_CXX_FLAGS " ${arch_flags}")
unset(arch_flags)