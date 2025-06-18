#include "StarStringView.hpp"

namespace Star {
// To string
String::String(StringView s) : m_string(s.utf8()) {}
String::String(std::string_view s) : m_string(s) {}

String& String::operator+=(StringView s) {
  m_string += s.utf8();
  return *this;
}

String& String::operator+=(std::string_view s) {
  m_string += s;
  return *this;
}

StringView::StringView() {}
StringView::StringView(StringView const& s) : m_view(s.m_view) {}
StringView::StringView(StringView&& s) noexcept : m_view(std::move(s.m_view)) {};
StringView::StringView(String const& s) : m_view(s.utf8()) {};
StringView::StringView(char const* s) : m_view(s) {};
StringView::StringView(char const* s, size_t n) : m_view(s, n) {};

StringView::StringView(std::string_view const& s) : m_view(s) {};
StringView::StringView(std::string_view&& s) noexcept : m_view(std::move(s)) {};
StringView::StringView(std::string const& s) : m_view(s) {}

StringView::StringView(Char const* s) : m_view((char const*)s, sizeof(*s)) {}
StringView::StringView(Char const* s, size_t n) : m_view((char const*)s, n * sizeof(*s)) {};

std::string_view const& StringView::utf8() const {
  return m_view;
}

std::string_view StringView::takeUtf8() {
  return take(m_view);
}

ByteArray StringView::utf8Bytes() const {
  return ByteArray(m_view.data(), m_view.size());
}

char const* StringView::utf8Ptr() const {
  return m_view.data();
}

size_t StringView::utf8Size() const {
  return m_view.size();
}

StringView::const_iterator StringView::begin() const {
  return const_iterator(m_view.begin());
}

StringView::const_iterator StringView::end() const {
  return const_iterator(m_view.end());
}

size_t StringView::size() const {
  return utf8Length(m_view.data(), m_view.size());
}

size_t StringView::length() const {
  return size();
}

bool StringView::empty() const {
  return m_view.empty();
}

StringView::Char StringView::operator[](size_t index) const {
  auto it = begin();
  for (size_t i = 0; i < index; ++i)
    ++it;
  return *it;
}

StringView::Char StringView::at(size_t index) const {
  auto it = begin();
  auto itEnd = end();
  for (size_t i = 0; i < index; ++i) {
    ++it;
    if (it == itEnd)
      throw OutOfRangeException(strf("Out of range in StringView::at({})", i));
  }
  return *it;
}

bool StringView::endsWith(StringView end, CaseSensitivity cs) const {
  auto endsize = end.size();
  if (endsize == 0)
    return true;

  auto mysize = size();
  if (endsize > mysize)
    return false;

  return compare(mysize - endsize, NPos, end, 0, NPos, cs) == 0;
}
bool StringView::endsWith(Char end, CaseSensitivity cs) const {
  if (m_view.empty())
    return false;

  return String::charEqual(end, operator[](size() - 1), cs);
}

bool StringView::beginsWith(StringView beg, CaseSensitivity cs) const {
  if (beg.m_view.empty())
    return true;

  size_t begSize = beg.size();
  auto it = begin();
  auto itEnd = end();
  for (size_t i = 0; i != begSize; ++i)
  {
    if (it == itEnd)
      return false;
    it++;
  }

  return compare(0, begSize, beg, 0, NPos, cs) == 0;
}

bool StringView::beginsWith(Char beg, CaseSensitivity cs) const {
  if (m_view.empty())
    return false;

  return String::charEqual(beg, operator[](0), cs);
}

void StringView::forEachSplitAnyView(StringView chars, SplitCallback callback) const {
  if (chars.empty())
    return;

  size_t beg = 0;
  while (true) {
    size_t end = m_view.find_first_of(chars.m_view, beg);
    if (end == NPos) {
      callback(m_view.substr(beg), beg, m_view.size() - beg);
      break;
    }
    callback(m_view.substr(beg, end - beg), beg, end - beg);
    beg = end + 1;
  }
}

void StringView::forEachSplitView(StringView pattern, SplitCallback callback) const {
  if (pattern.empty())
    return;

  size_t beg = 0;
  while (true) {
    size_t end = m_view.find(pattern.m_view, beg);
    if (end == NPos) {
      callback(m_view.substr(beg), beg, m_view.size() - beg);
      break;
    }
    callback(m_view.substr(beg, end - beg), beg, end - beg);
    beg = end + pattern.m_view.size();
  }
}

bool StringView::hasChar(Char c) const {
  for (Char ch : *this)
    if (ch == c)
      return true;
  return false;
}

bool StringView::hasCharOrWhitespace(Char c) const {
  return empty() ? String::isSpace(c) : hasChar(c);
}

size_t StringView::find(Char c, size_t pos, CaseSensitivity cs) const {
  auto it = begin();
  for (size_t i = 0; i < pos; ++i) {
    if (it == end())
      break;
    ++it;
  }

  while (it != end()) {
    if (String::charEqual(c, *it, cs))
      return pos;
    ++pos;
    ++it;
  }

  return NPos;
}

size_t StringView::find(StringView str, size_t pos, CaseSensitivity cs) const {
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
    if (String::charEqual(*sit, *mit, cs)) {
      do {
        ++mit;
        ++sit;
        if (sit == str.end())
          return pos;
        else if (mit == end())
          break;
      } while (String::charEqual(*sit, *mit, cs));
      sit = str.begin();
    }
    ++pos;
    mit = ++it;
  }

