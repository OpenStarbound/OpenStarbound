#include "StarException.hpp"
#include "StarCasting.hpp"
#include "StarLogging.hpp"

#include <execinfo.h>
#include <cstdlib>
#ifdef STAR_USE_CPPTRACE
#include "cpptrace/cpptrace.hpp"
#include "cpptrace/formatting.hpp"
#endif

namespace Star {

#ifdef STAR_USE_CPPTRACE

static inline std::string captureBacktrace() {
  auto formatter = cpptrace::formatter{}
    .paths(cpptrace::formatter::path_mode::basename);
  std::ostringstream out;
  formatter.print(out, cpptrace::generate_trace());
  return out.str();
}
  
#endif

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
#ifdef STAR_USE_CPPTRACE
  auto printException = [](std::ostream& os, bool fullStacktrace, char const* type, std::string message, std::string stack) {
#else
  auto printException = [](std::ostream& os, bool fullStacktrace, char const* type, std::string message, Maybe<StackCapture> stack) {
#endif
    os << "(" << type << ")";
    if (!message.empty())
      os << " " << message;

#ifdef STAR_USE_CPPTRACE
    if (fullStacktrace && !stack.empty()) {
      os << std::endl;
      os << stack;
    }
#else
    if (fullStacktrace && stack) {
      os << std::endl;
      os << outputStack(*stack);
    }
#endif
  };
#ifdef STAR_USE_CPPTRACE
  m_printException = bind(printException, _1, _2, type, std::move(message), genStackTrace ? captureBacktrace() : std::string());
#else
  m_printException = bind(printException, _1, _2, type, std::move(message), genStackTrace ? captureStack() : Maybe<StackCapture>());
#endif
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
#ifdef STAR_USE_CPPTRACE
  Logger::info("Stack Trace ({})...\n{}", message, captureBacktrace());
#else
  Logger::info("Stack Trace ({})...\n{}", message, outputStack(captureStack()));
#endif
}

void fatalError(char const* message, bool showStackTrace) {
  if (showStackTrace)
#ifdef STAR_USE_CPPTRACE
    Logger::error("Fatal Error: {}\n{}", message, captureBacktrace());
#else
    Logger::error("Fatal Error: {}\n{}", message, outputStack(captureStack()));
#endif
  else
    Logger::error("Fatal Error: {}", message);

  std::abort();
}

void fatalException(std::exception const& e, bool showStackTrace) {
  if (showStackTrace)
#ifdef STAR_USE_CPPTRACE
    Logger::error("Fatal Exception caught: {}\nCaught at:\n{}", outputException(e, true), captureBacktrace());
#else
    Logger::error("Fatal Exception caught: {}\nCaught at:\n{}", outputException(e, true), outputStack(captureStack()));
#endif
  else
    Logger::error("Fatal Exception caught: {}", outputException(e, showStackTrace));

  std::abort();
}

}
