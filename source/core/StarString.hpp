#ifndef STAR_STRING_HPP
#define STAR_STRING_HPP

#include "StarUnicode.hpp"
#include "StarHash.hpp"
#include "StarByteArray.hpp"
#include "StarList.hpp"
#include "StarMap.hpp"
#include "StarSet.hpp"
#include "StarFormat.hpp"

namespace Star {

STAR_CLASS(StringList);
STAR_CLASS(String);
STAR_CLASS(StringView);

STAR_EXCEPTION(StringException, StarException);

// A Unicode string class, which is a basic UTF-8 aware wrapper around
// std::string.  Provides methods for accessing UTF-32 "Char" type, which
// provides access to each individual code point.  Printing, hashing, copying,
// and in-order access should be basically as fast as std::string, but the more
// complex string processing methods may be much worse.
//
// All case sensitive / insensitive functionality is based on ASCII tolower and
// toupper, and will have no effect on characters outside ASCII.  Therefore,
// case insensitivity is really only appropriate for code / script processing,
// not for general strings.
class String {
public:
  typedef Utf32Type Char;

  // std::basic_string equivalent that guarantees const access time for
  // operator[], etc
  typedef std::basic_string<Char> WideString;

  typedef U8ToU32Iterator<std::string::const_iterator> const_iterator;
  typedef Char value_type;
  typedef value_type const& const_reference;

  enum CaseSensitivity {
    CaseSensitive,
    CaseInsensitive
  };

  // Space, horizontal tab, newline, carriage return, and BOM / ZWNBSP
  static bool isSpace(Char c);
  static bool isAsciiNumber(Char c);
  static bool isAsciiLetter(Char c);

  // These methods only actually work on unicode characters below 127, i.e.
  // ASCII subset.
  static Char toLower(Char c);
  static Char toUpper(Char c);
  static bool charEqual(Char c1, Char c2, CaseSensitivity cs);

  // Join two strings together with a joiner, so that only one instance of the
  // joiner is in between the left and right strings.  For example, joins "foo"
  // and "bar" with "?" to produce "foo?bar".  Gets rid of repeat joiners, so
  // "foo?" and "?bar" with "?" also becomes "foo?bar".  Also, if left or right
  // is empty, does not add a joiner, for example "" and "baz" joined with "?"
  // produces "baz".
  static String joinWith(String const& join, String const& left, String const& right);
  template <typename... StringType>
  static String joinWith(String const& join, String const& first, String const& second, String const& third, StringType const&... rest);

  String();
  String(String const& s);
  String(String&& s);

  // These assume utf8 input
  String(char const* s);
  String(char const* s, size_t n);
  String(std::string const& s);
  String(std::string&& s);

  String(std::wstring const& s);
  String(Char const* s);
  String(Char const* s, size_t n);
  String(Char c, size_t n);

  explicit String(Char c);

  // const& to internal utf8 data
  std::string const& utf8() const;
  std::string takeUtf8();
  ByteArray utf8Bytes() const;
  // Pointer to internal utf8 data, null-terminated.
  char const* utf8Ptr() const;
  size_t utf8Size() const;

  std::wstring wstring() const;
  WideString wideString() const;

  const_iterator begin() const;
  const_iterator end() const;

  size_t size() const;
  size_t length() const;

  void clear();
  void reserve(size_t n);
  bool empty() const;

  Char operator[](size_t i) const;
  // Throws StringException if i out of range.
  Char at(size_t i) const;

  String toUpper() const;
  String toLower() const;
  String titleCase() const;

  bool endsWith(String const& end, CaseSensitivity cs = CaseSensitive) const;
  bool endsWith(Char end, CaseSensitivity cs = CaseSensitive) const;
  bool beginsWith(String const& beg, CaseSensitivity cs = CaseSensitive) const;
  bool beginsWith(Char beg, CaseSensitivity cs = CaseSensitive) const;

  String reverse() const;

  String rot13() const;

  StringList split(Char c, size_t maxSplit = NPos) const;
  StringList split(String const& pattern, size_t maxSplit = NPos) const;
  StringList rsplit(Char c, size_t maxSplit = NPos) const;
  StringList rsplit(String const& pattern, size_t maxSplit = NPos) const;

