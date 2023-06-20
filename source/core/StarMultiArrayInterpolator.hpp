#ifndef STAR_MULTI_ARRAY_INTERPOLATOR_HPP
#define STAR_MULTI_ARRAY_INTERPOLATOR_HPP

#include "StarMultiArray.hpp"
#include "StarInterpolation.hpp"

namespace Star {

template <typename MultiArrayT, typename PositionT>
struct MultiArrayInterpolator2 {
  typedef MultiArrayT MultiArray;
  typedef PositionT Position;

  typedef typename MultiArray::Element Element;
  static size_t const Rank = MultiArray::Rank;

  typedef Array<size_t, Rank> IndexList;
  typedef Array<size_t, Rank> SizeList;
  typedef Array<Position, Rank> PositionList;
  typedef Array<Position, 2> WeightList;

  typedef std::function<WeightList(Position)> WeightFunction;

  WeightFunction weightFunction;
  BoundMode boundMode;

  MultiArrayInterpolator2(WeightFunction wf, BoundMode b = BoundMode::Clamp)
    : weightFunction(wf), boundMode(b) {}

  Element interpolate(MultiArray const& array, PositionList const& coord) const {
    IndexList imin;
    IndexList imax;
    PositionList offset;

    for (size_t i = 0; i < Rank; ++i) {
      auto binfo = getBound2(coord[i], array.size(i), boundMode);
      imin[i] = binfo.i0;
      imax[i] = binfo.i1;
      offset[i] = binfo.offset;
    }

    return interpolateSub(array, imin, imax, offset, IndexList(), 0);
  }

  Element interpolateSub(
      MultiArray const& array,
      IndexList const& imin, IndexList const& imax,
      PositionList const& offset, IndexList const& index,
      size_t const dim) const {
    IndexList minIndex = index;
    IndexList maxIndex = index;

    minIndex[dim] = imin[dim];
    maxIndex[dim] = imax[dim];

    WeightList weights = weightFunction(offset[dim]);

    if (dim == Rank - 1) {
      return weights[0] * array(minIndex) + weights[1] * array(maxIndex);
    } else {
      return
        weights[0] * interpolateSub(array, imin, imax, offset, minIndex, dim+1) +
        weights[1] * interpolateSub(array, imin, imax, offset, maxIndex, dim+1);
    }
  }
};

template <typename MultiArrayT, typename PositionT>
struct MultiArrayInterpolator4 {
  typedef MultiArrayT MultiArray;
  typedef PositionT Position;

  typedef typename MultiArray::Element Element;
  static size_t const Rank = MultiArray::Rank;

  typedef Array<size_t, Rank> IndexList;
  typedef Array<size_t, Rank> SizeList;
  typedef Array<Position, Rank> PositionList;
  typedef Array<Position, 4> WeightList;

  typedef std::function<WeightList(Position)> WeightFunction;

  WeightFunction weightFunction;
  BoundMode boundMode;

  MultiArrayInterpolator4(WeightFunction wf, BoundMode b = BoundMode::Clamp)
    : weightFunction(wf), boundMode(b) {}

  Element interpolate(MultiArray const& array, PositionList const& coord) const {
    IndexList index0;
    IndexList index1;
    IndexList index2;
    IndexList index3;
    PositionList offset;

    for (size_t i = 0; i < Rank; ++i) {
      auto bound = getBound4(coord[i], array.size(i), boundMode);
      index0[i] = bound.i0;
      index1[i] = bound.i1;
      index2[i] = bound.i2;
      index3[i] = bound.i3;
      offset[i] = bound.offset;
    }

    return interpolateSub(array, index0, index1, index2, index3, offset, IndexList(), 0);
  }

