#ifndef STAR_LEXICAL_CAST_HPP
#define STAR_LEXICAL_CAST_HPP

#include "StarString.hpp"
#include "StarMaybe.hpp"

#include <sstream>
#include <locale>

namespace Star {

STAR_EXCEPTION(BadLexicalCast, StarException);

// Very simple basic lexical cast using stream input.  Always operates in the
// "C" locale.
template <typename Type>
Maybe<Type> maybeLexicalCast(std::string const& s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  Type result;
  std::istringstream stream(s);
  stream.flags(flags);
  stream.imbue(std::locale::classic());

  if (!(stream >> result))
    return {};

  // Confirm that we read everything out of the stream
  char ch;
  if (stream >> ch)
    return {};

  return result;
}

template <typename Type>
Maybe<Type> maybeLexicalCast(char const* s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  return maybeLexicalCast<Type>(std::string(s), flags);
}

template <typename Type>
Maybe<Type> maybeLexicalCast(String const& s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  return maybeLexicalCast<Type>(s.utf8(), flags);
}

template <typename Type>
Type lexicalCast(std::string const& s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  auto m = maybeLexicalCast<Type>(s, flags);
  if (m)
    return m.take();
  else
    throw BadLexicalCast();
}

template <typename Type>
Type lexicalCast(char const* s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  return lexicalCast<Type>(std::string(s), flags);
}

template <typename Type>
Type lexicalCast(String const& s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  return lexicalCast<Type>(s.utf8(), flags);
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
