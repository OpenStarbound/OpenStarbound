#pragma once

#ifdef STAR_SYSTEM_WINDOWS
#define NOMINMAX
#include <windows.h>
#endif

namespace Star {
#ifdef STAR_SYSTEM_WINDOWS
  DWORD WINAPI writeMiniDump(void* ExceptionInfo);
#endif
}