  Element interpolateSub(
      MultiArray const& array,
      IndexList const& i0, IndexList const& i1,
      IndexList const& i2, IndexList const& i3,
      PositionList const& offset, IndexList const& index,
      size_t const dim) const {
    IndexList index0 = index;
    IndexList index1 = index;
    IndexList index2 = index;
    IndexList index3 = index;

    index0[dim] = i0[dim];
    index1[dim] = i1[dim];
    index2[dim] = i2[dim];
    index3[dim] = i3[dim];

    WeightList weights = weightFunction(offset[dim]);

    if (dim == Rank - 1) {
      return
        weights[0] * array(index0) +
        weights[1] * array(index1) +
        weights[2] * array(index2) +
        weights[3] * array(index3);
    } else {
      return
        weights[0] * interpolateSub(array, i0, i1, i2, i3, offset, index0, dim+1) +
        weights[1] * interpolateSub(array, i0, i1, i2, i3, offset, index1, dim+1) +
        weights[2] * interpolateSub(array, i0, i1, i2, i3, offset, index2, dim+1) +
        weights[3] * interpolateSub(array, i0, i1, i2, i3, offset, index3, dim+1);
    }
  }
};

template <typename MultiArrayT, typename PositionT>
struct MultiArrayPiecewiseInterpolator {
  typedef MultiArrayT MultiArray;
  typedef PositionT Position;

  typedef typename MultiArray::Element Element;
  static size_t const Rank = MultiArray::Rank;

  typedef Array<size_t, Rank> IndexList;
  typedef Array<size_t, Rank> SizeList;
  typedef Array<Position, Rank> PositionList;
  typedef Array<Position, 2> WeightList;

  typedef std::function<WeightList(Position)> WeightFunction;

  struct PiecewiseRange {
    size_t dim;
    Position offset;

    bool operator<(PiecewiseRange const& pr) const {
      return pr.offset < offset;
    }
  };
  typedef Array<PiecewiseRange, Rank> PiecewiseRangeList;

  WeightFunction weightFunction;
  BoundMode boundMode;

  MultiArrayPiecewiseInterpolator(WeightFunction wf, BoundMode b = BoundMode::Clamp)
    : weightFunction(wf), boundMode(b) {}

  // O(n) for n-dimensions.
  Element interpolate(MultiArray const& array, PositionList const& coord) const {
    PiecewiseRangeList piecewiseRangeList;

    IndexList minIndex;
    IndexList maxIndex;

    for (size_t i = 0; i < Rank; ++i) {
        PiecewiseRange range;
        range.dim = i;

        auto bound = getBound2(coord[i], array.size(i), boundMode);
        minIndex[i] = bound.i0;
        maxIndex[i] = bound.i1;
        range.offset = bound.offset;

        piecewiseRangeList[i] = range;
    }

    std::sort(piecewiseRangeList.begin(), piecewiseRangeList.end());

    IndexList location = minIndex;
    Element result = array(location);
    Element last = result;
    Element current;

    for (size_t i = 0; i < Rank; ++i) {
      auto const& pr = piecewiseRangeList[i];
      location[pr.dim] = maxIndex[pr.dim];
      current = array(location);

      WeightList weights = weightFunction(pr.offset);
      result += last * (weights[0] - 1) + current * weights[1];
      last = current;
    }

    return result;
  }
};

// Template specializations for Rank 2

template <typename ElementT, typename PositionT>
struct MultiArrayInterpolator2<MultiArray<ElementT, 2>, PositionT> {
  typedef Star::MultiArray<ElementT, 2> MultiArray;
  typedef PositionT Position;

  typedef typename MultiArray::Element Element;
  static size_t const Rank = 2;

  typedef Array<size_t, Rank> IndexList;
  typedef Array<size_t, Rank> SizeList;
  typedef Array<Position, Rank> PositionList;
  typedef Array<Position, 2> WeightList;

  typedef std::function<WeightList(Position)> WeightFunction;

  WeightFunction weightFunction;
  BoundMode boundMode;

  MultiArrayInterpolator2(WeightFunction wf, BoundMode b = BoundMode::Clamp)
    : weightFunction(wf), boundMode(b) {}

