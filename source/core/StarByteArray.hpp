#ifndef STAR_BYTE_ARRAY_H
#define STAR_BYTE_ARRAY_H

#include "StarHash.hpp"
#include "StarException.hpp"
#include "StarFormat.hpp"

namespace Star {

STAR_CLASS(ByteArray);

// Class to hold an array of bytes.  Contains an internal buffer that may be
// larger than what is reported by size(), to avoid repeated allocations when a
// ByteArray grows.
class ByteArray {
public:
  typedef char value_type;
  typedef char* iterator;
  typedef char const* const_iterator;

  // Constructs a byte array from a given c string WITHOUT including the
  // trailing '\0'
  static ByteArray fromCString(char const* str);
  // Same, but includes the trailing '\0'
  static ByteArray fromCStringWithNull(char const* str);
  static ByteArray withReserve(size_t capacity);

  ByteArray();
  ByteArray(size_t dataSize, char c);
  ByteArray(char const* data, size_t dataSize);
  ByteArray(ByteArray const& b);
  ByteArray(ByteArray&& b) noexcept;
  ~ByteArray();

  ByteArray& operator=(ByteArray const& b);
  ByteArray& operator=(ByteArray&& b) noexcept;

  char const* ptr() const;
  char* ptr();

  size_t size() const;
  // Maximum size before realloc
  size_t capacity() const;
  // Is zero size
  bool empty() const;

  // Sets size to 0.
  void clear();
  // Clears and resets buffer to empty.
  void reset();

  void reserve(size_t capacity);

  void resize(size_t size);
  // resize, filling new space with given byte if it exists.
  void resize(size_t size, char f);

  // fill array with byte.
  void fill(char c);
  // fill array and resize to new size.
  void fill(size_t size, char c);

  void append(ByteArray const& b);
  void append(char const* data, size_t len);
  void appendByte(char b);

  void copyTo(char* data, size_t len) const;
  void copyTo(char* data) const;

  // Copy from ByteArray starting at pos, to data, with size len.
  void copyTo(char* data, size_t pos, size_t len) const;
  // Copy from data pointer to ByteArray at pos with size len.
  // Resizes if needed.
  void writeFrom(char const* data, size_t pos, size_t len);

  ByteArray sub(size_t b, size_t s) const;
  ByteArray left(size_t s) const;
  ByteArray right(size_t s) const;

  void trimLeft(size_t s);
  void trimRight(size_t s);

  // returns location of first character that is different than the given
  // ByteArray.
  size_t diffChar(ByteArray const& b) const;
  // returns -1 if this < b, 0 if this == b, 1 if this > b
  int compare(ByteArray const& b) const;

  template <typename Combiner>
  ByteArray combineWith(Combiner&& combine, ByteArray const& rhs, bool extend = false);

  ByteArray andWith(ByteArray const& rhs, bool extend = false);
  ByteArray orWith(ByteArray const& rhs, bool extend = false);
  ByteArray xorWith(ByteArray const& rhs, bool extend = false);

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  void insert(size_t pos, char byte);
  iterator insert(const_iterator pos, char byte);
  void push_back(char byte);

  char& operator[](size_t i);
  char operator[](size_t i) const;
  char at(size_t i) const;

  bool operator<(ByteArray const& b) const;
  bool operator==(ByteArray const& b) const;
  bool operator!=(ByteArray const& b) const;

private:
  char* m_data;
  size_t m_capacity;
  size_t m_size;
};

template <>
struct hash<ByteArray> {
  size_t operator()(ByteArray const& b) const;
};

std::ostream& operator<<(std::ostream& os, ByteArray const& b);

inline void ByteArray::clear() {
  resize(0);
}

inline void ByteArray::resize(size_t size) {
  reserve(size);
  m_size = size;
}

inline void ByteArray::append(ByteArray const& b) {
  append(b.ptr(), b.size());
}

inline void ByteArray::append(const char* data, size_t len) {
  resize(m_size + len);
  std::memcpy(m_data + m_size - len, data, len);
}

inline void ByteArray::appendByte(char b) {
  resize(m_size + 1);
  m_data[m_size - 1] = b;
}

inline bool ByteArray::empty() const {
  return m_size == 0;
}

inline char const* ByteArray::ptr() const {
  return m_data;
}

inline char* ByteArray::ptr() {
  return m_data;
}

inline size_t ByteArray::size() const {
  return m_size;
}

inline size_t ByteArray::capacity() const {
  return m_capacity;
}

inline void ByteArray::copyTo(char* data, size_t len) const {
  len = min(m_size, len);
  std::memcpy(data, m_data, len);
}

inline void ByteArray::copyTo(char* data) const {
  copyTo(data, m_size);
}

inline void ByteArray::copyTo(char* data, size_t pos, size_t len) const {
  if (len == 0 || pos >= m_size)
    return;

  len = min(m_size - pos, len);
  std::memcpy(data, m_data + pos, len);
}

inline void ByteArray::writeFrom(const char* data, size_t pos, size_t len) {
  if (pos + len > m_size)
    resize(pos + len);

  std::memcpy(m_data + pos, data, len);
}

template <typename Combiner>
ByteArray ByteArray::combineWith(Combiner&& combine, ByteArray const& rhs, bool extend) {
  ByteArray const* smallerArray = &rhs;
  ByteArray const* largerArray = this;

  if (m_size < rhs.size())
    swap(smallerArray, largerArray);

  ByteArray res;
  res.resize(smallerArray->size());

  for (size_t i = 0; i < smallerArray->size(); ++i)
    res[i] = combine((*smallerArray)[i], (*largerArray)[i]);

  if (extend) {
    res.resize(largerArray->size());
    for (size_t i = smallerArray->size(); i < largerArray->size(); ++i)
      res[i] = (*largerArray)[i];
  }

  return res;
}

inline ByteArray::iterator ByteArray::begin() {
  return m_data;
}

inline ByteArray::iterator ByteArray::end() {
  return m_data + m_size;
}

inline ByteArray::const_iterator ByteArray::begin() const {
  return m_data;
}

inline ByteArray::const_iterator ByteArray::end() const {
  return m_data + m_size;
}

inline char& ByteArray::operator[](size_t i) {
  starAssert(i < m_size);
  return m_data[i];
}

inline char ByteArray::operator[](size_t i) const {
  starAssert(i < m_size);
  return m_data[i];
}

inline char ByteArray::at(size_t i) const {
  if (i >= m_size)
    throw OutOfRangeException(strf("Out of range in ByteArray::at({})", i));

  return m_data[i];
}

inline size_t hash<ByteArray>::operator()(ByteArray const& b) const {
  PLHasher hash;
  for (size_t i = 0; i < b.size(); ++i)
    hash.put(b[i]);
  return hash.hash();
}

}

template <> struct fmt::formatter<Star::ByteArray> : ostream_formatter {};

#endif
