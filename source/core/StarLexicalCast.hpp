#ifndef STAR_LEXICAL_CAST_HPP
#define STAR_LEXICAL_CAST_HPP

#include "StarString.hpp"
#include "StarStringView.hpp"
#include "StarMaybe.hpp"

#include <sstream>
#include <locale>

namespace Star {

STAR_EXCEPTION(BadLexicalCast, StarException);

// Very simple basic lexical cast using stream input.  Always operates in the
// "C" locale.
template <typename Type>
Maybe<Type> maybeLexicalCast(StringView s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  Type result;
  std::istringstream stream(std::string(s.utf8()));
  stream.flags(flags);
  stream.imbue(std::locale::classic());

  if (!(stream >> result))
    return {};

  // Confirm that we read everything out of the stream
  char ch;
  if (stream >> ch)
    return {};

  return move(result);
}

template <typename Type>
Type lexicalCast(StringView s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  auto m = maybeLexicalCast<Type>(s, flags);
  if (m)
    return m.take();
  else
    throw BadLexicalCast(strf("Lexical cast failed on '%s'", s));
}

template <class Type>
std::string toString(Type const& t, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  std::stringstream ss;
  ss.flags(flags);
  ss.imbue(std::locale::classic());
  ss << t;
  return ss.str();
}

}

#endif
