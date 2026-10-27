#pragma once

#include "StarByteArray.hpp"
#include "StarMaybe.hpp"

namespace Star {

STAR_EXCEPTION(UnicodeException, StarException);

typedef char Utf8Type;
typedef char32_t Utf32Type;

#define STAR_UTF32_REPLACEMENT_CHAR 0x0000fffdL

// Returns false if the given string contains invalid characters.
bool isValidUtf8(Utf8Type const* utf8, size_t size = NPos);
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
inline bool isUtf16Surrogate(Utf32Type codepoint) {
  return codepoint >= 0xd800 && codepoint <= 0xdfff;
}

Utf32Type utf32FromUtf16SurrogatePair(Utf32Type lead, Utf32Type trail);
pair<Utf32Type, Maybe<Utf32Type>> utf32ToUtf16SurrogatePair(Utf32Type codepoint);

// Bidirectional iterator that can make utf8 appear as utf32
// Requires a begin and end specified (to prevent out of bounds reads with invalid utf8)
template <class BaseIterator, class U32Type = Utf32Type>
class U8ToU32Iterator {
public:
  typedef ptrdiff_t difference_type;
  typedef U32Type value_type;
  typedef U32Type* pointer;
  typedef U32Type& reference;
  typedef std::bidirectional_iterator_tag iterator_category;

  U8ToU32Iterator() : m_position(), m_value(pending_read), m_begin(), m_end() {}

  U8ToU32Iterator(BaseIterator b, BaseIterator begin, BaseIterator end) : m_position(b), m_value(pending_read), m_begin(begin), m_end(end) {}

  U8ToU32Iterator(BaseIterator b, U8ToU32Iterator base) : m_position(b), m_value(pending_read), m_begin(base.m_begin), m_end(base.m_end) {}

  BaseIterator const& base() const {
    return m_position;
  }

  U32Type operator*() const {
    if (m_value == pending_read)
      extract_current();
    if (m_value == invalid)
      return STAR_UTF32_REPLACEMENT_CHAR;
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
  // special value for pending iterator reads:
  static U32Type const pending_read = 0xffffffffu;
  // special value for internal 'invalid' chars, to avoid cases where valid characters matching the placeholder are treated as invalid
  static U32Type const invalid = 0xfffffffeu;

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

  U32Type getU32(BaseIterator at) const {
    // returns the invalid value where invalid
    U32Type value = static_cast<Utf8Type>(*at);
    // we must not have a continuation character, and the lead must be valid (>0xf8 is too long, 0xf5-0xf7 always encode invalid chars (above 0x10ffff))
    if (((uint8_t)value & 0xC0u) == 0x80u || (uint8_t)value >= 0xf5u) {
      return invalid;
    }
    // see how many extra byts we have:
    unsigned extra = utf8_trailing_byte_count(*at);
    // extract the extra bits, 6 from each extra byte:
    BaseIterator next(at);
    for (unsigned c = 0; c < extra; ++c) {
      ++next;
      if (next == m_end) {
        return invalid;
      }
      value <<= 6;
      auto entry = static_cast<uint8_t>(*next);
      if ((entry & 0xC0u) != 0x80u) {
        // Not a continuation char
        return invalid;
      }
      value += entry & 0x3Fu;
    }
    // we now need to remove a few of the leftmost bits, but how many depends
    // upon how many extra bytes we've extracted:
    static const Utf32Type masks[4] = {
      0x7Fu, 0x7FFu, 0xFFFFu, 0x1FFFFFu};
    value &= masks[extra];
    // check the result:
    static const uint32_t overlong[4] = {
      0x00u, 0x80u, 0x800u, 0x10000u};
    if ((uint32_t)value > (uint32_t)0x10FFFFu || (uint32_t)value < overlong[extra] || isUtf16Surrogate(value)) {
      // Prevent overlong or invalid characters
      return invalid;
    }
    return value;
  }

  void increment() {
    // skip high surrogate first if there is one:
    unsigned c = utf8_byte_count(*m_position);
    if (c > 1) {
      // ensure the sequence is valid (invalid sequences are treated as individual characters) by decoding the entire character
      // if it's invalid, just increment once to the next character
      if (m_value == pending_read) {
        extract_current();
      }
      if (m_value == invalid) {
        ++m_position;
      } else {
        std::advance(m_position, c);
      }
    } else {
      ++m_position;
    }
    m_value = pending_read;
  }

  void decrement() {
    // Keep backtracking until we don't have a trailing character:
    unsigned count = 0;
    BaseIterator previous(m_position);
    while (((uint8_t)*--previous & 0xC0u) == 0x80u && previous != m_begin)
      ++count;
    // now check that the sequence was valid:
    U32Type value;
    if (count != utf8_trailing_byte_count(*previous) || (value = getU32(previous)) == invalid) {
      m_position--;
      m_value = invalid;
      return;
    }
    m_value = value;
    m_position = previous;
  }

  bool equal(const U8ToU32Iterator& that) const {
    return m_position == that.m_position;
  }

  void extract_current() const {
    m_value = getU32(m_position);
  }

  BaseIterator m_position;
  mutable U32Type m_value;

  BaseIterator m_begin;
  BaseIterator m_end;
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
  void push(U32Type c) const {
    if (c > 0x10FFFFu) {
      push(STAR_UTF32_REPLACEMENT_CHAR);
      return;
    }

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

}// namespace Star