  // Splits on any number of contiguous instances of any of the given
  // characters.  Behaves differently than regular split in that leading and
  // trailing instances of the characters are also ignored, and in general no
  // empty strings will be in the resulting split list.  If chars is empty,
  // then splits on any whitespace.
  StringList splitAny(String const& chars = "", size_t maxSplit = NPos) const;
  StringList rsplitAny(String const& chars = "", size_t maxSplit = NPos) const;

  // Split any with '\n\r'
  StringList splitLines(size_t maxSplit = NPos) const;
  // Shorthand for splitAny("");
  StringList splitWhitespace(size_t maxSplit = NPos) const;

  // Splits a string once based on the given characters (defaulting to
  // whitespace), and returns the first part.  This string is set to the
  // second part.
  String extract(String const& chars = "");
  String rextract(String const& chars = "");

  bool hasChar(Char c) const;
  // Identical to hasChar, except, if string is empty, tests if c is
  // whitespace.
  bool hasCharOrWhitespace(Char c) const;

  String replace(String const& rplc, String const& val) const;

  String trimEnd(String const& chars = "") const;
  String trimBeg(String const& chars = "") const;
  String trim(String const& chars = "") const;

  size_t find(Char c, size_t beg = 0, CaseSensitivity cs = CaseSensitive) const;
  size_t find(String const& s, size_t beg = 0, CaseSensitivity cs = CaseSensitive) const;
  size_t findLast(Char c, CaseSensitivity cs = CaseSensitive) const;
  size_t findLast(String const& s, CaseSensitivity cs = CaseSensitive) const;

  // If pattern is empty, finds first whitespace
  size_t findFirstOf(String const& chars = "", size_t beg = 0) const;

  // If pattern is empty, finds first non-whitespace
  size_t findFirstNotOf(String const& chars = "", size_t beg = 0) const;

  // finds the the start of the next 'boundary' in a string.  used for quickly
  // scanning a string
  size_t findNextBoundary(size_t index, bool backwards = false) const;

  String slice(SliceIndex a = SliceIndex(), SliceIndex b = SliceIndex(), int i = 1) const;

  void append(String const& s);
  void append(std::string const& s);
  void append(Char const* s);
  void append(Char const* s, size_t n);
  void append(char const* s);
  void append(char const* s, size_t n);
  void append(Char c);

  void prepend(String const& s);
  void prepend(std::string const& s);
  void prepend(Char const* s);
  void prepend(Char const* s, size_t n);
  void prepend(char const* s);
  void prepend(char const* s, size_t n);
  void prepend(Char c);

  void push_back(Char c);
  void push_front(Char c);

  bool contains(String const& s, CaseSensitivity cs = CaseSensitive) const;

  // Does this string match the given regular expression?
  bool regexMatch(String const& regex, bool full = true, bool caseSensitive = true) const;

  int compare(String const& s, CaseSensitivity cs = CaseSensitive) const;
  bool equals(String const& s, CaseSensitivity cs = CaseSensitive) const;
  // Synonym for equals(s, String::CaseInsensitive)
  bool equalsIgnoreCase(String const& s) const;

  String substr(size_t position, size_t n = NPos) const;
  void erase(size_t pos = 0, size_t n = NPos);

  String padLeft(size_t size, String const& filler) const;
  String padRight(size_t size, String const& filler) const;

  // Replace angle bracket tags in the string with values given by the given
  // lookup function.  Will be called as:
  // String lookup(String const& key);
  template <typename Lookup>
  String lookupTags(Lookup&& lookup) const;

  // StringView variant
  template <typename Lookup>
  Maybe<String> maybeLookupTagsView(Lookup&& lookup) const;

  template <typename Lookup>
  String lookupTagsView(Lookup&& lookup) const;

  // Replace angle bracket tags in the string with values given by the tags
  // map.  If replaceWithDefault is true, then values that are not found in the
  // tags map are replace with the default string.  If replaceWithDefault is
  // false, tags that are not found are not replaced at all.
  template <typename MapType>
  String replaceTags(MapType const& tags, bool replaceWithDefault = false, String defaultValue = "") const;

  String& operator=(String const& s);
  String& operator=(String&& s);

  String& operator+=(String const& s);
  String& operator+=(std::string const& s);
  String& operator+=(Char const* s);
  String& operator+=(char const* s);
  String& operator+=(Char c);

