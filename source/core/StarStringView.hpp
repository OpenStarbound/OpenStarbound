#ifndef STAR_STRING_VIEW_HPP
#define STAR_STRING_VIEW_HPP

#include "StarString.hpp"

namespace Star {

STAR_CLASS(StringView);
STAR_CLASS(String);

// This is a StringView version of Star::String
// I literally just copy-pasted it all from there
class StringView {
public:
  typedef String::Char Char;

  typedef U8ToU32Iterator<std::string_view::const_iterator> const_iterator;
  typedef Char value_type;
  typedef value_type const& const_reference;

  using CaseSensitivity = String::CaseSensitivity;

  StringView();
  StringView(StringView const& s);
  StringView(StringView&& s) noexcept;
  StringView(String const& s);

  // These assume utf8 input
  StringView(char const* s);
  StringView(char const* s, size_t n);
  StringView(std::string_view const& s);
  StringView(std::string_view&& s) noexcept;
  StringView(std::string const& s);

  StringView(Char const* s);
  StringView(Char const* s, size_t n);

  // const& to internal utf8 data
  std::string_view const& utf8() const;
  std::string_view takeUtf8();
  ByteArray utf8Bytes() const;
  // Pointer to internal utf8 data, null-terminated.
  char const* utf8Ptr() const;
  size_t utf8Size() const;

  const_iterator begin() const;
  const_iterator end() const;

  size_t size() const;
  size_t length() const;

  bool empty() const;

  Char operator[](size_t index) const;
  // Throws StringException if i out of range.
  Char at(size_t i) const;

  bool endsWith(StringView end, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  bool endsWith(Char end, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  bool beginsWith(StringView beg, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  bool beginsWith(Char beg, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;

  typedef function<void(StringView, size_t, size_t)> SplitCallback;
  void forEachSplitAnyView(StringView pattern, SplitCallback) const;
  void forEachSplitView(StringView pattern, SplitCallback) const;

  bool hasChar(Char c) const;
  // Identical to hasChar, except, if string is empty, tests if c is
  // whitespace.
  bool hasCharOrWhitespace(Char c) const;

  size_t find(Char c, size_t beg = 0, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  size_t find(StringView s, size_t beg = 0, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  size_t findLast(Char c, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  size_t findLast(StringView s, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;

  // If pattern is empty, finds first whitespace
  size_t findFirstOf(StringView chars = "", size_t beg = 0) const;

  // If pattern is empty, finds first non-whitespace
  size_t findFirstNotOf(StringView chars = "", size_t beg = 0) const;

  // finds the the start of the next 'boundary' in a string.  used for quickly
  // scanning a string
  size_t findNextBoundary(size_t index, bool backwards = false) const;

  bool contains(StringView s, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;

  int compare(StringView s, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  bool equals(StringView s, CaseSensitivity cs = CaseSensitivity::CaseSensitive) const;
  // Synonym for equals(s, String::CaseInsensitive)
  bool equalsIgnoreCase(StringView s) const;

  StringView substr(size_t position, size_t n = NPos) const;

  StringView& operator=(StringView s);

  friend bool operator==(StringView s1, StringView s2);
  friend bool operator!=(StringView s1, StringView s2);
  friend bool operator<(StringView s1, StringView s2);

  friend std::ostream& operator<<(std::ostream& os, StringView& s);

private:
  int compare(size_t selfOffset,
      size_t selfLen,
      StringView other,
      size_t otherOffset,
      size_t otherLen,
      CaseSensitivity cs) const;

  std::string_view m_view;
};

}

#endif