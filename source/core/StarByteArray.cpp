#include "StarByteArray.hpp"
#include "StarEncode.hpp"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace Star {

ByteArray ByteArray::fromCString(char const* str) {
  return ByteArray(str, strlen(str));
}

ByteArray ByteArray::fromCStringWithNull(char const* str) {
  size_t len = strlen(str);
  ByteArray ba(str, len + 1);
  ba[len] = 0;
  return ba;
}

ByteArray ByteArray::withReserve(size_t capacity) {
  ByteArray bytes;
  bytes.reserve(capacity);
  return bytes;
}

ByteArray::ByteArray() {
  m_data = nullptr;
  m_capacity = 0;
  m_size = 0;

  m_allocation = AllocationType::Heap;
  m_mapped_addr = nullptr;
  m_mapped_len = 0;
}

ByteArray::ByteArray(size_t dataSize, char c)
  : ByteArray() {
  fill(dataSize, c);
}

ByteArray::ByteArray(const char* data, size_t dataSize)
  : ByteArray() {
  append(data, dataSize);
}

ByteArray::ByteArray(ByteArray const& b)
  : ByteArray() {
  operator=(b);
}

ByteArray::ByteArray(ByteArray&& b) noexcept
  : ByteArray() {
  operator=(std::move(b));
}

ByteArray::~ByteArray() {
  reset();
}

ByteArray& ByteArray::operator=(ByteArray const& b) {
  if (&b != this) {
    clear();
    append(b);
  }

  return *this;
}

ByteArray& ByteArray::operator=(ByteArray&& b) noexcept {
  if (&b != this) {
    reset();

    m_data = take(b.m_data);
    m_capacity = take(b.m_capacity);
    m_size = take(b.m_size);
  }

  return *this;
}

void ByteArray::reset() {
  if (m_allocation == AllocationType::Mapped && m_mapped_addr) {
    munmap(m_mapped_addr, m_mapped_len); // Use aligned address/length
  } else if (m_data) {
    Star::free(m_data, m_capacity);
  }

  m_data = nullptr;
  m_size = 0;
  m_capacity = 0;
  m_mapped_addr = nullptr;
  m_mapped_len = 0;
  m_allocation = AllocationType::Heap;
}

void ByteArray::reserve(size_t newCapacity) {
  if (m_allocation == AllocationType::Mapped) {
    // Disallow resizing of mmap-backed buffers
    throw StarException("Cannot resize mmap-backed ByteArray");
  }

  if (newCapacity > m_capacity) {
    if (!m_data) {
      auto newMem = (char*)Star::malloc(newCapacity);
      if (!newMem)
        throw MemoryException::format("Could not set new ByteArray capacity {}\n", newCapacity);
      m_data = newMem;
      m_capacity = newCapacity;
    } else {
      newCapacity = max({m_capacity * 2, newCapacity, (size_t)8});
      auto newMem = (char*)Star::realloc(m_data, newCapacity);
      if (!newMem)
        throw MemoryException::format("Could not set new ByteArray capacity {}\n", newCapacity);
      m_data = newMem;
      m_capacity = newCapacity;
    }
  }
}

void ByteArray::resize(size_t size, char f) {
  if (m_size == size)
    return;

  size_t oldSize = m_size;
  resize(size);
  for (size_t i = oldSize; i < m_size; ++i)
    (*this)[i] = f;
}

void ByteArray::fill(size_t s, char c) {
  if (s != NPos)
    resize(s);

  memset(m_data, c, m_size);
}

void ByteArray::fill(char c) {
  fill(NPos, c);
}

ByteArray ByteArray::sub(size_t b, size_t s) const {
  if (b == 0 && s >= m_size) {
    return ByteArray(*this);
  } else {
    return ByteArray(m_data + b, min(m_size, b + s));
  }
}

ByteArray ByteArray::left(size_t s) const {
  return sub(0, s);
}

ByteArray ByteArray::right(size_t s) const {
  if (s > m_size)
    s = 0;
  else
    s = m_size - s;

  return sub(s, m_size);
}

void ByteArray::trimLeft(size_t s) {
  if (s >= m_size) {
    clear();
  } else {
    std::memmove(m_data, m_data + s, m_size - s);
    resize(m_size - s);
  }
}

void ByteArray::trimRight(size_t s) {
  if (s >= m_size)
    clear();
  else
    resize(m_size - s);
}

