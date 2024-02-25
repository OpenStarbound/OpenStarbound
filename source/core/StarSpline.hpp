#pragma once

#include "StarVector.hpp"
#include "StarInterpolation.hpp"
#include "StarLogging.hpp"
#include "StarLruCache.hpp"

namespace Star {

// Implementation of DeCasteljau Algorithm for Bezier Curves
template <typename DataT, size_t Dimension, size_t Order, class PointT = Vector<DataT, Dimension>>
class Spline : public Array<PointT, Order + 1> {
public:
  typedef Array<PointT, Order + 1> PointData;

  template <typename... T>
  Spline(PointT const& e1, T const&... rest)
    : PointData(e1, rest...) {
    m_pointCache.setMaxSize(1000);
    m_lengthCache.setMaxSize(1000);
  }

  Spline() : PointData(PointData::filled(PointT())) {
    m_pointCache.setMaxSize(1000);
    m_lengthCache.setMaxSize(1000);
  }

  PointT pointAt(float t) const {
    float u = clamp<float>(t, 0, 1);
    if (u != t) {
      t = u;
      Logger::warn("Passed out of range time to Spline::pointAt");
    }

    if (auto p = m_pointCache.ptr(t))
      return *p;

    PointData intermediates(*this);
    PointData temp;
    for (size_t order = Order + 1; order > 1; order--) {
      for (size_t i = 1; i < order; i++) {
        temp[i - 1] = lerp(t, intermediates[i - 1], intermediates[i]);
      }
      intermediates = std::move(temp);
    }

    m_pointCache.set(t, intermediates[0]);
    return intermediates[0];
  }

  PointT tangentAt(float t) const {
    float u = clamp<float>(t, 0, 1);
    if (u != t) {
      t = u;
      Logger::warn("Passed out of range time to Spline::tangentAt");
    }

    // constructs a hodograph and returns pointAt
    Spline<DataT, Dimension, Order - 1> hodograph;
    for (size_t i = 0; i < Order; i++) {
      hodograph[i] = ((*this)[i + 1] - (*this)[i]) * Order;
    }
    return hodograph.pointAt(t);
  }

  DataT length(float begin = 0, float end = 1, size_t subdivisions = 100) const {
    if (!(begin <= 1 && begin >= 0 && end <= 1 && end >= 0 && begin <= end)) {
      Logger::warn("Passed invalid range to Spline::length");
      return 0;
    }

    if (!begin) {
      if (auto p = m_lengthCache.ptr(end))
        return *p;
    }

    DataT res = 0;
    PointT previousPoint = pointAt(begin);
    for (size_t i = 1; i <= subdivisions; i++) {
      PointT currentPoint = pointAt(i / subdivisions * (end - begin));
      res += (currentPoint - previousPoint).magnitude();
      previousPoint = currentPoint;
    }

    if (!begin)
      m_lengthCache.set(end, res);

    return res;
  }

  float arcLenPara(float u, DataT epsilon = .01) const {
    if (u == 0)
      return 0;
    if (u == 1)
      return 1;
    u = clamp<float>(u, 0, 1);
    if (u == 0 || u == 1) {
      Logger::warn("Passed out of range time to Spline::arcLenPara");
      return u;
    }
    DataT targetLength = length() * u;
    float t = .5;
    float lower = 0;
    float upper = 1;
    DataT approxLen = length(0, t);
    while (targetLength - approxLen > epsilon || targetLength - approxLen < -epsilon) {
      if (targetLength > approxLen) {
        lower = t;
      } else {
        upper = t;
      }
      t = (upper - lower) * .5 + lower;
      approxLen = length(0, t);
    }
    return t;
  }

  PointT& origin() {
    m_pointCache.clear();
    m_lengthCache.clear();
    return (*this)[0];
  }

  PointT const& origin() const {
    return (*this)[0];
  }

  PointT& dest() {
    m_pointCache.clear();
    m_lengthCache.clear();
    return (*this)[Order];
  }

  PointT const& dest() const {
    return (*this)[Order];
  }

  PointT& operator[](size_t index) {
    m_pointCache.clear();
    m_lengthCache.clear();
    return PointData::operator[](index);
  }

  PointT const& operator[](size_t index) const {
    return PointData::operator[](index);
  }

protected:
  mutable LruCache<float, PointT> m_pointCache;
  mutable LruCache<float, DataT> m_lengthCache;
};

typedef Spline<float, 2, 3, Vec2F> CSplineF;

}
