#include "StarString.hpp"
#include "StarBytes.hpp"
#include "StarFormat.hpp"

#include <cctype>
#include <regex>

namespace Star {

bool String::isSpace(Char c) {
  return
    c == 0x20 || // space
    c == 0x09 || // horizontal tab
    c == 0x0a || // newline
    c == 0x0d || // carriage return
    c == 0xfeff; // BOM or ZWNBSP
}

bool String::isAsciiNumber(Char c) {
  return c >= '0' && c <= '9';
}

bool String::isAsciiLetter(Char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

String::Char String::toLower(Char c) {
  if (c >= 'A' && c <= 'Z')
    return c + 32;
  else
    return c;
}

String::Char String::toUpper(Char c) {
  if (c >= 'a' && c <= 'z')
    return c - 32;
  else
    return c;
}

bool String::charEqual(Char c1, Char c2, CaseSensitivity cs) {
  if (cs == CaseInsensitive)
    return toLower(c1) == toLower(c2);
  else
    return c1 == c2;
}

String String::joinWith(String const& join, String const& left, String const& right) {
  if (left.empty())
    return right;
  if (right.empty())
    return left;

  if (left.endsWith(join)) {
    if (right.beginsWith(join)) {
      return left + right.substr(join.size());
    }
    return left + right;
  } else {
    if (right.beginsWith(join)) {
      return left + right;
    }
    return left + join + right;
  }
}

String::String() {}
String::String(String const& s) : m_string(s.m_string) {}
String::String(String&& s) : m_string(std::move(s.m_string)) {}
String::String(char const* s) : m_string(s) {}
String::String(char const* s, size_t n) : m_string(s, n) {}
String::String(std::string const& s) : m_string(s) {}
String::String(std::string&& s) : m_string(std::move(s)) {}

String::String(std::wstring const& s) {
  reserve(s.length());
  for (Char c : s)
    append(c);
}

String::String(Char const* s) {
  while (*s) {
    append(*s);
    ++s;
  }
}

String::String(Char const* s, size_t n) {
  reserve(n);
  for (size_t idx = 0; idx < n; ++idx) {
    append(*s);
    ++s;
  }
}

String::String(Char c, size_t n) {
  reserve(n);
  for (size_t i = 0; i < n; ++i)
    append(c);
}

String::String(Char c) {
  append(c);
}

std::string const& String::utf8() const {
  return m_string;
}

std::string String::takeUtf8() {
  return take(m_string);
}

ByteArray String::utf8Bytes() const {
  return ByteArray(m_string.c_str(), m_string.size());
}

char const* String::utf8Ptr() const {
  return m_string.c_str();
}

size_t String::utf8Size() const {
  return m_string.size();
}

std::wstring String::wstring() const {
  std::wstring string;
  for (Char c : *this)
    string.push_back(c);
  return string;
}

String::WideString String::wideString() const {
  WideString string;
  string.reserve(m_string.size());
  for (Char c : *this)
    string.push_back(c);
  return string;
}

String::const_iterator String::begin() const {
  return const_iterator(m_string.begin());
}

String::const_iterator String::end() const {
  return const_iterator(m_string.end());
}

size_t String::size() const {
  return utf8Length(m_string.c_str(), m_string.size());
}

size_t String::length() const {
  return size();
}

void String::clear() {
  m_string.clear();
}

void String::reserve(size_t n) {
  m_string.reserve(n);
}

bool String::empty() const {
  return m_string.empty();
}

String::Char String::operator[](size_t index) const {
  auto it = begin();
  for (size_t i = 0; i < index; ++i)
    ++it;
  return *it;
}

size_t CaseInsensitiveStringHash::operator()(String const& s) const {
  PLHasher hash;
  for (auto c : s)
    hash.put(String::toLower(c));
  return hash.hash();
}

bool CaseInsensitiveStringCompare::operator()(String const& lhs, String const& rhs) const {
  return lhs.equalsIgnoreCase(rhs);
}

String::Char String::at(size_t i) const {
  if (i > size())
    throw OutOfRangeException(strf("Out of range in String::at(%s)", i));
  return operator[](i);
}

String String::toUpper() const {
  String s;
  s.reserve(m_string.length());
  for (Char c : *this)
    s.append(toUpper(c));
  return s;
}

String String::toLower() const {
  String s;
  s.reserve(m_string.length());
  for (Char c : *this)
    s.append(toLower(c));
  return s;
}

String String::titleCase() const {
  String s;
  s.reserve(m_string.length());
  bool capNext = true;
  for (Char c : *this) {
    if (capNext)
      s.append(toUpper(c));
    else
      s.append(toLower(c));
    capNext = !std::isalpha(c);
  }
  return s;
}

bool String::endsWith(String const& end, CaseSensitivity cs) const {
  auto endsize = end.size();
  if (endsize == 0)
    return true;

  auto mysize = size();
  if (endsize > mysize)
    return false;

  return compare(mysize - endsize, NPos, end, 0, NPos, cs) == 0;
}

bool String::endsWith(Char end, CaseSensitivity cs) const {
  if (size() == 0)
    return false;

  return charEqual(end, operator[](size() - 1), cs);
}

bool String::beginsWith(String const& beg, CaseSensitivity cs) const {
  auto begSize = beg.size();
  if (begSize == 0)
    return true;

  auto mysize = size();
  if (begSize > mysize)
    return false;

  return compare(0, begSize, beg, 0, NPos, cs) == 0;
}

bool String::beginsWith(Char beg, CaseSensitivity cs) const {
  if (size() == 0)
    return false;

  return charEqual(beg, operator[](0), cs);
}

String String::reverse() const {
  String ret;
  ret.reserve(m_string.length());
  auto i = end();
  while (i != begin()) {
    --i;
    ret.append(*i);
  }
  return ret;
}

String String::rot13() const {
  String ret;
  ret.reserve(m_string.length());
  for (auto c : *this) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
      c += 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
      c -= 13;
    ret.append(c);
  }
  return ret;
}

StringList String::split(Char c, size_t maxSplit) const {
  return split(String(c), maxSplit);
}

StringList String::split(String const& pattern, size_t maxSplit) const {
  StringList ret;
  if (pattern.empty())
    return StringList(1, *this);

  size_t beg = 0;
  while (true) {
    if (ret.size() == maxSplit) {
      ret.append(m_string.substr(beg));
      break;
    }

    size_t end = m_string.find(pattern.m_string, beg);
    if (end == NPos) {
      ret.append(m_string.substr(beg));
      break;
    }
    ret.append(m_string.substr(beg, end - beg));
    beg = end + pattern.m_string.size();
  }

  starAssert(maxSplit == NPos || ret.size() <= maxSplit + 1);
  return ret;
}

StringList String::rsplit(Char c, size_t maxSplit) const {
  return rsplitAny(String(c), maxSplit);
}

StringList String::rsplit(String const& pattern, size_t maxSplit) const {
  // This is really inefficient!
  String v = reverse();
  String p = pattern.reverse();
  StringList l = v.split(p, maxSplit);
  for (auto& s : l)
    s = s.reverse();

  Star::reverse(l);
  return l;
}

StringList String::splitAny(String const& chars, size_t maxSplit) const {
  StringList ret;
  String next;
  bool doneSplitting = false;
  for (auto c : *this) {
    if (!doneSplitting && chars.hasCharOrWhitespace(c)) {
      if (!next.empty())
        ret.append(take(next));
    } else {
      if (ret.size() == maxSplit)
        doneSplitting = true;
      next.append(c);
    }
  }
  if (!next.empty())
    ret.append(move(next));
  return ret;
}

StringList String::rsplitAny(String const& chars, size_t maxSplit) const {
  // This is really inefficient!
  String v = reverse();
  StringList l = v.splitAny(chars, maxSplit);
  for (auto& s : l)
    s = s.reverse();

  Star::reverse(l);
  return l;
}

StringList String::splitLines(size_t maxSplit) const {
  return splitAny("\r\n", maxSplit);
}

StringList String::splitWhitespace(size_t maxSplit) const {
  return splitAny("", maxSplit);
}

String String::extract(String const& chars) {
  StringList l = splitAny(chars, 1);
  if (l.size() == 0) {
    return String();
  } else if (l.size() == 1) {
    clear();
    return l.at(0);
  } else {
    *this = l.at(1);
    return l.at(0);
  }
}

String String::rextract(String const& chars) {
  if (empty())
    return String();

  StringList l = rsplitAny(chars, 1);
  if (l.size() == 1) {
    clear();
    return l.at(0);
  } else {
    *this = l.at(0);
    return l.at(1);
  }
}

bool String::hasChar(Char c) const {
  for (Char ch : *this)
    if (ch == c)
      return true;
  return false;
}

bool String::hasCharOrWhitespace(Char c) const {
  if (empty())
    return isSpace(c);
  else
    return hasChar(c);
}

String String::replace(String const& rplc, String const& val) const {
  size_t index;
  size_t sz = size();
  size_t rsz = rplc.size();
  String ret;
  ret.reserve(m_string.length());

  if (rplc.empty())
    return *this;

  index = find(rplc);
  if (index == NPos)
    return *this;

  auto it = begin();
  for (size_t i = 0; i < index; ++i)
    ret.append(*it++);

  while (index < sz) {
    ret.append(val);
    index += rsz;
    for (size_t i = 0; i < rsz; ++i)
      ++it;

    size_t nindex = find(rplc, index);
    for (size_t i = index; i < nindex && i < sz; ++i)
      ret.append(*it++);

    index = nindex;
  }
  return ret;
}

String String::trimEnd(String const& pattern) const {
  size_t end;
  for (end = size(); end > 0; --end) {
    Char ec = (*this)[end - 1];
    if (!pattern.hasCharOrWhitespace(ec))
      break;
  }
  return substr(0, end);
}

String String::trimBeg(String const& pattern) const {
  size_t beg;
  for (beg = 0; beg < size(); ++beg) {
    Char bc = (*this)[beg];
    if (!pattern.hasCharOrWhitespace(bc))
      break;
  }
  return substr(beg);
}

String String::trim(String const& pattern) const {
  return trimEnd(pattern).trimBeg(pattern);
}

size_t String::find(Char c, size_t pos, CaseSensitivity cs) const {
  auto it = begin();
  for (size_t i = 0; i < pos; ++i) {
    if (it == end())
      break;
    ++it;
  }

  while (it != end()) {
    if (charEqual(c, *it, cs))
      return pos;
    ++pos;
    ++it;
  }

  return NPos;
}

size_t String::find(String const& str, size_t pos, CaseSensitivity cs) const {
  if (str.empty())
    return 0;

  auto it = begin();
  for (size_t i = 0; i < pos; ++i) {
    if (it == end())
      break;
    ++it;
  }

  const_iterator sit = str.begin();
  const_iterator mit = it;
  while (it != end()) {
    if (charEqual(*sit, *mit, cs)) {
      do {
        ++mit;
        ++sit;
        if (sit == str.end())
          return pos;
        else if (mit == end())
          break;
      } while (charEqual(*sit, *mit, cs));
      sit = str.begin();
    }
    ++pos;
    mit = ++it;
  }

  return NPos;
}

size_t String::findLast(Char c, CaseSensitivity cs) const {
  auto it = begin();

  size_t found = NPos;
  size_t pos = 0;
  while (it != end()) {
    if (charEqual(c, *it, cs))
      found = pos;
    ++pos;
    ++it;
  }

  return found;
}

size_t String::findLast(String const& str, CaseSensitivity cs) const {
  if (str.empty())
    return 0;

  size_t pos = 0;
  auto it = begin();
  size_t result = NPos;
  const_iterator sit = str.begin();
  const_iterator mit = it;
  while (it != end()) {
    if (charEqual(*sit, *mit, cs)) {
      do {
        ++mit;
        ++sit;
        if (sit == str.end()) {
          result = pos;
          break;
        }
        if (mit == end())
          break;
      } while (charEqual(*sit, *mit, cs));
      sit = str.begin();
    }
    ++pos;
    mit = ++it;
  }

  return result;
}

size_t String::findFirstOf(String const& pattern, size_t beg) const {
  auto it = begin();
  size_t i;
  for (i = 0; i < beg; ++i)
    ++it;

  while (it != end()) {
    if (pattern.hasCharOrWhitespace(*it))
      return i;
    ++it;
    ++i;
  }
  return NPos;
}

size_t String::findFirstNotOf(String const& pattern, size_t beg) const {
  auto it = begin();
  size_t i;
  for (i = 0; i < beg; ++i)
    ++it;

  while (it != end()) {
    if (!pattern.hasCharOrWhitespace(*it))
      return i;
    ++it;
    ++i;
  }
  return NPos;
}

size_t String::findNextBoundary(size_t index, bool backwards) const {
  starAssert(index <= size());
  if (!backwards && (index == size()))
    return index;
  if (backwards) {
    if (index == 0)
      return 0;
    index--;
  }
  Char c = this->at(index);
  while (!isSpace(c)) {
    if (backwards && (index == 0))
      return 0;
    index += backwards ? -1 : 1;
    if (index == size())
      return size();
    c = this->at(index);
  }
  while (isSpace(c)) {
    if (backwards && (index == 0))
      return 0;
    index += backwards ? -1 : 1;
    if (index == size())
      return size();
    c = this->at(index);
  }
  if (backwards && !(index == size()))
    return index + 1;
  return index;
}

String String::slice(SliceIndex a, SliceIndex b, int i) const {
  auto wide = wideString();
  wide = Star::slice(wide, a, b, i);
  return String(wide.c_str());
}

void String::append(String const& string) {
  m_string.append(string.m_string);
}

void String::append(std::string const& s) {
  m_string.append(s);
}

void String::append(Char const* s) {
  while (s)
    append(*s++);
}

void String::append(Char const* s, size_t n) {
  for (size_t i = 0; i < n; ++i)
    append(s[i]);
}

void String::append(char const* s) {
  m_string.append(s);
}

void String::append(char const* s, size_t n) {
  m_string.append(s, n);
}

void String::append(Char c) {
  char conv[6];
  size_t size = utf8EncodeChar(conv, c, 6);
  append(conv, size);
}

void String::prepend(String const& s) {
  auto ns = s;
  ns.append(*this);
  *this = move(ns);
}

void String::prepend(std::string const& s) {
  auto ns = String(s);
  ns.append(*this);
  *this = move(ns);
}

void String::prepend(Char const* s) {
  auto ns = String(s);
  ns.append(*this);
  *this = move(ns);
}

void String::prepend(Char const* s, size_t n) {
  auto ns = String(s, n);
  ns.append(*this);
  *this = move(ns);
}

void String::prepend(char const* s) {
  auto ns = String(s);
  ns.append(*this);
  *this = move(ns);
}

void String::prepend(char const* s, size_t n) {
  auto ns = String(s, n);
  ns.append(*this);
  *this = move(ns);
}

void String::prepend(Char c) {
  auto ns = String(c, 1);
  ns.append(*this);
  *this = move(ns);
}

void String::push_back(Char c) {
  append(c);
}

void String::push_front(Char c) {
  prepend(c);
}

bool String::contains(String const& s, CaseSensitivity cs) const {
  return find(s, 0, cs) != NPos;
}

bool String::regexMatch(String const& regex, bool full, bool caseSensitive) const {
  if (full) {
    if (caseSensitive)
      return std::regex_match(utf8(), std::regex(regex.utf8()));
    else
      return std::regex_match(utf8(), std::regex(regex.utf8(), std::regex::icase));
  } else {
    if (caseSensitive)
      return std::regex_search(utf8(), std::regex(regex.utf8()));
    else
      return std::regex_search(utf8(), std::regex(regex.utf8(), std::regex::icase));
  }
}

int String::compare(String const& s, CaseSensitivity cs) const {
  if (cs == CaseSensitive)
    return m_string.compare(s.m_string);
  else
    return compare(0, NPos, s, 0, NPos, cs);
}

bool String::equals(String const& s, CaseSensitivity cs) const {
  return compare(s, cs) == 0;
}

bool String::equalsIgnoreCase(String const& s) const {
  return compare(s, CaseInsensitive) == 0;
}

String String::substr(size_t position, size_t n) const {
  auto len = size();
  if (position > len)
    throw OutOfRangeException(strf("out of range in String::substr(%s, %s)", position, n));

  if (position == 0 && n >= len)
    return *this;

  String ret;
  ret.reserve(std::min(n, len - position));

  auto it = begin();
  std::advance(it, position);

  for (size_t i = 0; i < n; ++i) {
    if (it == end())
      break;
    ret.append(*it);
    ++it;
  }

  return ret;
}

void String::erase(size_t pos, size_t n) {
  String ns;
  ns.reserve(m_string.size() - std::min(n, m_string.size()));
  auto it = begin();
  for (size_t i = 0; i < pos; ++i)
    ns.append(*it++);
  for (size_t i = 0; i < n; ++i) {
    if (it == end())
      break;
    ++it;
  }
  while (it != end())
    ns.append(*it++);
  *this = ns;
}

String String::padLeft(size_t size, String const& filler) const {
  if (!filler.length())
    return *this;
  String rs;
  while (rs.length() + length() < size) {
    rs.append(filler);
  }
  if (rs.length())
    return rs + *this;
  return *this;
}

String String::padRight(size_t size, String const& filler) const {
  if (!filler.length())
    return *this;
  String rs = *this;
  while (rs.length() < size) {
    rs.append(filler);
  }
  return rs;
}

String& String::operator=(String const& s) {
  m_string = s.m_string;
  return *this;
}

String& String::operator=(String&& s) {
  m_string = move(s.m_string);
  return *this;
}

String& String::operator+=(String const& s) {
  append(s);
  return *this;
}

String& String::operator+=(std::string const& s) {
  append(s);
  return *this;
}

String& String::operator+=(Char const* s) {
  append(s);
  return *this;
}

String& String::operator+=(char const* s) {
  append(s);
  return *this;
}

String& String::operator+=(Char c) {
  append(c);
  return *this;
}

bool operator==(String const& s1, String const& s2) {
  return s1.m_string == s2.m_string;
}

bool operator==(String const& s1, std::string const& s2) {
  return s1.m_string == s2;
}

bool operator==(String const& s1, String::Char const* s2) {
  return s1 == String(s2);
}

bool operator==(String const& s1, char const* s2) {
  return s1.m_string == s2;
}

bool operator==(std::string const& s1, String const& s2) {
  return s1 == s2.m_string;
}

bool operator==(String::Char const* s1, String const& s2) {
  return String(s1) == s2;
}

bool operator==(char const* s1, String const& s2) {
  return s1 == s2.m_string;
}

bool operator!=(String const& s1, String const& s2) {
  return s1.m_string != s2.m_string;
}

bool operator!=(String const& s1, std::string const& s2) {
  return s1.m_string != s2;
}

bool operator!=(String const& s1, String::Char const* s2) {
  return s1 != String(s2);
}

bool operator!=(String const& s1, char const* s2) {
  return s1.m_string != s2;
}

bool operator!=(std::string const& s1, String const& s2) {
  return s1 != s2.m_string;
}

bool operator!=(String::Char const* s1, String const& s2) {
  return String(s1) != s2;
}

bool operator!=(char const* s1, String const& s2) {
  return s1 != s2.m_string;
}

bool operator<(String const& s1, String const& s2) {
  return s1.m_string < s2.m_string;
}

bool operator<(String const& s1, std::string const& s2) {
  return s1.m_string < s2;
}

bool operator<(String const& s1, String::Char const* s2) {
  return s1 < String(s2);
}

bool operator<(String const& s1, char const* s2) {
  return s1.m_string < s2;
}

bool operator<(std::string const& s1, String const& s2) {
  return s1 < s2.m_string;
}

bool operator<(String::Char const* s1, String const& s2) {
  return String(s1) < s2;
}

bool operator<(char const* s1, String const& s2) {
  return s1 < s2.m_string;
}

String operator+(String s1, String const& s2) {
  s1.append(s2);
  return s1;
}

String operator+(String s1, std::string const& s2) {
  s1.append(s2);
  return s1;
}

String operator+(String s1, String::Char const* s2) {
  s1.append(s2);
  return s1;
}

String operator+(String s1, char const* s2) {
  s1.append(s2);
  return s1;
}

String operator+(std::string const& s1, String const& s2) {
  return s1 + s2.m_string;
}

String operator+(String::Char const* s1, String const& s2) {
  return String(s1) + s2;
}

String operator+(char const* s1, String const& s2) {
  return s1 + s2.m_string;
}

String operator+(String s, String::Char c) {
  s.append(c);
  return s;
}

String operator+(String::Char c, String const& s) {
  String res(c);
  res.append(s);
  return res;
}

String operator*(String const& s, unsigned times) {
  String res;
  for (unsigned i = 0; i < times; ++i)
    res.append(s);
  return res;
}

String operator*(unsigned times, String const& s) {
  return s * times;
}

std::ostream& operator<<(std::ostream& os, String const& s) {
  os << s.utf8();
  return os;
}

std::istream& operator>>(std::istream& is, String& s) {
  std::string temp;
  is >> temp;
  s = String(std::move(temp));
  return is;
}

int String::compare(size_t selfOffset, size_t selfLen, String const& other,
    size_t otherOffset, size_t otherLen, CaseSensitivity cs) const {
  auto selfIt = begin();
  auto otherIt = other.begin();

  while (selfOffset > 0 && selfIt != end()) {
    ++selfIt;
    --selfOffset;
  }

  while (otherOffset > 0 && otherIt != other.end()) {
    ++otherIt;
    --otherLen;
  }

  while (true) {
    if ((selfIt == end() || selfLen == 0) && (otherIt == other.end() || otherLen == 0))
      return 0;
    else if (selfIt == end() || selfLen == 0)
      return -1;
    else if (otherIt == other.end() || otherLen == 0)
      return 1;

    auto c1 = *selfIt;
    auto c2 = *otherIt;

    if (cs == CaseInsensitive) {
      c1 = toLower(c1);
      c2 = toLower(c2);
    }

    if (c1 < c2)
      return -1;
    else if (c2 < c1)
      return 1;

    ++selfIt;
    ++otherIt;
    --selfLen;
    --otherLen;
  }
}

StringList::StringList() : Base() {}

StringList::StringList(Base const& l) : Base(l) {}

StringList::StringList(Base&& l) : Base(std::move(l)) {}

StringList::StringList(StringList const& l) : Base(l) {}

StringList::StringList(StringList&& l) : Base(std::move(l)) {}

StringList::StringList(size_t n, String::Char const* const* list) {
  for (size_t i = 0; i < n; ++i)
    append(String(list[i]));
}

StringList::StringList(size_t n, char const* const* list) {
  for (size_t i = 0; i < n; ++i)
    append(String(list[i]));
}

StringList::StringList(size_t len, String const& s1) : Base(len, s1) {}

StringList::StringList(std::initializer_list<String> list) : Base(list) {}

StringList& StringList::operator=(Base const& rhs) {
  Base::operator=(rhs);
  return *this;
}

StringList& StringList::operator=(Base&& rhs) {
  Base::operator=(std::move(rhs));
  return *this;
}

StringList& StringList::operator=(StringList const& rhs) {
  Base::operator=(rhs);
  return *this;
}

StringList& StringList::operator=(StringList&& rhs) {
  Base::operator=(std::move(rhs));
  return *this;
}

StringList& StringList::operator=(initializer_list<String> list) {
  Base::operator=(std::move(list));
  return *this;
}

bool StringList::contains(String const& s, String::CaseSensitivity cs) const {
  for (const_iterator i = begin(); i != end(); ++i) {
    if (s.compare(*i, cs) == 0)
      return true;
  }
  return false;
}

StringList StringList::trimAll(String const& pattern) const {
  StringList r;
  for (auto const& s : *this)
    r.append(s.trim(pattern));
  return r;
}

String StringList::join(String const& separator) const {
  String joinedString;
  for (const_iterator i = begin(); i != end(); ++i) {
    if (i != begin())
      joinedString += separator;
    joinedString += *i;
  }

  return joinedString;
}

StringList StringList::slice(SliceIndex a, SliceIndex b, int i) const {
  return Star::slice(*this, a, b, i);
}

StringList StringList::sorted() const {
  StringList l = *this;
  l.sort();
  return l;
}

std::ostream& operator<<(std::ostream& os, const StringList& list) {
  os << "(";
  for (auto i = list.begin(); i != list.end(); ++i) {
    if (i != list.begin())
      os << ", ";

    os << '\'' << *i << '\'';
  }
  os << ")";
  return os;
}

size_t hash<StringList>::operator()(StringList const& sl) const {
  size_t h = 0;
  for (auto const& s : sl)
    hashCombine(h, hashOf(s));
  return h;
}

}