  Element interpolate(MultiArray const& array, PositionList const& coord) const {
    IndexList imin;
    IndexList imax;
    PositionList offset;

    for (size_t i = 0; i < Rank; ++i) {
      auto bound = getBound2(coord[i], array.size(i), boundMode);
      imin[i] = bound.i0;
      imax[i] = bound.i1;
      offset[i] = bound.offset;
    }

    WeightList xweights = weightFunction(offset[0]);
    WeightList yweights = weightFunction(offset[1]);

    return
      xweights[0] * (yweights[0] * array(imin[0], imin[1]) + yweights[1] * array(imin[0], imax[1])) +
      xweights[1] * (yweights[0] * array(imax[0], imin[1]) + yweights[1] * array(imax[0], imax[1]));
  }
};

template <typename ElementT, typename PositionT>
struct MultiArrayInterpolator4<MultiArray<ElementT, 2>, PositionT> {
  typedef Star::MultiArray<ElementT, 2> MultiArray;
  typedef PositionT Position;

  typedef typename MultiArray::Element Element;
  static size_t const Rank = 2;

  typedef Array<size_t, Rank> IndexList;
  typedef Array<size_t, Rank> SizeList;
  typedef Array<Position, Rank> PositionList;
  typedef Array<Position, 4> WeightList;

  typedef std::function<WeightList(Position)> WeightFunction;

  WeightFunction weightFunction;
  BoundMode boundMode;

  MultiArrayInterpolator4(WeightFunction wf, BoundMode b = BoundMode::Clamp)
    : weightFunction(wf), boundMode(b) {}

  Element interpolate(MultiArray const& array, PositionList const& coord) const {
    IndexList index0;
    IndexList index1;
    IndexList index2;
    IndexList index3;
    PositionList offset;

    for (size_t i = 0; i < Rank; ++i) {
      auto bound = getBound4(coord[i], array.size(i), boundMode);
      index0[i] = bound.i0;
      index1[i] = bound.i1;
      index2[i] = bound.i2;
      index3[i] = bound.i3;
      offset[i] = bound.offset;
    }

    WeightList xweights = weightFunction(offset[0]);
    WeightList yweights = weightFunction(offset[1]);

    return
      xweights[0] * (
          yweights[0] * array(index0[0], index0[1]) +
          yweights[1] * array(index0[0], index1[1]) +
          yweights[2] * array(index0[0], index2[1]) +
          yweights[3] * array(index0[0], index3[1])
        ) +
      xweights[1] * (
          yweights[0] * array(index1[0], index0[1]) +
          yweights[1] * array(index1[0], index1[1]) +
          yweights[2] * array(index1[0], index2[1]) +
          yweights[3] * array(index1[0], index3[1])
        ) +
      xweights[2] * (
          yweights[0] * array(index2[0], index0[1]) +
          yweights[1] * array(index2[0], index1[1]) +
          yweights[2] * array(index2[0], index2[1]) +
          yweights[3] * array(index2[0], index3[1])
        ) +
      xweights[3] * (
          yweights[0] * array(index3[0], index0[1]) +
          yweights[1] * array(index3[0], index1[1]) +
          yweights[2] * array(index3[0], index2[1]) +
          yweights[3] * array(index3[0], index3[1])
        );
  }
};

// Template specializations for Rank 3

template <typename ElementT, typename PositionT>
struct MultiArrayInterpolator2<MultiArray<ElementT, 3>, PositionT> {
  typedef Star::MultiArray<ElementT, 3> MultiArray;
  typedef PositionT Position;

  typedef typename MultiArray::Element Element;
  static size_t const Rank = 3;

  typedef Array<size_t, Rank> IndexList;
  typedef Array<size_t, Rank> SizeList;
  typedef Array<Position, Rank> PositionList;
  typedef Array<Position, 2> WeightList;

  typedef std::function<WeightList(Position)> WeightFunction;

  WeightFunction weightFunction;
  BoundMode boundMode;

  MultiArrayInterpolator2(WeightFunction wf, BoundMode b = BoundMode::Clamp)
    : weightFunction(wf), boundMode(b) {}