size_t ByteArray::diffChar(const ByteArray& b) const {
  size_t s = min(m_size, b.size());
  char* ac = m_data;
  char* bc = b.m_data;
  size_t i;
  for (i = 0; i < s; ++i) {
    if (ac[i] != bc[i])
      break;
  }

  return i;
}

int ByteArray::compare(const ByteArray& b) const {
  if (m_size == 0 && b.m_size == 0)
    return 0;

  if (m_size == 0)
    return -1;

  if (b.m_size == 0)
    return 1;

  size_t d = diffChar(b);
  if (d == m_size) {
    if (d != b.m_size)
      return -1;
    else
      return 0;
  }

  if (d == b.m_size) {
    if (d != m_size)
      return 1;
    else
      return 0;
  }

  unsigned char c1 = (*this)[d];
  unsigned char c2 = b[d];

  if (c1 < c2) {
    return -1;
  } else if (c1 > c2) {
    return 1;
  } else {
    return 0;
  }
}

ByteArray ByteArray::andWith(ByteArray const& rhs, bool extend) {
  return combineWith([](char a, char b) { return a & b; }, rhs, extend);
}

ByteArray ByteArray::orWith(ByteArray const& rhs, bool extend) {
  return combineWith([](char a, char b) { return a | b; }, rhs, extend);
}

ByteArray ByteArray::xorWith(ByteArray const& rhs, bool extend) {
  return combineWith([](char a, char b) { return a ^ b; }, rhs, extend);
}

void ByteArray::insert(size_t pos, char byte) {
  starAssert(pos <= m_size);
  resize(m_size + 1);
  for (size_t i = m_size - 1; i > pos; --i)
    m_data[i] = m_data[i - 1];
  m_data[pos] = byte;
}

ByteArray::iterator ByteArray::insert(const_iterator pos, char byte) {
  size_t d = pos - begin();
  insert(d, byte);
  return begin() + d + 1;
}

void ByteArray::push_back(char byte) {
  resize(m_size + 1);
  m_data[m_size - 1] = byte;
}

bool ByteArray::operator<(const ByteArray& b) const {
  return compare(b) < 0;
}

bool ByteArray::operator==(const ByteArray& b) const {
  return compare(b) == 0;
}

bool ByteArray::operator!=(const ByteArray& b) const {
  return compare(b) != 0;
}

std::ostream& operator<<(std::ostream& os, const ByteArray& b) {
  os << "0x" << hexEncode(b);
  return os;
}

ByteArray ByteArray::fromMMap(const char* path, size_t offset, size_t length) {
  int fd = open(path, O_RDONLY);
  if (fd == -1)
    throw IOException::format("Failed to open '{}'", path);

  struct stat st;
  if (fstat(fd, &st) == -1) {
    close(fd);
    throw IOException::format("Failed to stat '{}'", path);
  }

  size_t file_size = st.st_size;
  if (offset > file_size) {
    close(fd);
    throw IOException::format("Offset {} exceeds file size {}", offset, file_size);
  }

  // Calculate page-aligned offset (required by mmap)
  size_t page_size = sysconf(_SC_PAGESIZE);
  size_t aligned_offset = offset - (offset % page_size);
  size_t offset_within_page = offset - aligned_offset;

  // Adjust length to fit within the file
  size_t max_length = file_size - offset;
  if (length == 0 || length > max_length)
    length = max_length;

  // Total mapped region length (aligned)
  size_t mapped_length = offset_within_page + length;
  if (mapped_length == 0) {
    close(fd);
    return ByteArray(); // Empty
  }

  // Map the aligned region
  void* mapped_addr = mmap(nullptr, mapped_length, PROT_READ, MAP_PRIVATE, fd, aligned_offset);
  close(fd);

  if (mapped_addr == MAP_FAILED)
    throw IOException::format("mmap failed for '{}'", path);

  // Construct ByteArray
  ByteArray ba;
  ba.m_mapped_addr = mapped_addr;
  ba.m_mapped_len = mapped_length;
  ba.m_data = static_cast<char*>(mapped_addr) + offset_within_page; // Adjusted pointer
  ba.m_size = length;
  ba.m_capacity = length; // Mapped capacity is fixed
  ba.m_allocation = AllocationType::Mapped;

  return ba;
}

}
