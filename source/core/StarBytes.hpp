#ifndef STAR_BYTES_HPP
#define STAR_BYTES_HPP

#include "StarMemory.hpp"

namespace Star {

enum class ByteOrder {
  BigEndian,
  LittleEndian,
  NoConversion
};

ByteOrder platformByteOrder();

void swapByteOrder(void* ptr, size_t len);
void swapByteOrder(void* dest, void const* src, size_t len);

void toByteOrder(ByteOrder order, void* ptr, size_t len);
void toByteOrder(ByteOrder order, void* dest, void const* src, size_t len);
void fromByteOrder(ByteOrder order, void* ptr, size_t len);
void fromByteOrder(ByteOrder order, void* dest, void const* src, size_t len);

template <typename T>
T toByteOrder(ByteOrder order, T const& t) {
  T ret;
  toByteOrder(order, &ret, &t, sizeof(t));
  return ret;
}

template <typename T>
T fromByteOrder(ByteOrder order, T const& t) {
  T ret;
  fromByteOrder(order, &ret, &t, sizeof(t));
  return ret;
}

template <typename T>
T toBigEndian(T const& t) {
  return toByteOrder(ByteOrder::BigEndian, t);
}

template <typename T>
T fromBigEndian(T const& t) {
  return fromByteOrder(ByteOrder::BigEndian, t);
}

template <typename T>
T toLittleEndian(T const& t) {
  return toByteOrder(ByteOrder::LittleEndian, t);
}

template <typename T>
T fromLittleEndian(T const& t) {
  return fromByteOrder(ByteOrder::LittleEndian, t);
}

inline ByteOrder platformByteOrder() {
#if STAR_LITTLE_ENDIAN
  return ByteOrder::LittleEndian;
#else
  return ByteOrder::BigEndian;
#endif
}

inline void swapByteOrder(void* ptr, size_t len) {
  uint8_t* data = static_cast<uint8_t*>(ptr);
  uint8_t spare;
  for (size_t i = 0; i < len / 2; ++i) {
    spare = data[len - 1 - i];
    data[len - 1 - i] = data[i];
    data[i] = spare;
  }
}

inline void swapByteOrder(void* dest, const void* src, size_t len) {
  const uint8_t* srcdata = static_cast<const uint8_t*>(src);
  uint8_t* destdata = static_cast<uint8_t*>(dest);
  for (size_t i = 0; i < len; ++i)
    destdata[len - 1 - i] = srcdata[i];
}

inline void toByteOrder(ByteOrder order, void* ptr, size_t len) {
  if (order != ByteOrder::NoConversion && platformByteOrder() != order)
    swapByteOrder(ptr, len);
}

inline void toByteOrder(ByteOrder order, void* dest, void const* src, size_t len) {
  if (order != ByteOrder::NoConversion && platformByteOrder() != order)
    swapByteOrder(dest, src, len);
  else
    memcpy(dest, src, len);
}

inline void fromByteOrder(ByteOrder order, void* ptr, size_t len) {
  if (order != ByteOrder::NoConversion && platformByteOrder() != order)
    swapByteOrder(ptr, len);
}

inline void fromByteOrder(ByteOrder order, void* dest, void const* src, size_t len) {
  if (order != ByteOrder::NoConversion && platformByteOrder() != order)
    swapByteOrder(dest, src, len);
  else
    memcpy(dest, src, len);
}

}

#endif
