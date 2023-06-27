INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    fmt
    lua
  )

SET (star_extern_HEADERS
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
