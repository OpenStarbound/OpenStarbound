#include "StarWorldGeometry.hpp"

namespace Star {

function<float(float, float)> WorldGeometry::xDiffFunction() const {
  if (m_size[0] == 0) {
    return [](float x1, float x2) -> float { return x1 - x2; };
  } else {
    unsigned xsize = m_size[0];
    return [xsize](float x1, float x2) -> float { return wrapDiffF<float>(x1, x2, xsize); };
  }
}

function<Vec2F(Vec2F, Vec2F)> WorldGeometry::diffFunction() const {
  if (m_size[0] == 0) {
    return [](Vec2F const& a, Vec2F const& b) -> Vec2F { return a - b; };
  } else {
    unsigned xsize = m_size[0];
    return [xsize](Vec2F const& a, Vec2F const& b) -> Vec2F {
      return Vec2F(wrapDiffF<float>(a[0], b[0], xsize), a[1] - b[1]);
    };
  }
}

function<float(float, float, float)> WorldGeometry::xLerpFunction(Maybe<float> discontinuityThreshold) const {
  if (m_size[0] == 0) {
    return [](float, float min, float) -> float { return min; };
  } else {
    unsigned xsize = m_size[0];
    return [discontinuityThreshold, xsize](float offset, float min, float max) -> float {
      float distance = wrapDiffF<float>(max, min, xsize);
      if (discontinuityThreshold && distance > *discontinuityThreshold)
        return min + distance;
      return min + offset * distance;
    };
  }
}

function<Vec2F(float, Vec2F, Vec2F)> WorldGeometry::lerpFunction(Maybe<float> discontinuityThreshold) const {
  if (m_size[0] == 0) {
    return [](float, Vec2F const& min, Vec2F const&) -> Vec2F { return min; };
  } else {
    unsigned xsize = m_size[0];
    return [discontinuityThreshold, xsize](float offset, Vec2F const& min, Vec2F const& max) -> Vec2F {
      Vec2F distance = Vec2F(wrapDiffF<float>(max[0], min[0], xsize), max[1] - min[1]);
      if (discontinuityThreshold && distance.magnitude() > *discontinuityThreshold)
        return min + distance;
      return min + offset * distance;
    };
  }
}

StaticList<RectF, 2> WorldGeometry::splitRect(RectF const& bbox) const {
  if (bbox.isNull() || m_size[0] == 0)
    return {bbox};

  Vec2F minWrap = xwrap(bbox.min());
  RectF bboxWrap = RectF(minWrap, minWrap + bbox.size());

  // This does not work for ranges greater than m_size[0] wide!
  starAssert(bbox.xMax() - bbox.xMin() <= (float)m_size[0]);

  // Since min is wrapped, we're only checking to see if max is on the other
  // side of the wrap point
  if (bboxWrap.xMax() > m_size[0]) {
    return {RectF(bboxWrap.xMin(), bboxWrap.yMin(), m_size[0], bboxWrap.yMax()),
        RectF(0, bboxWrap.yMin(), bboxWrap.xMax() - m_size[0], bboxWrap.yMax())};
  } else {
    return {bboxWrap};
  }
}

StaticList<RectF, 2> WorldGeometry::splitRect(RectF bbox, Vec2F const& position) const {
  bbox.translate(position);
  return splitRect(bbox);
}

StaticList<RectI, 2> WorldGeometry::splitRect(RectI const bbox) const {
  if (bbox.isNull() || m_size[0] == 0)
    return {bbox};

  Vec2I minWrap = xwrap(bbox.min());
  RectI bboxWrap = RectI(minWrap, minWrap + bbox.size());

  // This does not work for ranges greater than m_size[0] wide!
  starAssert(bbox.xMax() - bbox.xMin() <= (int)m_size[0]);

  // Since min is wrapped, we're only checking to see if max is on the other
  // side of the wrap point
  if (bboxWrap.xMax() > (int)m_size[0]) {
    return {RectI(bboxWrap.xMin(), bboxWrap.yMin(), m_size[0], bboxWrap.yMax()),
        RectI(0, bboxWrap.yMin(), bboxWrap.xMax() - m_size[0], bboxWrap.yMax())};
  } else {
    return {bboxWrap};
  }
}

StaticList<Line2F, 2> WorldGeometry::splitLine(Line2F line, bool preserveDirection) const {
  if (m_size[0] == 0)
    return {line};

  bool swapDirection = line.makePositive() && preserveDirection;
  Vec2F minWrap = xwrap(line.min());

  // diff is safe because we're looking for the line gnostic diff
  Line2F lineWrap = Line2F(minWrap, minWrap + line.diff());

  // Since min is wrapped, we're only checking to see if max is on the other
  // side of the wrap point
  if (lineWrap.max()[0] > m_size[0]) {
    Vec2F intersection = lineWrap.intersection(Line2F(Vec2F(m_size[0], 0), Vec2F(m_size)), true).point;
    if (swapDirection)
      return {Line2F(lineWrap.max() - Vec2F(m_size[0], 0), Vec2F(0, intersection[1])),
          Line2F(Vec2F(m_size[0], intersection[1]), lineWrap.min())};
    else
      return {Line2F(lineWrap.min(), Vec2F(m_size[0], intersection[1])),
          Line2F(Vec2F(0, intersection[1]), lineWrap.max() - Vec2F(m_size[0], 0))};
  } else {
    if (swapDirection)
      lineWrap.reverse();
    return {lineWrap};
  }
}

StaticList<Line2F, 2> WorldGeometry::splitLine(Line2F line, Vec2F const& position, bool preserveDirection) const {
  line.translate(position);
  return splitLine(line, preserveDirection);
}

StaticList<PolyF, 2> WorldGeometry::splitPoly(PolyF const& poly) const {
  if (poly.isNull() || m_size[0] == 0)
    return {poly};

  Array<PolyF, 2> res;
  bool polySelect = false;

  Line2F worldBoundRight = {Vec2F(m_size[0], 0), Vec2F(m_size[0], 1)};
  Line2F worldBoundLeft = {Vec2F(0, 0), Vec2F(0, 1)};

  for (unsigned i = 0; i < poly.sides(); i++) {
    Line2F segment = poly.side(i);
    if ((segment.min()[0] < 0) ^ (segment.max()[0] < 0)) {
      Vec2F worldCorrect = {(float)m_size[0], 0};
      Vec2F intersect = segment.intersection(worldBoundLeft, true).point;
      if (segment.min()[0] < 0) {
        res[polySelect].add(segment.min() + worldCorrect);
        res[polySelect].add(Vec2F(m_size[0], intersect[1]));
        polySelect = !polySelect;
        res[polySelect].add(Vec2F(0, intersect[1]));
      } else {
        res[polySelect].add(segment.min());
        res[polySelect].add(Vec2F(0, intersect[1]));
        polySelect = !polySelect;
        res[polySelect].add(Vec2F(m_size[0], intersect[1]));
      }
    } else if ((segment.min()[0] > m_size[0]) ^ (segment.max()[0] > m_size[0])) {
      Vec2F worldCorrect = {(float)m_size[0], 0};
      Vec2F intersect = segment.intersection(worldBoundRight, true).point;
      if (segment.min()[0] > m_size[0]) {
        res[polySelect].add(segment.min() - worldCorrect);
        res[polySelect].add(Vec2F(0, intersect[1]));
        polySelect = !polySelect;
        res[polySelect].add(Vec2F(m_size[0], intersect[1]));
      } else {
        res[polySelect].add(segment.min());
        res[polySelect].add(Vec2F(m_size[0], intersect[1]));
        polySelect = !polySelect;
        res[polySelect].add(Vec2F(0, intersect[1]));
      }
    } else {
      if (segment.min()[0] < 0) {
        res[polySelect].add(segment.min() + Vec2F((float)m_size[0], 0));
      } else if (segment.min()[0] > m_size[0]) {
        res[polySelect].add(segment.min() - Vec2F((float)m_size[0], 0));
      } else {
        res[polySelect].add(segment.min());
      }
    }
  }

  if (res[1].isNull())
    return {res[0]};
  if (res[0].isNull())
    return {res[1]};
  else
    return {res[0], res[1]};
}

StaticList<PolyF, 2> WorldGeometry::splitPoly(PolyF poly, Vec2F const& position) const {
  poly.translate(position);
  return splitPoly(poly);
}

StaticList<Vec2I, 2> WorldGeometry::splitXRegion(Vec2I const& xRegion) const {
  if (m_size[0] == 0)
    return {xRegion};

  starAssert(xRegion[1] >= xRegion[0]);

  // This does not work for ranges greater than m_size[0] wide!
  starAssert(xRegion[1] - xRegion[0] <= (int)m_size[0]);

  int x1 = xwrap(xRegion[0]);
  int x2 = x1 + xRegion[1] - xRegion[0];

  if (x2 > (int)m_size[0]) {
    return {Vec2I(x1, m_size[0]), Vec2I(0.0f, x2 - m_size[0])};
  } else {
    return {{x1, x2}};
  }
}

StaticList<Vec2F, 2> WorldGeometry::splitXRegion(Vec2F const& xRegion) const {
  if (m_size[0] == 0)
    return {xRegion};

  starAssert(xRegion[1] >= xRegion[0]);

  // This does not work for ranges greater than m_size[0] wide!
  starAssert(xRegion[1] - xRegion[0] <= (float)m_size[0]);

  float x1 = xwrap(xRegion[0]);
  float x2 = x1 + xRegion[1] - xRegion[0];

  if (x2 > m_size[0]) {
    return {Vec2F(x1, m_size[0]), Vec2F(0.0f, x2 - m_size[0])};
  } else {
    return {{x1, x2}};
  }
}

bool WorldGeometry::rectContains(RectF const& rect, Vec2F const& pos) const {
  auto wpos = xwrap(pos);
  for (auto const& r : splitRect(rect)) {
    if (r.contains(wpos))
      return true;
  }
  return false;
}

bool WorldGeometry::rectIntersectsRect(RectF const& rect1, RectF const& rect2) const {
  for (auto const& r1 : splitRect(rect1)) {
    for (auto const& r2 : splitRect(rect2)) {
      if (r1.intersects(r2))
        return true;
    }
  }
  return false;
}

RectF WorldGeometry::rectOverlap(RectF const& rect1, RectF const& rect2) const {
  return rect1.overlap(RectF::withSize(nearestTo(rect1.min(), rect2.min()), rect2.size()));
}

bool WorldGeometry::polyContains(PolyF const& poly, Vec2F const& pos) const {
  auto wpos = xwrap(pos);
  for (auto const& p : splitPoly(poly)) {
    if (p.contains(wpos))
      return true;
  }
  return false;
}

float WorldGeometry::polyOverlapArea(PolyF const& poly1, PolyF const& poly2) const {
  float area = 0.0f;
  for (auto const& p1 : splitPoly(poly1)) {
    for (auto const& p2 : splitPoly(poly2))
      area += PolyF::clip(p1, p2).convexArea();
  }
  return area;
}

bool WorldGeometry::lineIntersectsRect(Line2F const& line, RectF const& rect) const {
  for (auto l : splitLine(line)) {
    for (auto box : splitRect(rect)) {
      if (box.intersects(l)) {
        return true;
      }
    }
  }
  return false;
}

bool WorldGeometry::lineIntersectsPoly(Line2F const& line, PolyF const& poly) const {
  for (auto a : splitLine(line)) {
    for (auto b : splitPoly(poly)) {
      if (b.intersects(a)) {
        return true;
      }
    }
  }

  return false;
}

bool WorldGeometry::polyIntersectsPoly(PolyF const& polyA, PolyF const& polyB) const {
  for (auto a : splitPoly(polyA)) {
    for (auto b : splitPoly(polyB)) {
      if (b.intersects(a))
        return true;
    }
  }

  return false;
}

bool WorldGeometry::rectIntersectsCircle(RectF const& rect, Vec2F const& center, float radius) const {
  if (rect.contains(center))
    return true;
  for (auto const& e : rect.edges()) {
    if (lineIntersectsCircle(e, center, radius))
      return true;
  }
  return false;
}

bool WorldGeometry::lineIntersectsCircle(Line2F const& line, Vec2F const& center, float radius) const {
  for (auto const& sline : splitLine(line)) {
    if (sline.distanceTo(nearestTo(sline.center(), center)) <= radius)
      return true;
  }
  return false;
}

Maybe<Vec2F> WorldGeometry::lineIntersectsPolyAt(Line2F const& line, PolyF const& poly) const {
  for (auto a : splitLine(line, true)) {
    for (auto b : splitPoly(poly)) {
      if (auto intersection = b.lineIntersection(a))
        return intersection->point;
    }
  }

  return {};
}

float WorldGeometry::polyDistance(PolyF const& poly, Vec2F const& point) const {
  auto spoint = nearestTo(poly.center(), point);
  return poly.distance(spoint);
}

Vec2F WorldGeometry::nearestCoordInBox(RectF const& box, Vec2F const& pos) const {
  RectF t(box);
  auto offset = t.center();
  auto r = diff(pos, offset);
  t.setCenter({});
  return t.nearestCoordTo(r) + offset;
}

Vec2F WorldGeometry::diffToNearestCoordInBox(RectF const& box, Vec2F const& pos) const {
  RectF t(box);
  auto offset = t.center();
  auto r = diff(pos, offset);
  t.setCenter({});
  auto coord = t.nearestCoordTo(r) + offset;
  return diff(pos, coord);
}

}
