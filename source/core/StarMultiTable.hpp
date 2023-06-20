#ifndef STAR_MULTI_TABLE_HPP
#define STAR_MULTI_TABLE_HPP

#include "StarMultiArrayInterpolator.hpp"

namespace Star {

// Provides a method for storing, retrieving, and interpolating uneven
// n-variate data.  Access times involve a binary search over the domain of
// each dimension, so is O(log(n)*m) where n is the size of the largest
// dimension, and m is the table_rank.
template <typename ElementT, typename PositionT, size_t RankN>
class MultiTable {
public:
  typedef ElementT Element;
  typedef PositionT Position;
  static size_t const Rank = RankN;

  typedef Star::MultiArray<ElementT, RankN> MultiArray;

  typedef Star::MultiArrayInterpolator2<MultiArray, Position> Interpolator2;
  typedef Star::MultiArrayInterpolator4<MultiArray, Position> Interpolator4;
  typedef Star::MultiArrayPiecewiseInterpolator<MultiArray, Position> PiecewiseInterpolator;

  typedef Array<Position, Rank> PositionArray;
  typedef Array<Position, 2> WeightArray2;
  typedef Array<Position, 4> WeightArray4;
  typedef typename MultiArray::SizeArray SizeArray;
  typedef typename MultiArray::IndexArray IndexArray;
  typedef List<Position> Range;
  typedef Array<Range, Rank> RangeArray;

  typedef std::function<WeightArray2(Position)> WeightFunction2;
  typedef std::function<WeightArray4(Position)> WeightFunction4;
  typedef std::function<Element(PositionArray const&)> InterpolateFunction;

  MultiTable() : m_interpolationMode(InterpolationMode::Linear), m_boundMode(BoundMode::Clamp) {}

  // Set input ranges on a particular dimension.  Will resize underlying storage
  // to fit range.
  void setRange(std::size_t dim, Range const& range) {
    SizeArray sizes = m_array.size();
    sizes[dim] = range.size();
    m_array.resize(sizes);

    m_ranges[dim] = range;
  }

  void setRanges(RangeArray const& ranges) {
    SizeArray arraySize;

    for (size_t dim = 0; dim < Rank; ++dim) {
      arraySize[dim] = ranges[dim].size();
      m_ranges[dim] = ranges[dim];
    }

    m_array.resize(arraySize);
  }

  // Set array element based on index.
  void set(IndexArray const& index, Element const& element) {
    m_array.set(index, element);
  }

  // Get array element based on index.
  Element const& get(IndexArray const& index) const {
    return m_array(index);
  }

  MultiArray const& array() const {
    return m_array;
  }

  MultiArray& array() {
    return m_array;
  }

  InterpolationMode interpolationMode() const {
    return m_interpolationMode;
  }

  void setInterpolationMode(InterpolationMode interpolationMode) {
    m_interpolationMode = interpolationMode;
  }

  BoundMode boundMode() const {
    return m_boundMode;
  }

  void setBoundMode(BoundMode boundMode) {
    m_boundMode = boundMode;
  }

  Element interpolate(PositionArray const& coord) const {
    if (m_interpolationMode == InterpolationMode::HalfStep) {
      PiecewiseInterpolator piecewiseInterpolator(StepWeightOperator<Position>(), m_boundMode);
      return piecewiseInterpolator.interpolate(m_array, toIndexSpace(coord));

    } else if (m_interpolationMode == InterpolationMode::Linear) {
      Interpolator2 interpolator2(LinearWeightOperator<Position>(), m_boundMode);
      return interpolator2.interpolate(m_array, toIndexSpace(coord));

    } else if (m_interpolationMode == InterpolationMode::Cubic) {
      // MultiTable uses CubicWeights with linear extrapolation (not
      // configurable atm)
      Interpolator4 interpolator4(Cubic4WeightOperator<Position>(true), m_boundMode);
      return interpolator4.interpolate(m_array, toIndexSpace(coord));

    } else {
      throw MathException("Unsupported interpolation type in MultiTable::interpolate");
    }
  }

  // Synonym for inteprolate
  Element operator()(PositionArray const& coord) const {
    return interpolate(coord);
  }

  // op should take a PositionArray parameter and return an element.
  template <typename OpType>
  void eval(OpType op) {
    m_array.forEach(EvalWrapper<OpType>(op, *this));
  }

private:
  template <typename Coordinate>
  inline PositionArray toIndexSpace(Coordinate const& coord) const {
    PositionArray indexCoord;
    for (size_t i = 0; i < Rank; ++i)
      indexCoord[i] = inverseLinearInterpolateLower(m_ranges[i].begin(), m_ranges[i].end(), coord[i]);
    return indexCoord;
  }

  template <typename OpType>
  struct EvalWrapper {
    EvalWrapper(OpType& o, MultiTable const& t)
      : op(o), table(t) {}

    template <typename IndexArray>
    void operator()(IndexArray const& indexArray, Element& element) {
      PositionArray rangeArray;
      for (size_t i = 0; i < Rank; ++i)
        rangeArray[i] = table.m_ranges[i][indexArray[i]];

      element = op(rangeArray);
    }

    OpType& op;
    MultiTable const& table;
  };

  RangeArray m_ranges;
  MultiArray m_array;
  InterpolationMode m_interpolationMode;
  BoundMode m_boundMode;
};

typedef MultiTable<float, float, 2> MultiTable2F;
typedef MultiTable<double, double, 2> MultiTable2D;

typedef MultiTable<float, float, 3> MultiTable3F;
typedef MultiTable<double, double, 3> MultiTable3D;

typedef MultiTable<float, float, 4> MultiTable4F;
typedef MultiTable<double, double, 4> MultiTable4D;

}

#endif
