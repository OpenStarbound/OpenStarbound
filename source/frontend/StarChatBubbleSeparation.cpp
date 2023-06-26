#include "StarChatBubbleSeparation.hpp"
#include "StarLogging.hpp"

namespace Star {

bool compareLeft(RectF const& a, RectF const& b) {
  return a.xMin() < b.xMin();
}

bool compareRight(RectF const& a, RectF const& b) {
  return a.xMax() > b.xMax();
}

bool compareOverlapLeft(RectF const& newBox, RectF const& fixedBox) {
  return newBox.xMax() < fixedBox.xMin();
}

bool compareOverlapRight(RectF const& newBox, RectF const& fixedBox) {
  return newBox.xMin() > fixedBox.xMax();
}

template <typename Compare>
void appendHorizontalOverlaps(List<RectF>& overlaps,
    List<RectF> const& boxes,
    List<RectF>::const_iterator leftBound,
    Compare compare,
    RectF const& box) {
  auto i = leftBound;
  if (i == boxes.begin())
    return;
  --i;
  while (!compare(box, *i)) {
    overlaps.append(*i);
    if (i == boxes.begin())
      return;
    --i;
  }
}

RectF separateBubble(List<RectF> const& sortedLeftEdges, List<RectF> const& sortedRightEdges, List<RectF>& outLeftEdges, List<RectF>& outRightEdges, RectF box) {
  // We have to maintain two lists of boxes: one sorted by the left edges and
  // one
  // by the right edges. This is because boxes can be different sizes, and
  // if we only check one edge, appendHorizontalOverlaps can miss any boxes
  // whose projections onto the X axis entirely contain other boxes'.
  auto leftOverlapBound = upper_bound(sortedLeftEdges.begin(), sortedLeftEdges.end(), box, compareOverlapLeft);
  auto rightOverlapBound = upper_bound(sortedRightEdges.begin(), sortedRightEdges.end(), box, compareOverlapRight);

  List<RectF> horizontalOverlaps;
  appendHorizontalOverlaps(horizontalOverlaps, sortedLeftEdges, leftOverlapBound, compareOverlapRight, box);
  appendHorizontalOverlaps(horizontalOverlaps, sortedRightEdges, rightOverlapBound, compareOverlapLeft, box);

  // horizontalOverlaps now consists of the boxes that (when projected onto the
  // X axis)
  // overlap with 'box'.

  while (true) {
    // While box is overlapping any other boxes, move it halfway away.
    List<RectF> overlappingBoxes = horizontalOverlaps.filtered([&box](RectF const& overlapper) {
      if (overlapper.intersects(box, false)) {
        Vec2F oSize = overlapper.size(), bSize = box.size();
        if (oSize[0] == bSize[0]) {
          if (oSize[1] == bSize[1])
            return overlapper.center()[1] <= box.center()[1];
          else
            return oSize[1] > bSize[1];
        }
        else if (oSize[0] > bSize[0])
          return true;
      }
      return false;
    });
    if (overlappingBoxes.empty())
      break;
    RectF overlapBoundBox = fold(overlappingBoxes, box, [](RectF const& a, RectF const& b) { return a.combined(b); });
    SpatialLogger::logPoly("screen", PolyF(box), { 255, 0, 0, 255 });
    SpatialLogger::logPoly("screen", PolyF(overlapBoundBox), { 0, 0, 255, 255 });
    auto height = box.height();
    box.setYMin(overlapBoundBox.yMax());
    box.setYMax(box.yMin() + height);
  }

  outLeftEdges.append(box);
  outRightEdges.append(box);

  return box;
}

}
