#include "StarLexicalCast.hpp"

namespace Star {

void throwLexicalCastError(std::errc ec, const char* first, const char* last) {
  StringView str(first, last - first);
  if (ec == std::errc::invalid_argument)
    throw BadLexicalCast(strf("Lexical cast failed on '{}' (invalid argument)", str));
  else
    throw BadLexicalCast(strf("Lexical cast failed on '{}'", str));
}

}