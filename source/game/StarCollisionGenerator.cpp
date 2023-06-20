#include "StarCollisionGenerator.hpp"

namespace Star {

void CollisionGenerator::init(CollisionKindAccessor accessor) {
  m_accessor = accessor;
}

List<CollisionBlock> CollisionGenerator::getBlocks(RectI const& region) const {
  if (region.isNull())
    return {};

  List<CollisionBlock> list;

  populateCollisionBuffer(region);

  getBlocksMarchingSquares(list, region, CollisionKind::Dynamic);
  getBlocksPlatforms(list, region, CollisionKind::Platform);

  return list;
}

void CollisionGenerator::getBlocksPlatforms(List<CollisionBlock>& list, RectI const& region, CollisionKind kind) const {
  int xMin = region.xMin();
  int xMax = region.xMax();
  int yMin = region.yMin();
  int yMax = region.yMax();

  for (int x = xMin; x < xMax; ++x) {
    for (int y = yMin; y < yMax; ++y) {
      if (collisionKind(x, y) == kind) {
        auto addBlock = [&](PolyF::VertexList vertices) {
          CollisionBlock block;
          block.space = Vec2I(x, y);
          block.kind = kind;
          block.poly = PolyF(move(vertices));
          block.polyBounds = block.poly.boundBox();
          list.append(move(block));
        };

        // This was once simple and elegant and made sense but then I made it
        // match the actual platform rendering more closely and now it's a big
        // shitty pile of special cases again. RIP.

        bool right = collisionKind(x + 1, y) == kind;
        bool left = collisionKind(x - 1, y) == kind;

        bool downRight = collisionKind(x + 1, y - 1) == kind && collisionKind(x + 1, y) != kind;
        bool downLeft = collisionKind(x - 1, y - 1) == kind && collisionKind(x - 1, y) != kind;

        bool upRight = collisionKind(x + 1, y + 1) == kind && !left && !right;
        bool upLeft = collisionKind(x - 1, y + 1) == kind && !left && !right;

        bool above = collisionKind(x, y + 1) == kind;
        bool below = collisionKind(x, y - 1) == kind;

        if (downRight && downLeft && upRight && upLeft) {
          addBlock({Vec2F(x, y), Vec2F(x + 1, y + 1)});
          addBlock({Vec2F(x + 1, y), Vec2F(x, y + 1)});
        } else if (above && below) {
          addBlock({Vec2F(x, y + 1), Vec2F(x + 1, y + 1)});
        } else if (upLeft && downLeft && !upRight && !downRight) {
          addBlock({Vec2F(x + 1, y), Vec2F(x, y + 1)});
        } else if (upRight && downRight && !upLeft && !upRight) {
          addBlock({Vec2F(x, y), Vec2F(x + 1, y + 1)});
        } else if (upRight && downLeft) {
          addBlock({Vec2F(x, y), Vec2F(x + 1, y + 1)});

          // special case block for connecting flat platform above
          if (above && collisionKind(x + 1, y + 1) == kind)
            addBlock({Vec2F(x + 1, y + 1), Vec2F(x + 2, y + 2)});
        } else if (upLeft && downRight) {
          addBlock({Vec2F(x + 1, y), Vec2F(x, y + 1)});

          // special case block for connecting flat platform above
          if (above && collisionKind(x - 1, y + 1) == kind)
            addBlock({Vec2F(x, y + 1), Vec2F(x - 1, y + 2)});
        } else if (above && !downRight && !downLeft) {
          addBlock({Vec2F(x, y + 1), Vec2F(x + 1, y + 1)});
        } else if (upLeft && !upRight) {
          addBlock({Vec2F(x + 1, y), Vec2F(x, y + 1)});
        } else if (upRight && !upLeft) {
          addBlock({Vec2F(x, y), Vec2F(x + 1, y + 1)});
        } else if (downRight && (left || !below)) {
          addBlock({Vec2F(x + 1, y), Vec2F(x, y + 1)});
        } else if (downLeft && (right || !below)) {
          addBlock({Vec2F(x, y), Vec2F(x + 1, y + 1)});
        } else {
          addBlock({Vec2F(x, y + 1), Vec2F(x + 1, y + 1)});
        }
      }
    }
  }
}

void CollisionGenerator::getBlocksMarchingSquares(List<CollisionBlock>& list, RectI const& region, CollisionKind kind) const {

  // uses binary masking to assign each group of 4 tiles a value between 0 and 15
  // with corners ul = 1, ur = 2, lr = 4, ll = 8

  // points spaced at 0.5 around the edge of a 1x1 square, clockwise from bottom left,
  // plus the center point for special cases
  static Vec2F const msv[9] = {
    Vec2F(0.5, 0.5),
    Vec2F(0.5, 1.0),
    Vec2F(0.5, 1.5),
    Vec2F(1.0, 1.5),
    Vec2F(1.5, 1.5),
    Vec2F(1.5, 1.0),
    Vec2F(1.5, 0.5),
    Vec2F(1.0, 0.5),
    Vec2F(1.0, 1.0)
  };

  // refers to vertex offset indices in msv, with 8 being no vertex
  static unsigned const msp[22][6] = {
    {9, 9, 9, 9, 9, 9},
    {1, 2, 3, 9, 9, 9},
    {3, 4, 5, 9, 9, 9},
    {1, 2, 4, 5, 9, 9},
    {7, 5, 6, 9, 9, 9},
    {1, 2, 3, 5, 6, 7},
    {7, 3, 4, 6, 9, 9},
    {1, 2, 4, 6, 7, 9},
    {0, 1, 7, 9, 9, 9},
    {0, 2, 3, 7, 9, 9},
    {0, 1, 3, 4, 5, 7},
    {0, 2, 4, 5, 7, 9},
    {0, 1, 5, 6, 9, 9},
    {0, 2, 3, 5, 6, 9},
    {0, 1, 3, 4, 6, 9},
    {0, 2, 4, 6, 9, 9},
    // special cases for squared off top corners
    {5, 6, 7, 8, 9, 9}, // top left corner
    {0, 1, 8, 7, 9, 9}, // top right corner
    // special cases for hollowed out bottom corners
    {0, 2, 3, 8, 9, 9}, // lower left corner part 1
    {0, 8, 5, 6, 9, 9}, // lower left corner part 2
    {0, 1, 8, 6, 9, 9}, // lower right corner part 1
    {6, 8, 3, 4, 9, 9} // lower right corner part 2
  };

  auto addBlock = [&](int x, int y, uint8_t svi) {
    CollisionBlock block;
    block.space = Vec2I(x, y);
    block.poly.clear();
    for (auto i : msp[svi]) {
      if (i == 9)
        break;
      block.poly.add(Vec2F(x, y) + msv[i]);
    }
    block.polyBounds = block.poly.boundBox();
    block.kind = std::max({collisionKind(x, y), collisionKind(x + 1, y), collisionKind(x, y + 1), collisionKind(x + 1, y + 1)});
    list.append(move(block));
  };

  int xMin = region.xMin();
  int xMax = region.xMax();
  int yMin = region.yMin();
  int yMax = region.yMax();

  for (int x = xMin; x < xMax; ++x) {
    for (int y = yMin; y < yMax; ++y) {
      uint8_t neighborMask = 0;
      if (collisionKind(x, y + 1) >= kind)
        neighborMask |= 1;
      if (collisionKind(x + 1, y + 1) >= kind)
        neighborMask |= 2;
      if (collisionKind(x + 1, y) >= kind)
        neighborMask |= 4;
      if (collisionKind(x, y) >= kind)
        neighborMask |= 8;

      if (neighborMask == 4) {
        if (collisionKind(x + 2, y) >= kind &&
            collisionKind(x + 2, y + 1) < kind &&
            collisionKind(x, y - 1) < kind) {

          addBlock(x, y, 16);
          continue;
        }
      } else if (neighborMask == 8) {
        if (collisionKind(x - 1, y) >= kind &&
            collisionKind(x - 1, y + 1) < kind &&
            collisionKind(x + 1, y - 1) < kind) {

          addBlock(x, y, 17);
          continue;
        }
      } else if (neighborMask == 13) {
        if (collisionKind(x, y + 2) >= kind &&
            collisionKind(x + 1, y + 2) < kind &&
            collisionKind(x + 2, y) >= kind) {

          addBlock(x, y, 18);
          addBlock(x, y, 19);
          continue;
        }
      } else if (neighborMask == 14) {
        if (collisionKind(x, y + 2) < kind &&
            collisionKind(x + 1, y + 2) >= kind &&
            collisionKind(x - 1, y) >= kind) {

          addBlock(x, y, 20);
          addBlock(x, y, 21);
          continue;
        }
      }

      if (neighborMask != 0)
        addBlock(x, y, neighborMask);
    }
  }
}

void CollisionGenerator::populateCollisionBuffer(RectI const& region) const {
  int xmin = region.xMin() - BlockInfluenceRadius;
  int ymin = region.yMin() - BlockInfluenceRadius;
  int xmax = region.xMax() + BlockInfluenceRadius;
  int ymax = region.yMax() + BlockInfluenceRadius;

  m_collisionBufferCorner = {xmin, ymin};
  m_collisionBuffer.resize(xmax - xmin, ymax - ymin);
  for (int x = xmin; x < xmax; ++x)
    for (int y = ymin; y < ymax; ++y)
      m_collisionBuffer(x - xmin, y - ymin) = m_accessor(x, y);
}

CollisionKind CollisionGenerator::collisionKind(int x, int y) const {
  return m_collisionBuffer(x - m_collisionBufferCorner[0], y - m_collisionBufferCorner[1]);
}
  
}
