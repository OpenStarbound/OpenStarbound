#include "StarMiniDump.hpp"
#include "StarSignalHandler.hpp"
#include "StarFormat.hpp"
#include "StarString.hpp"
#include "StarLogging.hpp"

#include <windows.h>

namespace Star {

String g_sehMessage;

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
    static bool dumpWritten = false;
    if (!dumpWritten) {
      dumpWritten = true;
      writeMiniDump(ExceptionInfo);
    }
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
      g_sehMessage = strf("{} ({} @ {})",
          g_sehMessage,
          (PVOID)ExceptionInfo->ExceptionRecord->ExceptionCode,
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
    LONG result = EXCEPTION_CONTINUE_SEARCH;

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
      handleFatalError("Stack overflow detected", ExceptionInfo);
      result = EXCEPTION_CONTINUE_EXECUTION;
    }
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
      handleFatalError("Access violation detected", ExceptionInfo);
      result = EXCEPTION_CONTINUE_EXECUTION;
    }
    if ((ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)) {
      handleFatalError("Illegal instruction encountered", ExceptionInfo);
      result = EXCEPTION_CONTINUE_EXECUTION;
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
      result = EXCEPTION_CONTINUE_EXECUTION;
    }

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO) {
      handleFatalError("Division by zero", ExceptionInfo);
      result = EXCEPTION_CONTINUE_EXECUTION;
    }

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INT_OVERFLOW) {
      handleFatalError("Integer overflow", ExceptionInfo);
      result = EXCEPTION_CONTINUE_EXECUTION;
    }

    if ((ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_DATATYPE_MISALIGNMENT)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INVALID_DISPOSITION)
        || (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INVALID_HANDLE)) {
      handleFatalError("Error occurred", ExceptionInfo);
      result = EXCEPTION_CONTINUE_EXECUTION;
    }

    return result;
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
