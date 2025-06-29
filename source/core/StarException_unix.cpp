#include "StarException.hpp"
#include "StarCasting.hpp"
#include "StarLogging.hpp"

#include <execinfo.h>
#include <cstdlib>
#include <signal.h>
#include <backtrace.h>

namespace Star {

static void error_callback(void* data, const char* message, int error_number) {
  if (error_number == -1) {
    fprintf(stderr, "If you want backtraces, you have to compile with -g\n\n");
    _Exit(1);
  } else {
    fprintf(stderr, "Backtrace error %d: %s\n", error_number, message);
  };
};

static inline std::string captureBacktrace() {
  static backtrace_state* state = nullptr;

  auto full_callback = [](void* data, uintptr_t pc, const char* pathname, int line, const char* function) -> int {
    if (!data)
      return 0;
    auto& out = *(std::pair<std::string, bool>*)data;
    if (pathname != NULL || function != NULL || line != 0) {
      if (out.second) {
        out.first += "  ...\n";
        out.second = false;
      };
      const char* filename = rindex(pathname, '/');
      out.first += strf("{}:{} ({}) [{}]\n", filename ? ++filename : pathname, line, function, fmt::ptr((void*)pc));
    } else
      out.second = true;
    return 0;
  };

  auto error_callback = [](void* data, const char* message, int error) {
    std::string str;
    if (error == -1)
      str = "Missing symbols\n";
    else
      str = strf("Backtrace error #{}: {}\n", error, message);
    if (data)
      ((std::pair<std::string, bool>*)data)->first.append(str);
  };

  if (!state)
    state = backtrace_create_state(NULL, 1, error_callback, NULL);
  std::pair<std::string, bool> out;
  backtrace_full(state, 0, full_callback, error_callback, &out);
  if (out.second)
    out.first += "  ...\n";
  return std::move(out.first);
}

static size_t const StackLimit = 256;

typedef pair<Array<void*, StackLimit>, size_t> StackCapture;

inline StackCapture captureStack() {
  StackCapture stackCapture;
  stackCapture.second = backtrace(stackCapture.first.ptr(), StackLimit);
  return stackCapture;
}

OutputProxy outputStack(StackCapture stack) {
  return OutputProxy([stack = std::move(stack)](std::ostream & os) {
      char** symbols = backtrace_symbols(stack.first.ptr(), stack.second);
      for (size_t i = 0; i < stack.second; ++i) {
        os << symbols[i];
        if (i + 1 < stack.second)
          os << std::endl;
      }

      if (stack.second == StackLimit)
        os << std::endl << "[Stack Output Limit Reached]";

      ::free(symbols);
    });
}

StarException::StarException() noexcept
  : StarException(std::string("StarException")) {}

StarException::~StarException() noexcept {}

StarException::StarException(std::string message, bool genStackTrace) noexcept
  : StarException("StarException", std::move(message), genStackTrace) {}

StarException::StarException(std::exception const& cause) noexcept
  : StarException("StarException", std::string(), cause) {}

StarException::StarException(std::string message, std::exception const& cause) noexcept
  : StarException("StarException", std::move(message), cause) {}

const char* StarException::what() const throw() {
  if (m_whatBuffer.empty()) {
    std::ostringstream os;
    m_printException(os, false);
    m_whatBuffer = os.str();
  }
  return m_whatBuffer.c_str();
}

StarException::StarException(char const* type, std::string message, bool genStackTrace) noexcept {
  auto printException = [](std::ostream& os, bool fullStacktrace, char const* type, std::string message, std::string stack) {
    os << "(" << type << ")";
    if (!message.empty())
      os << " " << message;

    if (fullStacktrace && !stack.empty()) {
      os << std::endl;
      os << stack;
    }
  };

  m_printException = bind(printException, _1, _2, type, std::move(message), genStackTrace ? captureBacktrace() : std::string());
}

StarException::StarException(char const* type, std::string message, std::exception const& cause) noexcept
  : StarException(type, std::move(message)) {
  auto printException = [](std::ostream& os, bool fullStacktrace, function<void(std::ostream&, bool)> self, function<void(std::ostream&, bool)> cause) {
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

  m_printException = bind(printException, _1, _2, m_printException, std::move(printCause));
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
    return OutputProxy(bind([](std::ostream& os, std::string what) { os << "std::exception: " << what; }, _1, std::string(e.what())));
}

void printStack(char const* message) {
  Logger::info("Stack Trace ({})...\n{}", message, captureBacktrace());
}

void fatalError(char const* message, bool showStackTrace) {
  if (showStackTrace)
    Logger::error("Fatal Error: {}\n{}", message, captureBacktrace());
  else
    Logger::error("Fatal Error: {}", message);

  std::abort();
}

void fatalException(std::exception const& e, bool showStackTrace) {
  if (showStackTrace)
    Logger::error("Fatal Exception caught: {}\nCaught at:\n{}", outputException(e, true), captureBacktrace());
  else
    Logger::error("Fatal Exception caught: {}", outputException(e, showStackTrace));

  std::abort();
}

}
