#pragma once

#include "StarString.hpp"
#include "StarEncode.hpp"
#include "StarBytes.hpp"
#include "StarFormat.hpp"

namespace Star {

// Currently the specification of the "language" is incredibly simple The only
// thing we process are quoted strings and backslashes Backslashes function as
// a useful subset of C++ This means: Newline: \n Tab: \t Backslash: \\ Single
// Quote: \' Double Quote: \" Null: \0 Space: "\ " (without quotes ofc, not
// actually C++) Also \v \b \a \f \r Plus Unicode \uxxxx Not implemented octal
// and hexadecimal, because it's possible to construct invalid unicode code
// points using them

STAR_EXCEPTION(ShellParsingException, StarException);

class ShellParser {
public:
  ShellParser();
  typedef String::Char Char;

  enum class TokenType {
    Word,
    // TODO: braces, brackets, actual shell stuff

  };

  struct Token {
    TokenType type;
    String token;
  };

  List<Token> tokenize(String const& command);
  StringList tokenizeToStringList(String const& command);

private:
  void init(String const& command);

  String word();
  Char parseBackslash();
  Char parseUnicodeEscapeSequence(Maybe<Char> previousCodepoint = {});

  bool isSpace(Char letter) const;
  bool isQuote(Char letter) const;

  bool inQuotedString() const;
  bool notDone() const;

  Maybe<Char> current() const;
  Maybe<Char> next();
  Maybe<Char> previous();

  String::const_iterator m_begin;
  String::const_iterator m_current;
  String::const_iterator m_end;

  Char m_quotedType;
};

}