  Element interpolate(MultiArray const& array, PositionList const& coord) const {
    IndexList imin;
    IndexList imax;
    PositionList offset;

    for (size_t i = 0; i < Rank; ++i) {
      auto bound = getBound2(coord[i], array.size(i), boundMode);
      imin[i] = bound.i0;
      imax[i] = bound.i1;
      offset[i] = bound.offset;
    }

    WeightList xweights = weightFunction(offset[0]);
    WeightList yweights = weightFunction(offset[1]);
    WeightList zweights = weightFunction(offset[2]);

    return
      xweights[0] * (
            yweights[0] * (
              zweights[0] * array(imin[0], imin[1], imin[2]) +
              zweights[1] * array(imin[0], imin[1], imax[2])
            ) +
            yweights[1] * (
              zweights[0] * array(imin[0], imax[1], imin[2]) +
              zweights[1] * array(imin[0], imax[1], imax[2])
            )
          ) +
      xweights[1] * (
            yweights[0] * (
              zweights[0] * array(imax[0], imin[1], imin[2]) +
              zweights[1] * array(imax[0], imin[1], imax[2])
            ) +
            yweights[1] * (
              zweights[0] * array(imax[0], imax[1], imin[2]) +
              zweights[1] * array(imax[0], imax[1], imax[2])
            )
          );
  }
};

template <typename ElementT, typename PositionT>
struct MultiArrayInterpolator4<MultiArray<ElementT, 3>, PositionT> {
  typedef Star::MultiArray<ElementT, 3> MultiArray;
  typedef PositionT Position;

  typedef typename MultiArray::Element Element;
  static size_t const Rank = 3;

  typedef Array<size_t, Rank> IndexList;
  typedef Array<size_t, Rank> SizeList;
  typedef Array<Position, Rank> PositionList;
  typedef Array<Position, 4> WeightList;

  typedef std::function<WeightList(Position)> WeightFunction;

  WeightFunction weightFunction;
  BoundMode boundMode;

  MultiArrayInterpolator4(WeightFunction wf, BoundMode b = BoundMode::Clamp)
    : weightFunction(wf), boundMode(b) {}

