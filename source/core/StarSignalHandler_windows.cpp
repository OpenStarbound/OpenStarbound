#include "StarSignalHandler.hpp"
#include "StarFormat.hpp"
#include "StarString.hpp"
#include "StarLogging.hpp"

#include <windows.h>
#include "minidumpapiset.h"

namespace Star {

String g_sehMessage;

static DWORD WINAPI writeMiniDump(void* ExceptionInfo) {
  auto hFile = CreateFileA("starbound.dmp", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_HANDLE_VALUE)
    return 0;
  MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo{};
  dumpExceptionInfo.ThreadId = GetCurrentThreadId();
  dumpExceptionInfo.ExceptionPointers = (PEXCEPTION_POINTERS)ExceptionInfo;
  dumpExceptionInfo.ClientPointers = FALSE;
  MiniDumpWriteDump(
    GetCurrentProcess(),
    GetCurrentProcessId(),
    hFile,
    MiniDumpNormal,
    &dumpExceptionInfo,
    NULL,
    NULL);
  CloseHandle(hFile);
  if (dumpExceptionInfo.ExceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
    MessageBoxA(NULL, "Stack overflow encountered\nA minidump has been generated", NULL, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
  }
  return 0;
};

struct SignalHandlerImpl {
  bool handlingFatal;
  bool handlingInterrupt;
  bool interrupted;

  PVOID handler;

  SignalHandlerImpl() : handlingFatal(false), handlingInterrupt(false), interrupted(false) {}

  ~SignalHandlerImpl() {
    setHandleFatal(false);
    setHandleInterrupt(false);
  }

  void setHandleFatal(bool b) {
    handlingFatal = b;

    if (handler) {
      RemoveVectoredExceptionHandler(handler);
      handler = nullptr;
    }

    if (handlingFatal)
      handler = AddVectoredExceptionHandler(1, vectoredExceptionHandler);
  }

  void setHandleInterrupt(bool b) {
    handlingInterrupt = b;

    SetConsoleCtrlHandler(nullptr, false);

    if (handlingInterrupt)
      SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleCtrlHandler, true);
  }

  static void sehTrampoline() {
    fatalError(g_sehMessage.utf8Ptr(), true);
  }

  static void handleFatalError(String const& msg, PEXCEPTION_POINTERS ExceptionInfo) {
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
      String mode;
      DWORD modeFlag = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
      if (modeFlag == 0)
        mode = "Read";
      else if (modeFlag == 1)
        mode = "Write";
      else if (modeFlag == 8)
        mode = "Execute";
      else
        mode = strf("Mode({})", modeFlag);
      g_sehMessage = strf("Access violation detected at {} ({} of address {})",
          ExceptionInfo->ExceptionRecord->ExceptionAddress,
          mode,
          (PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
    } else {
      g_sehMessage = msg;
      g_sehMessage = strf("{} (%p @ {})",
          g_sehMessage,
          ExceptionInfo->ExceptionRecord->ExceptionCode,
          ExceptionInfo->ExceptionRecord->ExceptionAddress);
      for (DWORD i = 0; i < ExceptionInfo->ExceptionRecord->NumberParameters; i++)
        g_sehMessage = strf("{} [{}]", g_sehMessage, (PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[i]);
    }

// setup a hijack into our own trampoline as if the failure actually was a
// function call
#ifdef STAR_ARCHITECTURE_X86_64
    DWORD64 rsp = ExceptionInfo->ContextRecord->Rsp - 8;
    DWORD64 rip = ExceptionInfo->ContextRecord->Rip; // an offset avoid the issue of gdb thinking
    // the error is one statement too early, but
    // the offset is instruction dependent, and we
    // don't know its size + 1;
    *((DWORD64*)rsp) = rip;
    ExceptionInfo->ContextRecord->Rsp = rsp;
    ExceptionInfo->ContextRecord->Rip = (DWORD64)&sehTrampoline;
#else
    DWORD esp = ExceptionInfo->ContextRecord->Esp - 4;
    DWORD eip = ExceptionInfo->ContextRecord->Eip; // an offset avoid the issue of gdb thinking the
    // error is one statement too early, but the
    // offset is instruction dependent, and we don't
    // know its size + 1;
    *((DWORD*)esp) = eip;
    ExceptionInfo->ContextRecord->Esp = esp;
    ExceptionInfo->ContextRecord->Eip = (DWORD)&sehTrampoline;
#endif
  }

  static LONG CALLBACK vectoredExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo) {
    auto thread = CreateThread(NULL, 0, writeMiniDump, (void*)ExceptionInfo, 0, NULL);
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
      handleFatalError("Access violation detected", ExceptionInfo);
      return EXCEPTION_CONTINUE_EXECUTION;
    }
    if ((ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)) {
      handleFatalError("Illegal instruction encountered", ExceptionInfo);
      return EXCEPTION_CONTINUE_EXECUTION;
    }

    if ((ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_DENORMAL_OPERAND)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_DIVIDE_BY_ZERO)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_INEXACT_RESULT)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_INVALID_OPERATION)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_OVERFLOW)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_STACK_CHECK)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_UNDERFLOW)

            ) {
      handleFatalError("Floating point exception", ExceptionInfo);
      return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO) {
      handleFatalError("Division by zero", ExceptionInfo);
      return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INT_OVERFLOW) {
      handleFatalError("Integer overflow", ExceptionInfo);
      return EXCEPTION_CONTINUE_EXECUTION;
    }

    if ((ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_DATATYPE_MISALIGNMENT)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INVALID_DISPOSITION)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INVALID_HANDLE)) {
      handleFatalError("Error occured", ExceptionInfo);
      return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (thread != NULL) {
      WaitForSingleObject(thread, 10000);
      CloseHandle(thread);
    }
    return EXCEPTION_CONTINUE_SEARCH;
  }

  static BOOL WINAPI consoleCtrlHandler(DWORD) {
    if (SignalHandler::s_singleton)
      SignalHandler::s_singleton->interrupted = true;
    return true;
  }
};

SignalHandlerImplUPtr SignalHandler::s_singleton;

SignalHandler::SignalHandler() {
  if (s_singleton)
    throw StarException("Singleton SignalHandler has been constructed twice!");

  s_singleton = make_unique<SignalHandlerImpl>();
}

SignalHandler::~SignalHandler() {
  s_singleton.reset();
}

void SignalHandler::setHandleFatal(bool handleFatal) {
  s_singleton->setHandleFatal(handleFatal);
}

bool SignalHandler::handlingFatal() const {
  return s_singleton->handlingFatal;
}

void SignalHandler::setHandleInterrupt(bool handleInterrupt) {
  s_singleton->setHandleInterrupt(handleInterrupt);
}

bool SignalHandler::handlingInterrupt() const {
  return s_singleton->handlingInterrupt;
}

bool SignalHandler::interruptCaught() const {
  return s_singleton->interrupted;
}

}