  friend bool operator==(String const& s1, String const& s2);
  friend bool operator==(String const& s1, std::string const& s2);
  friend bool operator==(String const& s1, Char const* s2);
  friend bool operator==(String const& s1, char const* s2);
  friend bool operator==(std::string const& s1, String const& s2);
  friend bool operator==(Char const* s1, String const& s2);
  friend bool operator==(char const* s1, String const& s2);

  friend bool operator!=(String const& s1, String const& s2);
  friend bool operator!=(String const& s1, std::string const& s2);
  friend bool operator!=(String const& s1, Char const* s2);
  friend bool operator!=(String const& s1, char const* c);
  friend bool operator!=(std::string const& s1, String const& s2);
  friend bool operator!=(Char const* s1, String const& s2);
  friend bool operator!=(char const* s1, String const& s2);

  friend bool operator<(String const& s1, String const& s2);
  friend bool operator<(String const& s1, std::string const& s2);
  friend bool operator<(String const& s1, Char const* s2);
  friend bool operator<(String const& s1, char const* s2);
  friend bool operator<(std::string const& s1, String const& s2);
  friend bool operator<(Char const* s1, String const& s2);
  friend bool operator<(char const* s1, String const& s2);

  friend String operator+(String s1, String const& s2);
  friend String operator+(String s1, std::string const& s2);
  friend String operator+(String s1, Char const* s2);
  friend String operator+(String s1, char const* s2);
  friend String operator+(std::string const& s1, String const& s2);
  friend String operator+(Char const* s1, String const& s2);
  friend String operator+(char const* s1, String const& s2);

  friend String operator+(String s, Char c);
  friend String operator+(Char c, String const& s);

  friend String operator*(String const& s, unsigned times);
  friend String operator*(unsigned times, String const& s);

  friend std::ostream& operator<<(std::ostream& os, String const& s);
  friend std::istream& operator>>(std::istream& is, String& s);

  // String view functions
  String(StringView s);
  String(std::string_view s);

  String& operator+=(StringView s);
  String& operator+=(std::string_view s);

private:
  int compare(size_t selfOffset,
      size_t selfLen,
      String const& other,
      size_t otherOffset,
      size_t otherLen,
      CaseSensitivity cs) const;

  std::string m_string;
};

class StringList : public List<String> {
public:
  typedef List<String> Base;

  typedef Base::iterator iterator;
  typedef Base::const_iterator const_iterator;
  typedef Base::value_type value_type;
  typedef Base::reference reference;
  typedef Base::const_reference const_reference;

  template <typename Container>
  static StringList from(Container const& m);

  StringList();
  StringList(Base const& l);
  StringList(Base&& l);
  StringList(StringList const& l);
  StringList(StringList&& l);
  StringList(size_t len, String::Char const* const* list);
  StringList(size_t len, char const* const* list);
  explicit StringList(size_t len, String const& s1 = String());
  StringList(std::initializer_list<String> list);

  template <typename InputIterator>
  StringList(InputIterator beg, InputIterator end)
    : Base(beg, end) {}

  StringList& operator=(Base const& rhs);
  StringList& operator=(Base&& rhs);
  StringList& operator=(StringList const& rhs);
  StringList& operator=(StringList&& rhs);
  StringList& operator=(initializer_list<String> list);

  bool contains(String const& s, String::CaseSensitivity cs = String::CaseSensitive) const;
  StringList trimAll(String const& chars = "") const;
  String join(String const& separator = "") const;

  StringList slice(SliceIndex a = SliceIndex(), SliceIndex b = SliceIndex(), int i = 1) const;

  template <typename Filter>
  StringList filtered(Filter&& filter) const;

  template <typename Comparator>
  StringList sorted(Comparator&& comparator) const;

