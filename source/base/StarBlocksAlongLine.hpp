#ifndef STAR_BLOCKS_ALONG_LINE_HPP
#define STAR_BLOCKS_ALONG_LINE_HPP

#include "StarVector.hpp"

namespace Star {

// Iterate over integral cells based on Bresenham's line drawing algorithm.
// Returns false immediately when the callback returns false for any cell,
// returns true after iterating through every cell otherwise.
template <typename Scalar>
bool forBlocksAlongLine(Vector<Scalar, 2> origin, Vector<Scalar, 2> const& dxdy, function<bool(int, int)> callback) {
  Vector<Scalar, 2> remote = origin + dxdy;

  double dx = dxdy[0];
  if (dx < 0)
    dx *= -1;

  double dy = dxdy[1];
  if (dy < 0)
    dy *= -1;

  double oxfloor = floor(origin[0]);
  double oyfloor = floor(origin[1]);
  double rxfloor = floor(remote[0]);
  double ryfloor = floor(remote[1]);

  if (dx == 0) {
    if (oyfloor < ryfloor) {
      for (int i = oyfloor; i <= ryfloor; ++i) {
        if (!callback(oxfloor, i))
          return false;
      }
    } else {
      for (int i = oyfloor; i >= ryfloor; --i) {
        if (!callback(oxfloor, i))
          return false;
      }
    }
    return true;

  } else if (dy == 0) {
    if (oxfloor < rxfloor) {
      for (int i = oxfloor; i <= rxfloor; ++i) {
        if (!callback(i, oyfloor))
          return false;
      }
    } else {
      for (int i = oxfloor; i >= rxfloor; --i) {
        if (!callback(i, oyfloor))
          return false;
      }
    }
    return true;

  } else {
    int x = oxfloor;
    int y = oyfloor;

    int n = 1;
    int x_inc, y_inc;
    double error;

    if (dxdy[0] > 0) {
      x_inc = 1;
      n += int(rxfloor) - x;
      error = (oxfloor + 1 - origin[0]) * dy;
    } else {
      x_inc = -1;
      n += x - int(rxfloor);
      error = (origin[0] - oxfloor) * dy;
    }

    if (dxdy[1] > 0) {
      y_inc = 1;
      n += int(ryfloor) - y;
      error -= (oyfloor + 1 - origin[1]) * dx;
    } else {
      y_inc = -1;
      n += y - int(ryfloor);
      error -= (origin[1] - oyfloor) * dx;
    }

    for (; n > 0; --n) {
      if (!callback(x, y))
        return false;

      if (error > 0) {
        y += y_inc;
        error -= dx;
      } else if (error < 0) {
        x += x_inc;
        error += dy;
      } else {
        --n;
        y += y_inc;
        x += x_inc;
        error += dy;
        error -= dx;
      }
    }
    return true;
  }
}

}

#endif
