#ifndef STAR_STATIC_RANDOM_HPP
#define STAR_STATIC_RANDOM_HPP

#include "StarString.hpp"
#include "StarXXHash.hpp"

namespace Star {

// Cross-platform, predictable random number generators based on XXHash.
// Supports primitive types as well as strings for input data.

inline void staticRandomHash32Iter(XXHash32&) {}

template <typename T, typename... TL>
void staticRandomHash32Iter(XXHash32& hash, T const& v, TL const&... rest) {
  xxHash32Push(hash, v);
  staticRandomHash32Iter(hash, rest...);
}

template <typename T, typename... TL>
uint32_t staticRandomHash32(T const& v, TL const&... rest) {
  XXHash32 hash(2938728349u);
  staticRandomHash32Iter(hash, v, rest...);
  return hash.digest();
}

inline void staticRandomHash64Iter(XXHash64&) {}

template <typename T, typename... TL>
void staticRandomHash64Iter(XXHash64& hash, T const& v, TL const&... rest) {
  xxHash64Push(hash, v);
  staticRandomHash64Iter(hash, rest...);
}

template <typename T, typename... TL>
uint64_t staticRandomHash64(T const& v, TL const&... rest) {
  XXHash64 hash(1997293021376312589);
  staticRandomHash64Iter(hash, v, rest...);
  return hash.digest();
}

template <typename T, typename... TL>
uint32_t staticRandomU32(T const& d, TL const&... rest) {
  return staticRandomHash32(d, rest...);
}

template <typename T, typename... TL>
uint64_t staticRandomU64(T const& d, TL const&... rest) {
  return staticRandomHash64(d, rest...);
}

template <typename T, typename... TL>
int32_t staticRandomI32(T const& d, TL const&... rest) {
  return (int32_t)staticRandomU32(d, rest...);
}

template <typename T, typename... TL>
int32_t staticRandomI32Range(int32_t min, int32_t max, T const& d, TL const&... rest) {
  uint64_t denom = (uint64_t)(-1) / ((uint64_t)(max - min) + 1);
  return (int32_t)(staticRandomU64(d, rest...) / denom + min);
}

template <typename T, typename... TL>
uint32_t staticRandomU32Range(uint32_t min, uint32_t max, T const& d, TL const&... rest) {
  uint64_t denom = (uint64_t)(-1) / ((uint64_t)(max - min) + 1);
  return staticRandomU64(d, rest...) / denom + min;
}

template <typename T, typename... TL>
int64_t staticRandomI64(T const& d, TL const&... rest) {
  return (int64_t)staticRandomU64(d, rest...);
}

// Generates values in the range [0.0, 1.0]
template <typename T, typename... TL>
float staticRandomFloat(T const& d, TL const&... rest) {
  return (staticRandomU32(d, rest...) & 0x7fffffff) / 2147483648.0;
}

template <typename T, typename... TL>
float staticRandomFloatRange(float min, float max, T const& d, TL const&... rest) {
  return staticRandomFloat(d, rest...) * (max - min) + min;
}

// Generates values in the range [0.0, 1.0]
template <typename T, typename... TL>
double staticRandomDouble(T const& d, TL const&... rest) {
  return (staticRandomU64(d, rest...) & 0x7fffffffffffffff) / 9223372036854775808.0;
}

template <typename T, typename... TL>
double staticRandomDoubleRange(double min, double max, T const& d, TL const&... rest) {
  return staticRandomDouble(d, rest...) * (max - min) + min;
}

template <typename Container, typename T, typename... TL>
typename Container::value_type& staticRandomFrom(Container& container, T const& d, TL const&... rest) {
  auto i = container.begin();
  std::advance(i, staticRandomI32Range(0, container.size() - 1, d, rest...));
  return *i;
}

template <typename Container, typename T, typename... TL>
typename Container::value_type const& staticRandomFrom(Container const& container, T const& d, TL const&... rest) {
  auto i = container.begin();
  std::advance(i, staticRandomI32Range(0, container.size() - 1, d, rest...));
  return *i;
}

template <typename Container, typename T, typename... TL>
typename Container::value_type staticRandomValueFrom(Container const& container, T const& d, TL const&... rest) {
  if (container.empty()) {
    return {};
  } else {
    auto i = container.begin();
    std::advance(i, staticRandomI32Range(0, container.size() - 1, d, rest...));
    return *i;
  }
}

template <typename T>
class URBG {
public:
  typedef function <T()> Function;

  URBG(Function func) : m_func(func) {};

  typedef T result_type;
  static constexpr T min() { return std::numeric_limits<T>::min(); };
  static constexpr T max() { return std::numeric_limits<T>::max(); };
  T operator()() { return m_func(); };
private:
  Function m_func;
};

template <typename Container, typename T, typename... TL>
void staticRandomShuffle(Container& container, T const& d, TL const&... rest) {
  int mix = 0;
  size_t max = container.size();
  std::shuffle(container.begin(), container.end(), URBG<size_t>([&]() {
    return staticRandomU32Range(0, max - 1, ++mix, d, rest...);
  }));
}

}

#endif
