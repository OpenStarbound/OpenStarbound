#pragma once

#ifdef STAR_SYSTEM_WINDOWS
#include <windows.h>
#endif

namespace Star {
#ifdef STAR_SYSTEM_WINDOWS
  DWORD WINAPI writeMiniDump(void* ExceptionInfo);
#endif
}