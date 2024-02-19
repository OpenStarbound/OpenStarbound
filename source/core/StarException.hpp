#ifndef STAR_EXCEPTION_HPP
#define STAR_EXCEPTION_HPP

#include "StarMemory.hpp"
#include "StarOutputProxy.hpp"

#include <string>
#include <sstream>

namespace Star {

template <typename... T>
std::string strf(fmt::format_string<T...> fmt, T&&... args);

class StarException : public std::exception {
public:
  template <typename... Args>
  static StarException format(fmt::format_string<Args...> fmt, Args const&... args);

  StarException() noexcept;
  virtual ~StarException() noexcept;

  explicit StarException(std::string message, bool genStackTrace = true) noexcept;
  explicit StarException(std::exception const& cause) noexcept;
  StarException(std::string message, std::exception const& cause) noexcept;

  virtual char const* what() const noexcept override;

  // If the given exception is really StarException, then this will call
  // StarException::printException, otherwise just prints std::exception::what.
  friend void printException(std::ostream& os, std::exception const& e, bool fullStacktrace);
  friend std::string printException(std::exception const& e, bool fullStacktrace);
  friend OutputProxy outputException(std::exception const& e, bool fullStacktrace);

protected:
  StarException(char const* type, std::string message, bool genStackTrace = true) noexcept;
  StarException(char const* type, std::string message, std::exception const& cause) noexcept;

private:
  // Takes the ostream to print to, whether to print the full stacktrace.  Must
  // not bind 'this', may outlive the exception in the case of chained
  // exception causes.
  function<void(std::ostream&, bool)> m_printException;

  // m_printException will be called without the stack-trace to print
  // m_whatBuffer, if the what() method is invoked.
  mutable std::string m_whatBuffer;
};

void printException(std::ostream& os, std::exception const& e, bool fullStacktrace);
std::string printException(std::exception const& e, bool fullStacktrace);
OutputProxy outputException(std::exception const& e, bool fullStacktrace);

void printStack(char const* message);

// Log error and stack-trace and possibly show a dialog box if available, then
// abort.
void fatalError(char const* message, bool showStackTrace);
void fatalException(std::exception const& e, bool showStackTrace);

#ifdef STAR_DEBUG
#define debugPrintStack() \
  { Star::printStack("Debug: file " STAR_STR(__FILE__) " line " STAR_STR(__LINE__)); }
#define starAssert(COND)                                                                                \
  {                                                                                                     \
    if (COND)                                                                                           \
      ;                                                                                                 \
    else                                                                                                \
      Star::fatalError("assert failure in file " STAR_STR(__FILE__) " line " STAR_STR(__LINE__), true); \
  }
#else
#define debugPrintStack() \
  {}
#define starAssert(COND) \
  {}
#endif

#define STAR_EXCEPTION(ClassName, BaseName)                                                                                       \
  class ClassName : public BaseName {                                                                                             \
  public:                                                                                                                         \
    template <typename... Args>                                                                                                   \
    static ClassName format(fmt::format_string<Args...> fmt, Args const&... args) {                                               \
      return ClassName(strf(fmt, args...));                                                                                       \
    }                                                                                                                             \
    ClassName() : BaseName(#ClassName, std::string()) {}                                                                          \
    explicit ClassName(std::string message, bool genStackTrace = true) : BaseName(#ClassName, std::move(message), genStackTrace) {} \
    explicit ClassName(std::exception const& cause) : BaseName(#ClassName, std::string(), cause) {}                               \
    ClassName(std::string message, std::exception const& cause) : BaseName(#ClassName, std::move(message), cause) {}              \
                                                                                                                                  \
  protected:                                                                                                                      \
    ClassName(char const* type, std::string message, bool genStackTrace = true) : BaseName(type, std::move(message), genStackTrace) {} \
    ClassName(char const* type, std::string message, std::exception const& cause)                                                 \
      : BaseName(type, std::move(message), cause) {}                                                                              \
  }

STAR_EXCEPTION(OutOfRangeException, StarException);
STAR_EXCEPTION(IOException, StarException);
STAR_EXCEPTION(MemoryException, StarException);

template <typename... Args>
StarException StarException::format(fmt::format_string<Args...> fmt, Args const&... args) {
  return StarException(strf(fmt, args...));
}

}

#endif
