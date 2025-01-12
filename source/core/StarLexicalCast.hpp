#pragma once

#include "StarFormat.hpp"
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

  return result;
}

template <typename Type>
Type lexicalCast(StringView s, std::ios_base::fmtflags flags = std::ios_base::boolalpha) {
  auto m = maybeLexicalCast<Type>(s, flags);
  if (m)
    return m.take();
  else
    throw BadLexicalCast(strf("Lexical cast failed on '{}'", s));
}

}