  StringList sorted() const;
};

std::ostream& operator<<(std::ostream& os, StringList const& list);

template <>
struct hash<String> {
  size_t operator()(String const& s) const;
};

struct CaseInsensitiveStringHash {
  size_t operator()(String const& s) const;
};

struct CaseInsensitiveStringCompare {
  bool operator()(String const& lhs, String const& rhs) const;
};

typedef HashSet<String> StringSet;

template <typename MappedT, typename HashT = hash<String>, typename ComparatorT = std::equal_to<String>>
using StringMap = HashMap<String, MappedT, HashT, ComparatorT>;

template <typename MappedT, typename HashT = hash<String>, typename ComparatorT = std::equal_to<String>>
using StableStringMap = StableHashMap<String, MappedT, HashT, ComparatorT>;

template <typename MappedT>
using CaseInsensitiveStringMap = StringMap<MappedT, CaseInsensitiveStringHash, CaseInsensitiveStringCompare>;

template <>
struct hash<StringList> {
  size_t operator()(StringList const& s) const;
};

template <typename... StringType>
String String::joinWith(
    String const& join, String const& first, String const& second, String const& third, StringType const&... rest) {
  return joinWith(join, joinWith(join, first, second), third, rest...);
}

template <typename Lookup>
String String::lookupTags(Lookup&& lookup) const {
  // Operates directly on the utf8 representation of the strings, rather than
  // using unicode find / replace methods

  auto substrInto = [](std::string const& ref, size_t position, size_t n, std::string& result) {
    auto len = ref.size();
    if (position > len)
      throw OutOfRangeException(strf("out of range in substrInto: {}", position));

    auto it = ref.begin();
    std::advance(it, position);

    for (size_t i = 0; i < n; ++i) {
      if (it == ref.end())
        break;
      result.push_back(*it);
      ++it;
    }
  };

  std::string finalString;

  size_t start = 0;
  size_t size = String::size();

  finalString.reserve(size);

  String key;

  while (true) {
    if (start >= size)
      break;

    size_t beginTag = m_string.find("<", start);
    size_t endTag = m_string.find(">", beginTag);
    if (beginTag != NPos && endTag != NPos) {
      substrInto(m_string, beginTag + 1, endTag - beginTag - 1, key.m_string);
      substrInto(m_string, start, beginTag - start, finalString);
      finalString += lookup(key).m_string;
      key.m_string.clear();
      start = endTag + 1;

    } else {
      substrInto(m_string, start, NPos, finalString);
      break;
    }
  }

  return finalString;
}

template <typename Lookup>
Maybe<String> String::maybeLookupTagsView(Lookup&& lookup) const {
  List<std::string_view> finalViews = {};
  std::string_view view(utf8());

  size_t start = 0;
  while (true) {
    if (start >= view.size())
      break;

    size_t beginTag = view.find_first_of('<', start);
    if (beginTag == NPos && !start)
      return Maybe<String>();

    size_t endTag = view.find_first_of('>', beginTag);
    if (beginTag != NPos && endTag != NPos) {
      finalViews.append(view.substr(start, beginTag - start));
      finalViews.append(lookup(view.substr(beginTag + 1, endTag - beginTag - 1)).takeUtf8());
      start = endTag + 1;
    } else {
      finalViews.append(view.substr(start));
      break;
    }
  }

  std::string finalString;
  size_t finalSize = 0;
  for (auto& view : finalViews)
    finalSize += view.size();

  finalString.reserve(finalSize);

  for (auto& view : finalViews)
    finalString += view;

  return String(finalString);
}

template <typename Lookup>
String String::lookupTagsView(Lookup&& lookup) const {
  auto result = maybeLookupTagsView(lookup);
  return result ? move(result.take()) : String();
}

template <typename MapType>
String String::replaceTags(MapType const& tags, bool replaceWithDefault, String defaultValue) const {
  return lookupTags([&](String const& key) -> String {
    auto i = tags.find(key);
    if (i == tags.end()) {
      if (replaceWithDefault)
        return defaultValue;
      else
        return "<" + key + ">";
    } else {
      return i->second;
    }
  });
}

inline size_t hash<String>::operator()(String const& s) const {
  PLHasher hash;
  for (auto c : s.utf8())
    hash.put(c);
  return hash.hash();
}

template <typename Container>
StringList StringList::from(Container const& m) {
  return StringList(m.begin(), m.end());
}

template <typename Filter>
StringList StringList::filtered(Filter&& filter) const {
  StringList l;
  l.filter(forward<Filter>(filter));
  return l;
}

template <typename Comparator>
StringList StringList::sorted(Comparator&& comparator) const {
  StringList l;
  l.sort(forward<Comparator>(comparator));
  return l;
}

}

template <> struct fmt::formatter<Star::String> : formatter<std::string> {
  fmt::appender format(Star::String const& s, format_context& ctx) const;
};

#endif
