#pragma once

#include "StarString.hpp"
#include "StarStringView.hpp"
#include "StarVector.hpp"
#include "StarDirectives.hpp"
#include "StarJson.hpp"

namespace Star {

unsigned const DefaultFontSize = 8;
float const DefaultLineSpacing = 1.3f;

struct TextStyle {
  float lineSpacing = DefaultLineSpacing;
  Vec4B color = Vec4B::filled(255);
  Vec4B shadow = Vec4B::filled(0);
  unsigned fontSize = DefaultFontSize;
  String font = "";
  Directives directives;
  Directives backDirectives;

  TextStyle() = default;
  TextStyle(Json const& config);
  TextStyle& loadJson(Json const& config);
};

namespace Text {
  unsigned char const StartEsc = '\x1b';
  unsigned char const EndEsc = ';';
  unsigned char const CmdEsc = '^';
  unsigned char const SpecialCharLimit = ' ';
  extern std::string const AllEsc;
  extern std::string const AllEscEnd;

  String stripEscapeCodes(String const& s);
  inline bool isEscapeCode(Utf32Type c) { return c == CmdEsc || c == StartEsc; }

  typedef function<bool(StringView text)> TextCallback;
  typedef function<bool(StringView commands)> CommandsCallback;
  bool processText(StringView text, TextCallback textFunc, CommandsCallback commandsFunc = CommandsCallback(), bool includeCommandSides = false);
  String preprocessEscapeCodes(String const& s);
  String extractCodes(String const& s);
}

}
