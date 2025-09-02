#pragma once

#include <numeric>
#include <type_traits>

#include "StarRect.hpp"

namespace Star {

template <typename T>
class Polygon {
public:
  template<typename S>
  using VertexT = Vector<S, 2>;
  using Vertex = VertexT<T>;

  using Line = Star::Line<T, 2>;
  using Rect = Star::Box<T, 2>;

  struct IntersectResult {
    // Whether or not the two objects intersect
    bool intersects;
    // How much *this* poly must be moved in order to make them not intersect
    // anymore
    Vertex overlap;
  };

  struct LineIntersectResult {
    // Point of intersection
    Vertex point;
    // t value at the point of intersection of the line that was checked
    T along;
    // Side that the line first intersected, if the line starts inside the
    // polygon, this will not be set.
    Maybe<size_t> intersectedSide;
  };

  typedef List<Vertex> VertexList;
  typedef typename VertexList::iterator iterator;
  typedef typename VertexList::const_iterator const_iterator;

  static Polygon convexHull(VertexList points);
  static Polygon clip(Polygon inputPoly, Polygon convexClipPoly);

  // Creates a null polygon
  Polygon();
  Polygon(Polygon const& rhs);
  Polygon(Polygon&& rhs);

  template <typename T2>
  explicit Polygon(Box<T2, 2> const& rect);

  template <typename T2>
  explicit Polygon(Polygon<T2> const& p2);

  // This seems weird, but it isn't.  SAT intersection works perfectly well
  // with one Poly having only a single vertex.
  explicit Polygon(Vertex const& coord);

  // When specifying a polygon using this constructor the list should be in
  // counterclockwise order.
  explicit Polygon(VertexList const& vertexes);

  Polygon(std::initializer_list<Vertex> vertexes);

  bool isNull() const;

  bool isConvex() const;
  float convexArea() const;

  void deduplicateVertexes(float maxDistance);

  void add(Vertex const& a);
  void remove(size_t i);

  void clear();

  VertexList const& vertexes() const;
  VertexList& vertexes();

  size_t sides() const;

  Line side(size_t i) const;

  T distance(Vertex const& c) const;

  void translate(Vertex const& c);

  void setCenter(Vertex const& c);

  void rotate(T a, Vertex const& c = Vertex());

  template<typename S>
  void scale(VertexT<S> const& s, VertexT<S> const& c = Vertex());
  template<typename S>
  void scale(S s, VertexT<S> const& c = Vertex());

  void flipHorizontal(T horizontalPos = T());
  void flipVertical(T verticalPos = T());

  template <typename T2>
  void transform(Matrix3<T2> const& transMat);

  Vertex const& operator[](size_t i) const;
  Vertex& operator[](size_t i);

  bool operator==(Polygon const& rhs) const;

  Polygon& operator=(Polygon const& rhs);
  Polygon& operator=(Polygon&& rhs);

  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

  // vertex and normal wrap around so that i can never be out of range.
  Vertex const& vertex(size_t i) const;
  Vertex normal(size_t i) const;

  Vertex center() const;

  // a point in the volume, within min and max y, moved downwards to be a half
  // width from the bottom (if that point is within a half width from the
  // top, center() is returned)
  Vertex bottomCenter() const;

  Rect boundBox() const;

  // Determine winding number of the given point.
  int windingNumber(Vertex const& p) const;

  bool contains(Vertex const& p) const;

  // Normal SAT intersection finding the shortest separation of two convex
  // polys.
  IntersectResult satIntersection(Polygon const& p) const;

  // A directional version of a SAT intersection that will only separate
  // parallel to the given direction.  If choseSign is true, then the
  // separation can occur either with the given direction or opposite it, but
  // still parallel.  If it is false, separation will always occur in the given
  // direction only.
  IntersectResult directionalSatIntersection(Polygon const& p, Vertex const& direction, bool chooseSign) const;

  // Returns the closest intersection with the poly, if any.
  Maybe<LineIntersectResult> lineIntersection(Line const& l) const;

  bool intersects(Polygon const& p) const;
  bool intersects(Line const& l) const;

private:
  // i must be between 0 and m_vertexes.size() - 1
  Line sideAt(size_t i) const;

