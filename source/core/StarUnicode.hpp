#pragma once

#include "StarByteArray.hpp"
#include "StarMaybe.hpp"

namespace Star {

STAR_EXCEPTION(UnicodeException, StarException);

typedef char Utf8Type;
typedef char32_t Utf32Type;

#define STAR_UTF32_REPLACEMENT_CHAR 0x000000b7L

void throwInvalidUtf8Sequence();
void throwMissingUtf8End();
void throwInvalidUtf32CodePoint(Utf32Type val);

// If passed NPos as a size, assumes modified UTF-8 and stops on NULL byte.
// Otherwise, ignores NULL.
size_t utf8Length(Utf8Type const* utf8, size_t size = NPos);
// Encode up to six utf8 bytes into a utf32 character.  If passed NPos as len,
// assumes modified UTF-8 and stops on NULL, otherwise ignores.
size_t utf8DecodeChar(Utf8Type const* utf8, Utf32Type* utf32, size_t len = NPos);
// Encode single utf32 char into up to 6 utf8 characters.
size_t utf8EncodeChar(Utf8Type* utf8, Utf32Type utf32, size_t len = 6);

Utf32Type hexStringToUtf32(std::string const& codepoint, Maybe<Utf32Type> previousCodepoint = {});
std::string hexStringFromUtf32(Utf32Type character);

bool isUtf16LeadSurrogate(Utf32Type codepoint);
bool isUtf16TrailSurrogate(Utf32Type codepoint);

Utf32Type utf32FromUtf16SurrogatePair(Utf32Type lead, Utf32Type trail);
pair<Utf32Type, Maybe<Utf32Type>> utf32ToUtf16SurrogatePair(Utf32Type codepoint);

// Bidirectional iterator that can make utf8 appear as utf32
template <class BaseIterator, class U32Type = Utf32Type>
class U8ToU32Iterator {
public:
  typedef ptrdiff_t difference_type;
  typedef U32Type value_type;
  typedef U32Type* pointer;
  typedef U32Type& reference;
  typedef std::bidirectional_iterator_tag iterator_category;

  U8ToU32Iterator() : m_position(), m_value(pending_read) {}

  U8ToU32Iterator(BaseIterator b) : m_position(b), m_value(pending_read) {}

  BaseIterator const& base() const {
    return m_position;
  }

  U32Type const& operator*() const {
    if (m_value == pending_read)
      extract_current();
    return m_value;
  }

  U8ToU32Iterator const& operator++() {
    increment();
    return *this;
  }

  U8ToU32Iterator operator++(int) {
    U8ToU32Iterator clone(*this);
    increment();
    return clone;
  }

  U8ToU32Iterator const& operator--() {
    decrement();
    return *this;
  }

  U8ToU32Iterator operator--(int) {
    U8ToU32Iterator clone(*this);
    decrement();
    return clone;
  }

  bool operator==(U8ToU32Iterator const& that) const {
    return equal(that);
  }

  bool operator!=(U8ToU32Iterator const& that) const {
    return !equal(that);
  }

private:
  // special values for pending iterator reads:
  static U32Type const pending_read = 0xffffffffu;

  static void invalid_sequence() {
    throwInvalidUtf8Sequence();
  }

  static unsigned utf8_byte_count(Utf8Type c) {
    // if the most significant bit with a zero in it is in position
    // 8-N then there are N bytes in this UTF-8 sequence:
    uint8_t mask = 0x80u;
    unsigned result = 0;
    while (c & mask) {
      ++result;
      mask >>= 1;
    }
    return (result == 0) ? 1 : ((result > 4) ? 4 : result);
  }

  static unsigned utf8_trailing_byte_count(Utf8Type c) {
    return utf8_byte_count(c) - 1;
  }

  void increment() {
    // skip high surrogate first if there is one:
    unsigned c = utf8_byte_count(*m_position);
    std::advance(m_position, c);
    m_value = pending_read;
  }

