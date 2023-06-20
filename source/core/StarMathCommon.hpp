#ifndef STAR_MATH_COMMON_HPP
#define STAR_MATH_COMMON_HPP

#include <type_traits>
#include <limits>

#include "StarMaybe.hpp"

namespace Star {

STAR_EXCEPTION(MathException, StarException);

namespace Constants {
  double const pi = 3.14159265358979323846;
  double const rad2deg = 57.2957795130823208768;
  double const deg2rad = 1 / rad2deg;
  double const sqrt2 = 1.41421356237309504880;
  double const log2e = 1.44269504088896340736;
}

// Really common std namespace includes, and replacements for std libraries
// that don't provide them

using std::abs;
using std::fabs;
using std::sqrt;
using std::floor;
using std::ceil;
using std::round;
using std::fmod;
using std::sin;
using std::cos;
using std::tan;
using std::pow;
using std::atan2;
using std::log;
using std::log10;
using std::copysign;

inline float log2(float f) {
  return log(f) * (float)Constants::log2e;
}

inline double log2(double d) {
  return log(d) * Constants::log2e;
}

// Count the number of '1' bits in the given unsigned integer
template <typename Int>
typename std::enable_if<std::is_integral<Int>::value && std::is_unsigned<Int>::value, unsigned>::type countSetBits(Int value) {
  unsigned count = 0;
  while (value != 0) {
    value &= (value - 1);
    ++count;
  }
  return count;
}

template <typename T, typename T2>
typename std::enable_if<!std::numeric_limits<T>::is_integer && !std::numeric_limits<T2>::is_integer && sizeof(T) >= sizeof(T2), bool>::type
nearEqual(T x, T2 y, unsigned ulp) {
  auto epsilon = std::numeric_limits<T>::epsilon();
  return abs(x - y) <= epsilon * max(abs(x), (T)abs(y)) * ulp;
}

template <typename T, typename T2>
typename std::enable_if<!std::numeric_limits<T>::is_integer && !std::numeric_limits<T2>::is_integer && sizeof(T) < sizeof(T2), bool>::type
nearEqual(T x, T2 y, unsigned ulp) {
  return nearEqual(y, x, ulp);
}

template <typename T, typename T2>
typename std::enable_if<std::numeric_limits<T>::is_integer && !std::numeric_limits<T2>::is_integer, bool>::type
nearEqual(T x, T2 y, unsigned ulp) {
  return nearEqual((double)x, y, ulp);
}

template <typename T, typename T2>
typename std::enable_if<!std::numeric_limits<T>::is_integer && std::numeric_limits<T2>::is_integer, bool>::type
nearEqual(T x, T2 y, unsigned ulp) {
  return nearEqual(x, (double)y, ulp);
}

template <typename T, typename T2>
typename std::enable_if<std::numeric_limits<T>::is_integer && std::numeric_limits<T2>::is_integer, bool>::type
nearEqual(T x, T2 y, unsigned) {
  return x == y;
}

template <typename T, typename T2>
bool nearEqual(T x, T2 y) {
  return nearEqual(x, y, 1);
}

template <typename T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type nearZero(T x, unsigned ulp = 2) {
  return abs(x) <= std::numeric_limits<T>::min() * ulp;
}

template <typename T>
typename std::enable_if<std::numeric_limits<T>::is_integer, bool>::type nearZero(T x) {
  return x == 0;
}

template <typename T>
constexpr T lowest() {
  return std::numeric_limits<T>::lowest();
}

template <typename T>
constexpr T highest() {
  return std::numeric_limits<T>::max();
}

template <typename T>
constexpr T square(T const& x) {
  return x * x;
}

template <typename T>
constexpr T cube(T const& x) {
  return x * x * x;
}

template <typename Float>
int ipart(Float f) {
  return (int)floor(f);
}

template <typename Float>
Float fpart(Float f) {
  return f - ipart(f);
}

template <typename Float>
Float rfpart(Float f) {
  return 1.0 - fpart(f);
}

template <typename T, typename T2>
T clampMagnitude(T const& v, T2 const& mag) {
  if (v > mag)
    return mag;
  else if (v < -mag)
    return -mag;
  else
    return v;
}

template <typename T>
T clamp(T const val, T const min, T const max) {
  return std::min(std::max(val, min), max);
}

template <typename T>
T clampDynamic(T const val, T const a, T const b) {
  return std::min(std::max(val, std::min(a, b)), std::max(a, b));
}

template <typename IntType, typename PowType>
IntType intPow(IntType i, PowType p) {
  starAssert(p >= 0);

  if (p == 0)
    return 1;
  if (p == 1)
    return i;

  IntType tmp = intPow(i, p / 2);
  if ((p % 2) == 0)
    return tmp * tmp;
  else
    return i * tmp * tmp;
}

template <typename Int>
bool isPowerOf2(Int x) {
  if (x < 1)
    return false;
  return (x & (x - 1)) == 0;
}

inline uint64_t ceilPowerOf2(uint64_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v++;
  return v;
}

template <typename Float>
Float sigmoid(Float x) {
  return 1 / (1 + std::exp(-x));
}

// returns a % m such that the answer is always positive.
// For example, -1 mod 10 is 9.
template <typename IntType>
IntType pmod(IntType a, IntType m) {
  IntType r = a % m;
  return r < 0 ? r + m : r;
}

// Same as pmod but for float like values.
template <typename Float>
Float pfmod(Float a, Float m) {
  if (m == 0)
    return a;

  return a - m * floor(a / m);
}

// Finds the *smallest* distance (in absolute value terms) from b to a (a - b)
// in a non-euclidean wrapping number line.  Suppose size is 100, wrapDiff(10,
// 109) would return 1, because 509 is congruent to the point 9.  On the other
// hand, wrapDiff(10, 111) would return -1, because 111 is congruent to the
// point 11.
template <typename Type>
Type wrapDiff(Type a, Type b, Type size) {
  a = pmod(a, size);
  b = pmod(b, size);

  Type diff = a - b;
  if (diff > size / 2)
    diff -= size;
  else if (diff < -size / 2)
    diff += size;

  return diff;
}

// Sampe as wrapDiff but for float like values
template <typename Type>
Type wrapDiffF(Type a, Type b, Type size) {
  a = pfmod(a, size);
  b = pfmod(b, size);

  Type diff = a - b;
  if (diff > size / 2)
    diff -= size;
  else if (diff < -size / 2)
    diff += size;

  return diff;
}

// like std::pow, except ignores sign, and the return value will match the sign
// of the value passed in.  ppow(-2, 2) == -4
template <typename Float>
Float ppow(Float val, Float pow) {
  return copysign(std::pow(std::fabs(val), pow), val);
}

// Returns angle wrapped around to the range [-pi, pi).
template <typename Float>
Float constrainAngle(Float angle) {
  angle = fmod((Float)(angle + Constants::pi), (Float)(Constants::pi * 2));
  if (angle < 0)
    angle += Constants::pi * 2;
  return angle - Constants::pi;
}

// Returns the closest angle movement to go from the given angle to the target
// angle, in radians.
template <typename Float>
Float angleDiff(Float angle, Float targetAngle) {
  double diff = fmod((Float)(targetAngle - angle + Constants::pi), (Float)(Constants::pi * 2));
  if (diff < 0)
    diff += Constants::pi * 2;
  return diff - Constants::pi;
}

// Approach the given goal value from the current value, at a maximum rate of
// change.  Rate should always be a positive value. (T must be signed).
template <typename T>
T approach(T goal, T current, T rate) {
  if (goal < current) {
    return max(current - rate, goal);
  } else if (goal > current) {
    return min(current + rate, goal);
  } else {
    return current;
  }
}

// Same as approach, specialied for angles, and always approaches from the
// closest absolute direction.
template <typename T>
T approachAngle(T goal, T current, T rate) {
  return constrainAngle(current + clampMagnitude<T>(angleDiff(current, goal), rate));
}

// Used in color conversion from floating point to uint8_t
inline uint8_t floatToByte(float val, bool doClamp = false) {
  if (doClamp)
    val = clamp(val, 0.0f, 1.0f);
  return (uint8_t)(val * 255.0f);
}

// Used in color conversion from uint8_t to normalized float.
inline float byteToFloat(uint8_t val) {
  return val / 255.0f;
}

// Turn a randomized floating point value from [0.0, 1.0] to [-1.0, 1.0]
template <typename Float>
Float randn(Float val) {
  return val * 2 - 1;
}

// Increments a value between min and max inclusive, cycling around to min when
// it would be incremented beyond max.  If the value is outside of the range,
// the next increment will start at min.
template <typename Integer>
Integer cycleIncrement(Integer val, Integer min, Integer max) {
  if (val < min || val >= max)
    return min;
  else
    return val + 1;
}

}

#endif
