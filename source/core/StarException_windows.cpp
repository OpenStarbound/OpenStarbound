#include "StarException.hpp"
#include "StarOutputProxy.hpp"
#include "StarLogging.hpp"
#include "StarCasting.hpp"
#include "StarString_windows.hpp"

#include <DbgHelp.h>
#include <sstream>

namespace Star {

struct WindowsSymInitializer {
  WindowsSymInitializer() {
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
      fatalError("SymInitialize failed", false);
  }
};
static WindowsSymInitializer g_windowsSymInitializer;

struct DbgHelpLock {
  DbgHelpLock() {
    InitializeCriticalSection(&criticalSection);
  }

  void lock() {
    EnterCriticalSection(&criticalSection);
  }

  void unlock() {
    LeaveCriticalSection(&criticalSection);
  }

  CRITICAL_SECTION criticalSection;
};
static DbgHelpLock g_dbgHelpLock;

static size_t const StackLimit = 256;

typedef pair<Array<DWORD64, StackLimit>, size_t> StackCapture;

inline StackCapture captureStack() {
  HANDLE process = GetCurrentProcess();
  HANDLE thread = GetCurrentThread();

  CONTEXT context;
  DWORD image;
  STACKFRAME64 stackFrame;

  memset(&context, 0, sizeof(CONTEXT));
  context.ContextFlags = CONTEXT_FULL;

  ZeroMemory(&stackFrame, sizeof(STACKFRAME64));
  stackFrame.AddrPC.Mode = AddrModeFlat;
  stackFrame.AddrReturn.Mode = AddrModeFlat;
  stackFrame.AddrFrame.Mode = AddrModeFlat;
  stackFrame.AddrStack.Mode = AddrModeFlat;

#ifdef STAR_ARCHITECTURE_I386

#ifdef STAR_COMPILER_MSVC
  __asm {
    mov [context.Ebp], ebp;
    mov [context.Esp], esp;
    call next;
  next:
    pop [context.Eip];
  }
#else
  DWORD eip_val = 0;
  DWORD esp_val = 0;
  DWORD ebp_val = 0;

  __asm__ __volatile__("call 1f\n1: pop %0" : "=g"(eip_val));
  __asm__ __volatile__("movl %%esp, %0" : "=g"(esp_val));
  __asm__ __volatile__("movl %%ebp, %0" : "=g"(ebp_val));

  context.Eip = eip_val;
  context.Esp = esp_val;
  context.Ebp = ebp_val;
#endif

  image = IMAGE_FILE_MACHINE_I386;

  stackFrame.AddrPC.Offset = context.Eip;
  stackFrame.AddrReturn.Offset = context.Eip;
  stackFrame.AddrFrame.Offset = context.Ebp;
  stackFrame.AddrStack.Offset = context.Esp;

#elif defined STAR_ARCHITECTURE_X86_64

  RtlCaptureContext(&context);

  image = IMAGE_FILE_MACHINE_AMD64;

  stackFrame.AddrPC.Offset = context.Rip;
  stackFrame.AddrReturn.Offset = context.Rip;
  stackFrame.AddrFrame.Offset = context.Rbp;
  stackFrame.AddrStack.Offset = context.Rsp;

#endif

  g_dbgHelpLock.lock();

  Array<DWORD64, StackLimit> addresses;
  size_t count = 0;
  for (size_t i = 0; i < StackLimit; i++) {
    if (!StackWalk64(image, process, thread, &stackFrame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
      break;
    if (stackFrame.AddrPC.Offset == 0)
      break;
    addresses[i] = stackFrame.AddrPC.Offset;
    ++count;
  }

  g_dbgHelpLock.unlock();

  return {addresses, count};
}

OutputProxy outputStack(StackCapture stack) {
  return OutputProxy([stack = move(stack)](std::ostream & os) {
    HANDLE process = GetCurrentProcess();
    g_dbgHelpLock.lock();
    for (size_t i = 0; i < stack.second; ++i) {
      char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
      PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
      symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
      symbol->MaxNameLen = MAX_SYM_NAME;

      DWORD64 displacement = 0;
      format(os, "[{}] {}", i, (void*)stack.first[i]);
      if (SymFromAddr(process, stack.first[i], &displacement, symbol))
        format(os, " {}", symbol->Name);

      if (i + 1 < stack.second)
        os << std::endl;
    }

    if (stack.second == StackLimit)
      os << std::endl << "[Stack Output Limit Reached]";

    g_dbgHelpLock.unlock();
  });
}

StarException::StarException() noexcept : StarException(std::string("StarException")) {}

StarException::~StarException() noexcept {}

StarException::StarException(std::string message, bool genStackTrace) noexcept : StarException("StarException", move(message), genStackTrace) {}

StarException::StarException(std::exception const& cause) noexcept
    : StarException("StarException", std::string(), cause) {}

StarException::StarException(std::string message, std::exception const& cause) noexcept
    : StarException("StarException", move(message), cause) {}

const char* StarException::what() const throw() {
  if (m_whatBuffer.empty()) {
    std::ostringstream os;
    m_printException(os, false);
    m_whatBuffer = os.str();
  }
  return m_whatBuffer.c_str();
}

StarException::StarException(char const* type, std::string message, bool genStackTrace) noexcept {
  auto printException = [](
      std::ostream& os, bool fullStacktrace, char const* type, std::string message, Maybe<StackCapture> stack) {
    os << "(" << type << ")";
    if (!message.empty())
      os << " " << message;

    if (fullStacktrace && stack) {
      os << std::endl;
      os << outputStack(*stack);
    }
  };

  m_printException = bind(printException, _1, _2, type, move(message), genStackTrace ? captureStack() : Maybe<StackCapture>());
}

StarException::StarException(char const* type, std::string message, std::exception const& cause) noexcept
    : StarException(type, move(message)) {
  auto printException = [](std::ostream& os,
      bool fullStacktrace,
      function<void(std::ostream&, bool)> self,
      function<void(std::ostream&, bool)> cause) {
    self(os, fullStacktrace);
    os << std::endl << "Caused by: ";
    cause(os, fullStacktrace);
  };

  std::function<void(std::ostream&, bool)> printCause;
  if (auto starException = as<StarException>(&cause)) {
    printCause = bind(starException->m_printException, _1, _2);
  } else {
    printCause = bind([](std::ostream& os, bool, std::string causeWhat) {
      os << "std::exception: " << causeWhat;
    }, _1, _2, std::string(cause.what()));
  }

  m_printException = bind(printException, _1, _2, m_printException, move(printCause));
}

std::string printException(std::exception const& e, bool fullStacktrace) {
  std::ostringstream os;
  printException(os, e, fullStacktrace);
  return os.str();
}

void printException(std::ostream& os, std::exception const& e, bool fullStacktrace) {
  if (auto starException = as<StarException>(&e))
    starException->m_printException(os, fullStacktrace);
  else
    os << "std::exception: " << e.what();
}

OutputProxy outputException(std::exception const& e, bool fullStacktrace) {
  if (auto starException = as<StarException>(&e))
    return OutputProxy(bind(starException->m_printException, _1, fullStacktrace));
  else
    return OutputProxy(
        bind([](std::ostream& os, std::string what) { os << "std::exception: " << what; }, _1, std::string(e.what())));
}

void printStack(char const* message) {
  Logger::info("Stack Trace ({})...\n{}", message, outputStack(captureStack()));
}

void fatalError(char const* message, bool showStackTrace) {
  std::ostringstream ss;
  ss << "Fatal Error: " << message << std::endl;
  if (showStackTrace)
    ss << outputStack(captureStack());

  Logger::log(LogLevel::Error, ss.str().c_str());
  MessageBoxW(NULL, stringToUtf16(ss.str()).get(), stringToUtf16("Error").get(), MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

  std::abort();
}

void fatalException(std::exception const& e, bool showStackTrace) {
  std::ostringstream ss;
  ss << "Fatal Exception caught: " << outputException(e, showStackTrace) << std::endl;
  if (showStackTrace)
    ss << "Caught at:" << std::endl << outputStack(captureStack());

  Logger::log(LogLevel::Error, ss.str().c_str());
  MessageBoxW(NULL, stringToUtf16(ss.str()).get(), stringToUtf16("Error").get(), MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

  std::abort();
}

}
