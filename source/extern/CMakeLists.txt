SET (OPUS_INSTALL_PKG_CONFIG_MODULE OFF)
SET (OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF)
SET (OPUS_X86_MAY_HAVE_AVX OFF)
SET (OPUS_X86_MAY_HAVE_SSE4_1 OFF)
SET (OPUS_ENABLE_FLOAT_API ON)
SET (OPUS_STACK_PROTECTOR OFF)
SET (OPUS_NONTHREADSAFE_PSEUDOSTACK OFF)
SET (OPUS_USE_ALLOCA ON)

ADD_SUBDIRECTORY (opus)

IF (OPUS_NONTHREADSAFE_PSEUDOSTACK)
    MESSAGE (FATAL_ERROR "Opus should not be using NONTHREADSAFE_PSEUDOSTACK")
ENDIF ()

INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    opus/include
    fmt
    lua
  )

SET (star_extern_HEADERS
    curve25519/include/curve25519_dh.h
    curve25519/include/ed25519_signature.h
    curve25519/include/external_calls.h
    fmt/core.h
    fmt/format.h
    fmt/format-inl.h
    fmt/ostream.h
    fmt/printf.h
    fmt/ranges.h
    fmt/std.h
    lauxlib.h
    lua.h
    lua.hpp
    luaconf.h
    lualib.h
    tinyformat.h
    xxhash.h
  )

SET (star_extern_SOURCES
    curve25519/source/sha512.c
    curve25519/source/curve25519_dh.c
    curve25519/source/curve25519_mehdi.c
    curve25519/source/curve25519_order.c
    curve25519/source/curve25519_utils.c
    curve25519/source/custom_blind.c
    curve25519/source/ed25519_sign.c
    curve25519/source/ed25519_verify.c
    xxhash.c
    fmt/format.cc
    lua/lapi.c
    lua/lauxlib.c
    lua/lbaselib.c
    lua/lbitlib.c
    lua/lcode.c
    lua/lcorolib.c
    lua/lctype.c
    lua/ldblib.c
    lua/ldebug.c
    lua/ldo.c
    lua/ldump.c
    lua/lfunc.c
    lua/lgc.c
    lua/linit.c
    lua/liolib.c
    lua/llex.c
    lua/lmathlib.c
    lua/lmem.c
    lua/loadlib.c
    lua/lobject.c
    lua/lopcodes.c
    lua/loslib.c
    lua/lparser.c
    lua/lstate.c
    lua/lstring.c
    lua/lstrlib.c
    lua/ltable.c
    lua/ltablib.c
    lua/ltm.c
    lua/lundump.c
    lua/lutf8lib.c
    lua/lvm.c
    lua/lzio.c
  )

ADD_LIBRARY (star_extern OBJECT ${star_extern_SOURCES} ${star_extern_HEADERS})
TARGET_LINK_LIBRARIES(star_extern PUBLIC opus)