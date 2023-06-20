#include "StarShellParser.hpp"

#include "gtest/gtest.h"

TEST(ShellParser, Simple) {
  Star::ShellParser parser;
  auto tokenSet1 = parser.tokenize("The first test.");

  EXPECT_EQ(tokenSet1.size(), 3u);
  EXPECT_EQ(tokenSet1.at(0).token, "The");
  EXPECT_EQ(tokenSet1.at(1).token, "first");
  EXPECT_EQ(tokenSet1.at(2).token, "test.");
}

TEST(ShellParser, DirectUnicode) {
  Star::ShellParser parser;
  auto tokenSet2 = parser.tokenize("Unicode Symbols: ❤ ☀ ☆ ☂ ☻ ♞ ☯ ☭ ☢ € → ☎ ❄ ♫ ✂ ▷ ✇ ♎ ⇧ ☮ ♻ ⌘ ⌛ ☘");

  EXPECT_EQ(tokenSet2.size(), 26u);
  EXPECT_EQ(tokenSet2.at(0).token, "Unicode");
  EXPECT_EQ(tokenSet2.at(1).token, "Symbols:");
  EXPECT_EQ(tokenSet2.at(10).token, "☢");
}

TEST(ShellParser, SimpleQuotes) {
  Star::ShellParser parser;
  auto tokenSet3 = parser.tokenize("\"This is a test\" 'This is another test'");

  EXPECT_EQ(tokenSet3.size(), 2u);
  EXPECT_EQ(tokenSet3.at(0).token, "This is a test");
  EXPECT_EQ(tokenSet3.at(1).token, "This is another test");
}

TEST(ShellParser, ComplexQuotes) {
  Star::ShellParser parser;
  auto tokenSet4 = parser.tokenize("\"'asdf' 'asdf asdf'\" '\"omg\" omg omg'");

  EXPECT_EQ(tokenSet4.size(), 2u);
  EXPECT_EQ(tokenSet4.at(0).token, "'asdf' 'asdf asdf'");
  EXPECT_EQ(tokenSet4.at(1).token, "\"omg\" omg omg");
}

TEST(ShellParser, SpacelessQuotes) {
  Star::ShellParser parser;
  auto tokenSet5 = parser.tokenize("\"asdf\"asdf asdf");

  EXPECT_EQ(tokenSet5.size(), 2u);
  EXPECT_EQ(tokenSet5.at(0).token, "asdfasdf");
  EXPECT_EQ(tokenSet5.at(1).token, "asdf");

  auto tokenSet6 = parser.tokenize("'asdf'asdf asdf");

  EXPECT_EQ(tokenSet6.size(), 2u);
  EXPECT_EQ(tokenSet6.at(0).token, "asdfasdf");
  EXPECT_EQ(tokenSet6.at(1).token, "asdf");
}

TEST(ShellParser, EscapedSpaces) {
  Star::ShellParser parser;
  auto tokenSet7 = parser.tokenize("This\\ is\\ a test");

  EXPECT_EQ(tokenSet7.size(), 2u);
  EXPECT_EQ(tokenSet7.at(0).token, "This is a");
  EXPECT_EQ(tokenSet7.at(1).token, "test");
}

TEST(ShellParser, EscapedUnicode) {
  Star::ShellParser parser;
  auto tokenSet8 = parser.tokenize("This is a unicode codepoint: \\u2603");

  EXPECT_EQ(tokenSet8.size(), 6u);
  EXPECT_EQ(tokenSet8.at(0).token, "This");
  EXPECT_EQ(tokenSet8.at(5).token, "☃");

  auto tokenSet9 = parser.tokenize("\\u");

  EXPECT_EQ(tokenSet9.size(), 1u);
  EXPECT_EQ(tokenSet9.at(0).token, "u");

  auto tokenSet10 = parser.tokenize("\\u2603\\u2603\\u2603 \\u2603");

  EXPECT_EQ(tokenSet10.size(), 2u);
  EXPECT_EQ(tokenSet10.at(0).token, "☃☃☃");
  EXPECT_EQ(tokenSet10.at(1).token, "☃");
}
