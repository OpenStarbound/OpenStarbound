#ifndef STAR_FORMAT_HPP
#define STAR_FORMAT_HPP

#include "StarMemory.hpp"

#include "fmt/core.h"
#include "fmt/ostream.h"
#include "fmt/format.h"
#include "fmt/ranges.h"

namespace Star {

struct FormatException : public std::exception {
  FormatException(std::string what) : whatmsg(move(what)) {}

  char const* what() const noexcept override {
    return whatmsg.c_str();
  }

  std::string whatmsg;
};

}

namespace Star {

template <typename... T>
void format(std::ostream& out, fmt::format_string<T...> fmt, T&&... args) {
  out << fmt::format(fmt, args...);
}

// Automatically flushes, use format to avoid flushing.
template <typename... Args>
void coutf(char const* fmt, Args const&... args) {
  format(std::cout, fmt, args...);
  std::cout.flush();
}

// Automatically flushes, use format to avoid flushing.
template <typename... Args>
void cerrf(char const* fmt, Args const&... args) {
  format(std::cerr, fmt, args...);
  std::cerr.flush();
}

template <typename... T>
std::string strf(fmt::format_string<T...> fmt, T&&... args) {
  return fmt::format(fmt, args...);
}

namespace OutputAnyDetail {
  template<typename T, typename CharT, typename Traits>
  std::basic_string<CharT, Traits> string(T const& t) {
    return fmt::format("<type {} at address: {}>", typeid(T).name(), (void*)&t);
  }

  template<typename T, typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& output(std::basic_ostream<CharT, Traits>& os, T const& t) {
    return os << string<T, CharT, Traits>(t);
  }

  namespace Operator {
    template<typename T, typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, T const& t) {
      return output(os, t);
    }
  }

  template <typename T>
  struct Wrapper {
    T const& wrapped;
  };

  template <typename T>
  std::ostream& operator<<(std::ostream& os, Wrapper<T> const& wrapper) {
    using namespace Operator;
    return os << wrapper.wrapped;
  }
}

// Wraps a type so that is printable no matter what..  If no operator<< is
// defined for a type, then will print <type [typeid] at address: [address]>
template <typename T>
OutputAnyDetail::Wrapper<T> outputAny(T const& t) {
  return OutputAnyDetail::Wrapper<T>{t};
}

struct OutputProxy {
  typedef function<void(std::ostream&)> PrintFunction;

  OutputProxy(PrintFunction p)
    : print(move(p)) {}

  PrintFunction print;
};

inline std::ostream& operator<<(std::ostream& os, OutputProxy const& p) {
  p.print(os);
  return os;
}

}

template <typename T>
struct fmt::formatter<Star::OutputAnyDetail::Wrapper<T>> : ostream_formatter {};
template <> struct fmt::formatter<Star::OutputProxy> : ostream_formatter {};

#endif