  void decrement() {
    // Keep backtracking until we don't have a trailing character:
    unsigned count = 0;
    while (((uint8_t) * --m_position & 0xC0u) == 0x80u)
      ++count;
    // now check that the sequence was valid:
    if (count != utf8_trailing_byte_count(*m_position))
      invalid_sequence();
    m_value = pending_read;
  }

  bool equal(const U8ToU32Iterator& that) const {
    return m_position == that.m_position;
  }

  void extract_current() const {
    m_value = static_cast<Utf8Type>(*m_position);
    // we must not have a continuation character:
    if (((uint8_t)m_value & 0xC0u) == 0x80u)
      invalid_sequence();
    // see how many extra byts we have:
    unsigned extra = utf8_trailing_byte_count(*m_position);
    // extract the extra bits, 6 from each extra byte:
    BaseIterator next(m_position);
    for (unsigned c = 0; c < extra; ++c) {
      ++next;
      m_value <<= 6;
      auto entry = static_cast<uint8_t>(*next);
      if ((c > 0) && ((entry & 0xC0u) != 0x80u))
        invalid_sequence();
      m_value += entry & 0x3Fu;
    }
    // we now need to remove a few of the leftmost bits, but how many depends
    // upon how many extra bytes we've extracted:
    static const Utf32Type masks[4] = {
        0x7Fu, 0x7FFu, 0xFFFFu, 0x1FFFFFu,
    };
    m_value &= masks[extra];
    // check the result:
    if ((uint32_t)m_value > (uint32_t)0x10FFFFu)
      invalid_sequence();
  }

  BaseIterator m_position;
  mutable U32Type m_value;
};

// Output iterator
template <class BaseIterator, class U32Type = Utf32Type>
class Utf8OutputIterator {
public:
  typedef void difference_type;
  typedef void value_type;
  typedef U32Type* pointer;
  typedef U32Type& reference;

  Utf8OutputIterator(const BaseIterator& b) : m_position(b) {}
  Utf8OutputIterator(const Utf8OutputIterator& that) : m_position(that.m_position) {}
  Utf8OutputIterator& operator=(const Utf8OutputIterator& that) {
    m_position = that.m_position;
    return *this;
  }

  const Utf8OutputIterator& operator*() const {
    return *this;
  }

  void operator=(U32Type val) const {
    push(val);
  }

  Utf8OutputIterator& operator++() {
    return *this;
  }

  Utf8OutputIterator& operator++(int) {
    return *this;
  }

private:
  static void invalid_utf32_code_point(U32Type val) {
    throwInvalidUtf32CodePoint(val);
  }

  void push(U32Type c) const {
    if (c > 0x10FFFFu)
      invalid_utf32_code_point(c);

    if ((uint32_t)c < 0x80u) {
      *m_position++ = static_cast<Utf8Type>((uint32_t)c);
    } else if ((uint32_t)c < 0x800u) {
      *m_position++ = static_cast<Utf8Type>(0xC0u + ((uint32_t)c >> 6));
      *m_position++ = static_cast<Utf8Type>(0x80u + ((uint32_t)c & 0x3Fu));
    } else if ((uint32_t)c < 0x10000u) {
      *m_position++ = static_cast<Utf8Type>(0xE0u + ((uint32_t)c >> 12));
      *m_position++ = static_cast<Utf8Type>(0x80u + (((uint32_t)c >> 6) & 0x3Fu));
      *m_position++ = static_cast<Utf8Type>(0x80u + ((uint32_t)c & 0x3Fu));
    } else {
      *m_position++ = static_cast<Utf8Type>(0xF0u + ((uint32_t)c >> 18));
      *m_position++ = static_cast<Utf8Type>(0x80u + (((uint32_t)c >> 12) & 0x3Fu));
      *m_position++ = static_cast<Utf8Type>(0x80u + (((uint32_t)c >> 6) & 0x3Fu));
      *m_position++ = static_cast<Utf8Type>(0x80u + ((uint32_t)c & 0x3Fu));
    }
  }

  mutable BaseIterator m_position;
};

}
