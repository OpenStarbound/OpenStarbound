#include "StarLexicalCast.hpp"

namespace Star {

void throwLexicalCastError(std::errc ec, const char* first, const char* last) {
  StringView str(first, last - first);
  if (ec == std::errc::invalid_argument)
    throw BadLexicalCast(strf("Lexical cast failed on '{}' (invalid argument)", str));
  else
    throw BadLexicalCast(strf("Lexical cast failed on '{}'", str));
}

template <>
bool tryLexicalCast(bool& result, const char* first, const char* last) {
  size_t len = last - first;
  if (strncmp(first, "true", len) == 0)
    result = true;
  else if (strncmp(first, "false", len) != 0)
    return false;

  result = false;
  return true;
}

template <>
bool lexicalCast(const char* first, const char* last) {
  size_t len = last - first;
  if (strncmp(first, "true", len) == 0)
    return true;
  else if (strncmp(first, "false", len) != 0)
    throwLexicalCastError(std::errc(), first, last);

  return false;
}

}