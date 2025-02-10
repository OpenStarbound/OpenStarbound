# Variables defined by this module:
#
#  RAVI_LIBRARY           The Ravi library
#  RAVICOMP_LIBRARY       Ravicomp, dependency of Ravi
#  MIR_LIBRARY            MIR, dependency of Ravi
#  RAVI_INCLUDE_DIR       The location of Ravi headers

find_library(RAVI_LIBRARY
  NAMES libravi libravi.so libravi.dylib
)
find_library(RAVICOMP_LIBRARY
  NAMES libravicomp libravicomp.so libravicomp.dylib
)
find_library(MIR_LIBRARY
  NAMES libc2mir libc2mir.so libc2mir.dylib
)
find_path(RAVI_INCLUDE_DIR
  ravi/lua.h
)

mark_as_advanced(
    RAVI_LIBRARY
    RAVICOMP_LIBRARY
    MIR_LIBRARY
    RAVI_INCLUDE_DIR
)