  Element interpolate(MultiArray const& array, PositionList const& coord) const {
    IndexList index0;
    IndexList index1;
    IndexList index2;
    IndexList index3;
    PositionList offset;

    for (size_t i = 0; i < Rank; ++i) {
      auto bound = getBound4(coord[i], array.size(i), boundMode);
      index0[i] = bound.i0;
      index1[i] = bound.i1;
      index2[i] = bound.i2;
      index3[i] = bound.i3;
      offset[i] = bound.offset;
    }

    WeightList xweights = weightFunction(offset[0]);
    WeightList yweights = weightFunction(offset[1]);
    WeightList zweights = weightFunction(offset[2]);

    return
      xweights[0] * (
            yweights[0] * (
              zweights[0] * array(index0[0], index0[1], index0[2]) +
              zweights[1] * array(index0[0], index0[1], index1[2]) +
              zweights[2] * array(index0[0], index0[1], index2[2]) +
              zweights[3] * array(index0[0], index0[1], index3[2])
            ) +
            yweights[1] * (
              zweights[0] * array(index0[0], index1[1], index0[2]) +
              zweights[1] * array(index0[0], index1[1], index1[2]) +
              zweights[2] * array(index0[0], index1[1], index2[2]) +
              zweights[3] * array(index0[0], index1[1], index3[2])
            ) +
            yweights[2] * (
              zweights[0] * array(index0[0], index2[1], index0[2]) +
              zweights[1] * array(index0[0], index2[1], index1[2]) +
              zweights[2] * array(index0[0], index2[1], index2[2]) +
              zweights[3] * array(index0[0], index2[1], index3[2])
            ) +
            yweights[3] * (
              zweights[0] * array(index0[0], index3[1], index0[2]) +
              zweights[1] * array(index0[0], index3[1], index1[2]) +
              zweights[2] * array(index0[0], index3[1], index2[2]) +
              zweights[3] * array(index0[0], index3[1], index3[2])
            )
          ) +
      xweights[1] * (
            yweights[0] * (
              zweights[0] * array(index1[0], index0[1], index0[2]) +
              zweights[1] * array(index1[0], index0[1], index1[2]) +
              zweights[2] * array(index1[0], index0[1], index2[2]) +
              zweights[3] * array(index1[0], index0[1], index3[2])
            ) +
            yweights[1] * (
              zweights[0] * array(index1[0], index1[1], index0[2]) +
              zweights[1] * array(index1[0], index1[1], index1[2]) +
              zweights[2] * array(index1[0], index1[1], index2[2]) +
              zweights[3] * array(index1[0], index1[1], index3[2])
            ) +
            yweights[2] * (
              zweights[0] * array(index1[0], index2[1], index0[2]) +
              zweights[1] * array(index1[0], index2[1], index1[2]) +
              zweights[2] * array(index1[0], index2[1], index2[2]) +
              zweights[3] * array(index1[0], index2[1], index3[2])
            ) +
            yweights[3] * (
              zweights[0] * array(index1[0], index3[1], index0[2]) +
              zweights[1] * array(index1[0], index3[1], index1[2]) +
              zweights[2] * array(index1[0], index3[1], index2[2]) +
              zweights[3] * array(index1[0], index3[1], index3[2])
            )
          ) +
      xweights[2] * (
            yweights[0] * (
              zweights[0] * array(index2[0], index0[1], index0[2]) +
              zweights[1] * array(index2[0], index0[1], index1[2]) +
              zweights[2] * array(index2[0], index0[1], index2[2]) +
              zweights[3] * array(index2[0], index0[1], index3[2])
            ) +
            yweights[1] * (
              zweights[0] * array(index2[0], index1[1], index0[2]) +
              zweights[1] * array(index2[0], index1[1], index1[2]) +
              zweights[2] * array(index2[0], index1[1], index2[2]) +
              zweights[3] * array(index2[0], index1[1], index3[2])
            ) +
            yweights[2] * (
              zweights[0] * array(index2[0], index2[1], index0[2]) +
              zweights[1] * array(index2[0], index2[1], index1[2]) +
              zweights[2] * array(index2[0], index2[1], index2[2]) +
              zweights[3] * array(index2[0], index2[1], index3[2])
            ) +
            yweights[3] * (
              zweights[0] * array(index2[0], index3[1], index0[2]) +
              zweights[1] * array(index2[0], index3[1], index1[2]) +
              zweights[2] * array(index2[0], index3[1], index2[2]) +
              zweights[3] * array(index2[0], index3[1], index3[2])
            )
          ) +
      xweights[3] * (
            yweights[0] * (
              zweights[0] * array(index3[0], index0[1], index0[2]) +
              zweights[1] * array(index3[0], index0[1], index1[2]) +
              zweights[2] * array(index3[0], index0[1], index2[2]) +
              zweights[3] * array(index3[0], index0[1], index3[2])
            ) +
            yweights[1] * (
              zweights[0] * array(index3[0], index1[1], index0[2]) +
              zweights[1] * array(index3[0], index1[1], index1[2]) +
              zweights[2] * array(index3[0], index1[1], index2[2]) +
              zweights[3] * array(index3[0], index1[1], index3[2])
            ) +
            yweights[2] * (
              zweights[0] * array(index3[0], index2[1], index0[2]) +
              zweights[1] * array(index3[0], index2[1], index1[2]) +
              zweights[2] * array(index3[0], index2[1], index2[2]) +
              zweights[3] * array(index3[0], index2[1], index3[2])
            ) +
            yweights[3] * (
              zweights[0] * array(index3[0], index3[1], index0[2]) +
              zweights[1] * array(index3[0], index3[1], index1[2]) +
              zweights[2] * array(index3[0], index3[1], index2[2]) +
              zweights[3] * array(index3[0], index3[1], index3[2])
            )
          );
  }
};

}

#endif
