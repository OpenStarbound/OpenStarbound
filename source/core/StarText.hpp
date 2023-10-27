#ifndef STAR_TEXT_HPP
#define STAR_TEXT_HPP
#include "StarString.hpp"
#include "StarStringView.hpp"

namespace Star {
  
namespace Text {
  unsigned char const StartEsc = '\x1b';
  unsigned char const EndEsc = ';';
  unsigned char const CmdEsc = '^';
  unsigned char const SpecialCharLimit = ' ';

  String stripEscapeCodes(String const& s);
  inline bool isEscapeCode(Utf32Type c) { return c == CmdEsc || c == StartEsc; }

  typedef function<bool(StringView text)> TextCallback;
  typedef function<bool(StringView commands)> CommandsCallback;
  bool processText(StringView text, TextCallback textFunc, CommandsCallback commandsFunc = CommandsCallback(), bool includeCommandSides = false);
  String preprocessEscapeCodes(String const& s);
  String extractCodes(String const& s);
}

}

#endif