  VertexList m_vertexes;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, Polygon<T> const& poly);

typedef Polygon<int> PolyI;
typedef Polygon<float> PolyF;
typedef Polygon<double> PolyD;

template <typename T>
Polygon<T> Polygon<T>::convexHull(VertexList points) {
  if (points.empty())
    return {};

  auto cross = [](Vertex o, Vertex a, Vertex b) {
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
  };
  sort(points);

  VertexList lower;
  for (auto const& point : points) {
    while (lower.size() >= 2 && cross(lower[lower.size() - 2], lower[lower.size() - 1], point) <= 0)
      lower.removeLast();
    lower.append(point);
  }

  VertexList upper;
  for (auto const& point : reverseIterate(points)) {
    while (upper.size() >= 2 && cross(upper[upper.size() - 2], upper[upper.size() - 1], point) <= 0)
      upper.removeLast();
    upper.append(point);
  }

  upper.removeLast();
  lower.removeLast();
  lower.appendAll(take(upper));
  return Polygon<T>(std::move(lower));
}

template <typename T>
Polygon<T> Polygon<T>::clip(Polygon inputPoly, Polygon convexClipPoly) {
  if (inputPoly.sides() == 0)
    return inputPoly;

  auto insideEdge = [](Line const& edge, Vertex const& p) {
    return ((edge.max() - edge.min()) ^ (p - edge.min())) > 0;
  };

  VertexList outputVertexes = take(inputPoly.m_vertexes);
  for (size_t i = 0; i < convexClipPoly.sides(); ++i) {
    if (outputVertexes.empty())
      break;

    Line clipEdge = convexClipPoly.sideAt(i);
    VertexList inputVertexes = take(outputVertexes);
    Vertex s = inputVertexes.last();
    for (Vertex e : inputVertexes) {
      if (insideEdge(clipEdge, e)) {
        if (!insideEdge(clipEdge, s))
          outputVertexes.append(clipEdge.intersection(Line(s, e)).point);
        outputVertexes.append(e);
      } else if (insideEdge(clipEdge, s)) {
        outputVertexes.append(clipEdge.intersection(Line(s, e)).point);
      }
      s = e;
    }
  }

  return Polygon(std::move(outputVertexes));
}

template <typename T>
Polygon<T>::Polygon() {}

template <typename T>
Polygon<T>::Polygon(Polygon const& rhs)
  : m_vertexes(rhs.m_vertexes) {}

template <typename T>
Polygon<T>::Polygon(Polygon&& rhs)
  : m_vertexes(std::move(rhs.m_vertexes)) {}

template <typename T>
template <typename T2>
Polygon<T>::Polygon(Box<T2, 2> const& rect) {
  m_vertexes = {
      Vertex(rect.min()), Vertex(rect.max()[0], rect.min()[1]), Vertex(rect.max()), Vertex(rect.min()[0], rect.max()[1])};
}

template <typename T>
template <typename T2>
Polygon<T>::Polygon(Polygon<T2> const& p2) {
  for (auto const& v : p2)
    m_vertexes.push_back(Vertex(v));
}

template <typename T>
Polygon<T>::Polygon(Vertex const& coord) {
  m_vertexes.push_back(coord);
}

template <typename T>
Polygon<T>::Polygon(VertexList const& vertexes)
  : m_vertexes(vertexes) {}

template <typename T>
Polygon<T>::Polygon(std::initializer_list<Vertex> vertexes)
  : m_vertexes(vertexes) {}

template <typename T>
bool Polygon<T>::isNull() const {
  return m_vertexes.empty();
}

template <typename T>
bool Polygon<T>::isConvex() const {
  if (sides() < 2)
    return true;

  for (unsigned i = 0; i < sides(); ++i) {
    if ((side(i + 1).diff() ^ side(i).diff()) > 0)
      return false;
  }

  return true;
}

template <typename T>
float Polygon<T>::convexArea() const {
  float area = 0.0f;
  for (size_t i = 0; i < m_vertexes.size(); ++i) {
    Vertex const& v1 = m_vertexes[i];
    Vertex const& v2 = i == m_vertexes.size() - 1 ? m_vertexes[0] : m_vertexes[i + 1];
    area += 0.5f * (v1[0] * v2[1] - v1[1] * v2[0]);
  }
  return area;
}

template <typename T>
void Polygon<T>::deduplicateVertexes(float maxDistance) {
  if (m_vertexes.empty())
    return;

  float distSquared = square(maxDistance);
  VertexList newVertexes = {m_vertexes[0]};
  for (size_t i = 1; i < m_vertexes.size(); ++i) {
    if (vmagSquared(m_vertexes[i] - newVertexes.last()) > distSquared)
      newVertexes.append(m_vertexes[i]);
  }

  if (vmagSquared(newVertexes.first() - newVertexes.last()) <= distSquared)
    newVertexes.removeLast();

  m_vertexes = std::move(newVertexes);
}

template <typename T>
void Polygon<T>::add(Vertex const& a) {
  m_vertexes.push_back(a);
}

template <typename T>
void Polygon<T>::remove(size_t i) {
  auto it = begin() + i % sides();
  m_vertexes.erase(it);
}

template <typename T>
void Polygon<T>::clear() {
  m_vertexes.clear();
}

template <typename T>
typename Polygon<T>::VertexList const& Polygon<T>::vertexes() const {
  return m_vertexes;
}

template <typename T>
typename Polygon<T>::VertexList& Polygon<T>::vertexes() {
  return m_vertexes;
}

template <typename T>
size_t Polygon<T>::sides() const {
  return m_vertexes.size();
}

template <typename T>
typename Polygon<T>::Line Polygon<T>::side(size_t i) const {
  return sideAt(i % m_vertexes.size());
}

template <typename T>
T Polygon<T>::distance(Vertex const& c) const {
  if (contains(c))
    return 0;

  T dist = highest<T>();
  for (size_t i = 0; i < m_vertexes.size(); ++i)
    dist = min(dist, sideAt(i).distanceTo(c));

  return dist;
}

template <typename T>
void Polygon<T>::translate(Vertex const& c) {
  for (auto& v : m_vertexes)
    v += c;
}

template <typename T>
void Polygon<T>::setCenter(Vertex const& c) {
  translate(c - center());
}

template <typename T>
void Polygon<T>::rotate(T a, Vertex const& c) {
  for (auto& v : m_vertexes)
    v = (v - c).rotate(a) + c;
}

template <typename T>
template <typename U>
void Polygon<T>::scale(VertexT<U> const& s, VertexT<U> const& c) {
  using V = VertexT<std::common_type_t<T, U>>;
  for (auto& v : m_vertexes) {
    v = Vertex(vmult(V(v) - V(c), V(s)) + V(c));
  }
}

template <typename T>
template <typename U>
void Polygon<T>::scale(U s, VertexT<U> const& c) {
  scale(VertexT<U>::filled(s), c);
}

template <typename T>
void Polygon<T>::flipHorizontal(T horizontalPos) {
  scale(Vertex(-1, 1), Vertex(horizontalPos, 0));
  // Reverse vertexes to make sure poly remains counter-clockwise after flip.
  std::reverse(m_vertexes.begin(), m_vertexes.end());
}

template <typename T>
void Polygon<T>::flipVertical(T verticalPos) {
  scale(Vertex(1, -1), Vertex(0, verticalPos));
  // Reverse vertexes to make sure poly remains counter-clockwise after flip.
  std::reverse(m_vertexes.begin(), m_vertexes.end());
}

template <typename T>
template <typename T2>
void Polygon<T>::transform(Matrix3<T2> const& transMat) {
  for (auto& v : m_vertexes)
    v = transMat.transformVec2(v);
}

template <typename T>
typename Polygon<T>::Vertex const& Polygon<T>::operator[](size_t i) const {
  return m_vertexes[i];
}

template <typename T>
typename Polygon<T>::Vertex& Polygon<T>::operator[](size_t i) {
  return m_vertexes[i];
}

template <typename T>
bool Polygon<T>::operator==(Polygon<T> const& rhs) const {
  return m_vertexes == rhs.m_vertexes;
}

template <typename T>
Polygon<T>& Polygon<T>::operator=(Polygon const& rhs) {
  m_vertexes = rhs.m_vertexes;
  return *this;
}

template <typename T>
Polygon<T>& Polygon<T>::operator=(Polygon&& rhs) {
  m_vertexes = std::move(rhs.m_vertexes);
  return *this;
}

template <typename T>
typename Polygon<T>::iterator Polygon<T>::begin() {
  return m_vertexes.begin();
}

template <typename T>
typename Polygon<T>::const_iterator Polygon<T>::begin() const {
  return m_vertexes.begin();
}

template <typename T>
typename Polygon<T>::iterator Polygon<T>::end() {
  return m_vertexes.end();
}

template <typename T>
typename Polygon<T>::const_iterator Polygon<T>::end() const {
  return m_vertexes.end();
}

template <typename T>
typename Polygon<T>::Vertex const& Polygon<T>::vertex(size_t i) const {
  return m_vertexes[i % m_vertexes.size()];
}

template <typename T>
typename Polygon<T>::Vertex Polygon<T>::normal(size_t i) const {
  Vertex diff = side(i).diff();

  if (diff == Vertex())
    return Vertex();

  return diff.rot90().normalized();
}

template <typename T>
typename Polygon<T>::Vertex Polygon<T>::center() const {
  return std::accumulate(m_vertexes.begin(), m_vertexes.end(), Vertex()) / (T)m_vertexes.size();
}

template <typename T>
typename Polygon<T>::Vertex Polygon<T>::bottomCenter() const {
  if (m_vertexes.size() == 0)
    return Vertex();
  Polygon<T>::Vertex center = std::accumulate(m_vertexes.begin(), m_vertexes.end(), Vertex()) / (T)m_vertexes.size();
  Polygon<T>::Vertex bottomLeft = *std::min_element(m_vertexes.begin(), m_vertexes.end());
  Polygon<T>::Vertex topRight = *std::max_element(m_vertexes.begin(), m_vertexes.end());
  Polygon<T>::Vertex size = topRight - bottomLeft;
  if (size.x() > size.y())
    return center;
  return Polygon<T>::Vertex(center.x(), bottomLeft.y() + size.x() / 2.0f);
}

template <typename T>
auto Polygon<T>::boundBox() const -> Rect {
  auto bounds = Rect::null();
  for (auto const& v : m_vertexes)
    bounds.combine(v);
  return bounds;
}

template <typename T>
int Polygon<T>::windingNumber(Vertex const& p) const {

  auto isLeft = [](Vertex const& p0, Vertex const& p1, Vertex const& p2) {
    return ((p1[0] - p0[0]) * (p2[1] - p0[1]) - (p2[0] - p0[0]) * (p1[1] - p0[1]));
  };

  // the winding number counter
  int wn = 0;

  // loop through all edges of the polygon
  for (size_t i = 0; i < m_vertexes.size(); ++i) {
    auto const& first = m_vertexes[i];
    auto const& second = i == m_vertexes.size() - 1 ? m_vertexes[0] : m_vertexes[i + 1];

    // start y <= p[1]
    if (first[1] <= p[1]) {
      if (second[1] > p[1]) {
        // an upward crossing
        if (isLeft(first, second, p) > 0) {
          // p left of edge
          // have a valid up intersect
          ++wn;
        }
      }
    } else {
      // start y > p[1] (no test needed)
      if (second[1] <= p[1]) {
        // a downward crossing
        if (isLeft(first, second, p) < 0) {
          // p right of edge
          // have a valid down intersect
          --wn;
        }
      }
    }
  }

  return wn;
}

template <typename T>
bool Polygon<T>::contains(Vertex const& p) const {
  return windingNumber(p) != 0;
}

template <typename T>
typename Polygon<T>::IntersectResult Polygon<T>::satIntersection(Polygon const& p) const {
  // "Accumulates" the shortest separating distance and axis of this poly and
  // the given poly, after projecting all the vertexes of each poly onto a
  // given axis.  Used by SAT intersection, meant to be called with each tested
  // axis.
  auto accumSeparator = [this](Polygon const& p, Vertex const& axis, T& shortestOverlap, Vertex& finalSepDir) {
    T myProjectionLow = std::numeric_limits<T>::max();
    T targetProjectionHigh = std::numeric_limits<T>::lowest();

    for (auto const& v : m_vertexes) {
      T p = axis[0] * v[0] + axis[1] * v[1];
      if (p < myProjectionLow)
        myProjectionLow = p;
    }

    for (auto const& v : p.m_vertexes) {
      T p = axis[0] * v[0] + axis[1] * v[1];
      if (p > targetProjectionHigh)
        targetProjectionHigh = p;
    }

    float overlap = targetProjectionHigh - myProjectionLow;
    if (overlap < shortestOverlap) {
      shortestOverlap = overlap;
      finalSepDir = axis;
    }
  };

  T overlap = std::numeric_limits<T>::max();
  Vertex separatingDir = Vertex();

  if (!m_vertexes.empty()) {
    Vertex pv = m_vertexes[m_vertexes.size() - 1];
    for (auto const& v : m_vertexes) {
      Vertex sideNormal = pv - v;
      if (sideNormal != Vertex()) {
        sideNormal = sideNormal.rot90().normalized();
        accumSeparator(p, -sideNormal, overlap, separatingDir);
      }
      pv = v;
    }
  }

  if (!p.m_vertexes.empty()) {
    Vertex pv = p.m_vertexes[p.m_vertexes.size() - 1];
    for (auto const& v : p.m_vertexes) {
      Vertex sideNormal = pv - v;
      if (sideNormal != Vertex()) {
        sideNormal = sideNormal.rot90().normalized();
        accumSeparator(p, sideNormal, overlap, separatingDir);
      }
      pv = v;
    }
  }

  IntersectResult isect;
  isect.intersects = (overlap > 0);
  isect.overlap = separatingDir * overlap;

  return isect;
}

template <typename T>
typename Polygon<T>::IntersectResult Polygon<T>::directionalSatIntersection(
    Polygon const& p, Vertex const& direction, bool chooseSign) const {
  // A "directional" version of accumSeparator, that when intersecting only
  // ever tries to separate in the given direction.
  auto directionalAccumSeparator = [this](Polygon const& p, Vertex axis, T& shortestOverlap,
      Vertex const& separatingDir, Vertex& finalSepDir, bool chooseDir) {
    T myProjectionLow = std::numeric_limits<T>::max();
    T targetProjectionHigh = std::numeric_limits<T>::lowest();

    for (auto const& v : m_vertexes) {
      T p = axis[0] * v[0] + axis[1] * v[1];
      if (p < myProjectionLow)
        myProjectionLow = p;
    }

    for (auto const& v : p.m_vertexes) {
      T p = axis[0] * v[0] + axis[1] * v[1];
      if (p > targetProjectionHigh)
        targetProjectionHigh = p;
    }

    float overlap = targetProjectionHigh - myProjectionLow;

    // Separation was found, skip the rest of the method.
    if (overlap <= 0) {
      if (overlap < shortestOverlap) {
        shortestOverlap = overlap;
        finalSepDir = axis;
      }
      return;
    }

    T axisDot = separatingDir * axis;

    // Now, if we don't have separation and the axis is perpendicular to
    // requested, we can do nothing, return.
    if (axisDot == 0)
      return;

    // Separate along the given separating direction enough to separate as
    // determined by this axis.
    T projOverlap = overlap / axisDot;
    if (chooseDir) {
      T absProjOverlap = (projOverlap >= 0) ? projOverlap : -projOverlap;
      if (absProjOverlap < shortestOverlap) {
        shortestOverlap = absProjOverlap;
        finalSepDir = separatingDir * (projOverlap / absProjOverlap);
      }
    } else if (projOverlap >= 0) {
      if (projOverlap < shortestOverlap) {
        shortestOverlap = projOverlap;
        finalSepDir = separatingDir;
      }
    }
  };

  T overlap = std::numeric_limits<T>::max();
  Vertex separatingDir = Vertex();

  if (!m_vertexes.empty()) {
    Vertex pv = m_vertexes[m_vertexes.size() - 1];
    for (auto const& v : m_vertexes) {
      Vertex sideNormal = pv - v;
      if (sideNormal != Vertex()) {
        sideNormal = sideNormal.rot90().normalized();
        directionalAccumSeparator(p, -sideNormal, overlap, direction, separatingDir, chooseSign);
      }
      pv = v;
    }
  }

  if (!p.m_vertexes.empty()) {
    Vertex pv = p.m_vertexes[p.m_vertexes.size() - 1];
    for (auto const& v : p.m_vertexes) {
      Vertex sideNormal = pv - v;
      if (sideNormal != Vertex()) {
        sideNormal = sideNormal.rot90().normalized();
        directionalAccumSeparator(p, sideNormal, overlap, direction, separatingDir, chooseSign);
      }
      pv = v;
    }
  }

  IntersectResult isect;
  isect.intersects = (overlap > 0);
  isect.overlap = separatingDir * overlap;

  return isect;
}

template <typename T>
auto Polygon<T>::lineIntersection(Line const& l) const -> Maybe<LineIntersectResult> {
  if (contains(l.min()))
    return LineIntersectResult{l.min(), T(0), {}};

  Maybe<LineIntersectResult> nearestIntersection;
  for (size_t i = 0; i < m_vertexes.size(); ++i) {
    auto intersection = l.intersection(sideAt(i));
    if (intersection.intersects) {
      if (!nearestIntersection || intersection.t < nearestIntersection->along)
        nearestIntersection = LineIntersectResult{intersection.point, intersection.t, i};
    }
  }
  return nearestIntersection;
}

template <typename T>
bool Polygon<T>::intersects(Polygon const& p) const {
  return satIntersection(p).intersects;
}

template <typename T>
bool Polygon<T>::intersects(Line const& l) const {
  if (contains(l.min()) || contains(l.max()))
    return true;

  for (size_t i = 0; i < m_vertexes.size(); ++i) {
    if (l.intersects(sideAt(i)))
      return true;
  }

  return false;
}

template <typename T>
auto Polygon<T>::sideAt(size_t i) const -> Line {
  if (i == m_vertexes.size() - 1)
    return Line(m_vertexes[i], m_vertexes[0]);
  else
    return Line(m_vertexes[i], m_vertexes[i + 1]);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, Polygon<T> const& poly) {
  os << "[Poly: ";
  for (auto i = poly.begin(); i != poly.end(); ++i) {
    if (i != poly.begin())
      os << ", ";
    os << *i;
  }
  os << "]";
  return os;
}

}