  return NPos;
}

size_t StringView::findLast(Char c, CaseSensitivity cs) const {
  auto it = begin();

  size_t found = NPos;
  size_t pos = 0;
  while (it != end()) {
    if (String::charEqual(c, *it, cs))
      found = pos;
    ++pos;
    ++it;
  }

  return found;
}

size_t StringView::findLast(StringView str, CaseSensitivity cs) const {
  if (str.empty())
    return 0;

  size_t pos = 0;
  auto it = begin();
  size_t result = NPos;
  const_iterator sit = str.begin();
  const_iterator mit = it;
  while (it != end()) {
    if (String::charEqual(*sit, *mit, cs)) {
      do {
        ++mit;
        ++sit;
        if (sit == str.end()) {
          result = pos;
          break;
        }
        if (mit == end())
          break;
      } while (String::charEqual(*sit, *mit, cs));
      sit = str.begin();
    }
    ++pos;
    mit = ++it;
  }

  return result;
}

size_t StringView::findFirstOf(StringView pattern, size_t beg) const {
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

size_t StringView::findFirstNotOf(StringView pattern, size_t beg) const {
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

size_t StringView::findNextBoundary(size_t index, bool backwards) const {
  //TODO: Make this faster.
  size_t mySize = size();
  starAssert(index <= mySize);
  if (!backwards && (index == mySize))
    return index;
  if (backwards) {
    if (index == 0)
      return 0;
    index--;
  }
  Char c = this->at(index);
  while (!String::isSpace(c)) {
    if (backwards && (index == 0))
      return 0;
    index += backwards ? -1 : 1;
    if (index == mySize)
      return mySize;
    c = this->at(index);
  }
  while (String::isSpace(c)) {
    if (backwards && (index == 0))
      return 0;
    index += backwards ? -1 : 1;
    if (index == mySize)
      return mySize;
    c = this->at(index);
  }
  if (backwards && !(index == mySize))
    return index + 1;
  return index;
}

bool StringView::contains(StringView s, CaseSensitivity cs) const {
  return find(s, 0, cs) != NPos;
}

int StringView::compare(StringView s, CaseSensitivity cs) const {
  if (cs == CaseSensitivity::CaseSensitive)
    return m_view.compare(s.m_view);
  else
    return compare(0, NPos, s, 0, NPos, cs);
}

bool StringView::equals(StringView s, CaseSensitivity cs) const {
  return compare(s, cs) == 0;
}

bool StringView::equalsIgnoreCase(StringView s) const {
  return compare(s, CaseSensitivity::CaseInsensitive) == 0;
}

StringView StringView::substr(size_t position, size_t n) const {
  StringView ret;
  auto itEnd = end();
  auto it = begin();

  for (size_t i = 0; i != position; ++i) {
    if (it == itEnd)
      throw OutOfRangeException(strf("out of range in StringView::substr({}, {})", position, n));
    it++;
  }

  const auto itStart = it;

  for (size_t i = 0; i != n; ++i) {
    if (it == itEnd)
      break;
    ++it;
  }

  return StringView(&*itStart.base(), it.base() - itStart.base());
}

int StringView::compare(size_t selfOffset, size_t selfLen, StringView other,
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

    if (cs == CaseSensitivity::CaseInsensitive) {
      c1 = String::toLower(c1);
      c2 = String::toLower(c2);
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

StringView& StringView::operator=(StringView s) {
  m_view = s.m_view;
  return *this;
}

bool operator==(StringView s1, const char* s2) {
  return s1.m_view.compare(s2) == 0;
}

bool operator==(StringView s1, std::string const& s2) {
  return s1.m_view.compare(s2) == 0;
}

bool operator==(StringView s1, String const& s2) {
  return s1.m_view.compare(s2.utf8()) == 0;
}

bool operator==(StringView s1, StringView s2) {
  return s1.m_view == s2.m_view;
}

bool operator!=(StringView s1, StringView s2) {
  return s1.m_view != s2.m_view;
}

bool operator<(StringView s1, StringView s2) {
  return s1.m_view < s2.m_view;
}

std::ostream& operator<<(std::ostream& os, StringView const& s) {
  os << s.utf8();
  return os;
}

}

fmt::appender fmt::formatter<Star::StringView>::format(Star::StringView const& s, format_context& ctx) const {
  return formatter<std::string_view>::format(s.utf8(), ctx);
};