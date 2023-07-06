#ifndef STAR_FORMAT_HPP
#define STAR_FORMAT_HPP

#include "StarMemory.hpp"
#include "StarException.hpp"

#include "fmt/core.h"
#include "fmt/ostream.h"
#include "fmt/format.h"
#include "fmt/ranges.h"

namespace Star {

STAR_EXCEPTION(FormatException, StarException);

template <typename... T>
std::string strf(fmt::format_string<T...> fmt, T&&... args) {
  try { return fmt::format(fmt, args...); }
  catch (std::exception const& e) {
    throw FormatException(strf("Exception thrown during string format: {}", e.what()));
  }
}

template <typename... T>
void format(std::ostream& out, fmt::format_string<T...> fmt, T&&... args) {
  out << strf(fmt, args...);
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

template <class Type>
inline std::string toString(Type const& t) {
  return fmt::to_string(t);
}

}

#endif
