#pragma once

#include "StarInterpolation.hpp"

namespace Star {

// Describes a simple table from index to value, which operates on bins
// corresponding to ranges of indexes.  IndexType can be any ordered type,
// ValueType can be anything.
template <typename IndexType, typename ValueType = IndexType>
class ParametricTable {
public:
  typedef IndexType Index;
  typedef ValueType Value;

  ParametricTable();

  template <typename OtherIndexType, typename OtherValueType>
  explicit ParametricTable(ParametricTable<OtherIndexType, OtherValueType> const& parametricFunction);

  // Construct a ParametricTable with a list of point pairs, which does not
  // have to be sorted (it will be sorted internally).  Throws an exception on
  // duplicate index values.
  template <typename PairContainer>
  explicit ParametricTable(PairContainer indexValuePairs);

  // addPoint does not need to be called in order, it will insert the point in
  // the correct ordered position for the given index, and return the position.
  size_t addPoint(IndexType index, ValueType value);
  void clearPoints();

  size_t size() const;
  bool empty() const;

  IndexType const& index(size_t i) const;
  ValueType const& value(size_t i) const;

  // Returns true if the values of the table are also valid indexes (true when
  // the data points are monotonic increasing)
  bool isInvertible() const;

  // Invert the table, switching indexes and values.  Throws an exception if
  // the function is not invertible.  Will not generally compile unless the
  // Index and Value types are the same type.
  void invert() const;

  // Find the value to the left of the given index.  If the index is lower than
  // the lowest index point, returns the first value.
  ValueType const& get(IndexType index) const;

protected:
  typedef std::vector<IndexType> IndexList;
  typedef std::vector<ValueType> ValueList;

  IndexList const& indexes() const;
  ValueList const& values() const;

private:
  IndexList m_indexes;
  ValueList m_values;
};

// Extension of ParametricTable that simplifies of all the complex
// interpolation code for interpolating an ordered list of points.  Useful for
// describing a simple 2d or n-dimensional (using VectorN for value type) curve
// of one variable.  IndexType should generally be float or double, and
// ValueType can be any type that can be interpolated.
template <typename IndexType, typename ValueType = IndexType>
class ParametricFunction : public ParametricTable<IndexType, ValueType> {
public:
  typedef ParametricTable<IndexType, ValueType> Base;

  ParametricFunction(
      InterpolationMode interpolationMode = InterpolationMode::Linear, BoundMode boundMode = BoundMode::Clamp);

  template <typename OtherIndexType, typename OtherValueType>
  explicit ParametricFunction(ParametricFunction<OtherIndexType, OtherValueType> const& parametricFunction);

  template <typename PairContainer>
  explicit ParametricFunction(PairContainer indexValuePairs,
      InterpolationMode interpolationMode = InterpolationMode::Linear,
      BoundMode boundMode = BoundMode::Clamp);

  InterpolationMode interpolationMode() const;
  void setInterpolationMode(InterpolationMode interpolationType);

  BoundMode boundMode() const;
  void setBoundMode(BoundMode boundMode);

  // Interpolates a value at the given index according to the interpolation and
  // bound mode.
  ValueType interpolate(IndexType index) const;

  // Synonym for interpolate
  ValueType operator()(IndexType index) const;

private:
  InterpolationMode m_interpolationMode;
  BoundMode m_boundMode;
};

template <typename IndexType, typename ValueType>
ParametricTable<IndexType, ValueType>::ParametricTable() {}

template <typename IndexType, typename ValueType>
template <typename OtherIndexType, typename OtherValueType>
ParametricTable<IndexType, ValueType>::ParametricTable(
    ParametricTable<OtherIndexType, OtherValueType> const& parametricTable) {
  for (size_t i = 0; i < parametricTable.size(); ++i) {
    m_indexes.push_back(parametricTable.index(i));
    m_values.push_back(parametricTable.value(i));
  }
}

template <typename IndexType, typename ValueType>
template <typename PairContainer>
ParametricTable<IndexType, ValueType>::ParametricTable(PairContainer indexValuePairs) {
  if (indexValuePairs.empty())
    return;

  sort(indexValuePairs,
      [](typename PairContainer::value_type const& a, typename PairContainer::value_type const& b) {
        return std::get<0>(a) < std::get<0>(b);
      });

  for (auto const& pair : indexValuePairs) {
    m_indexes.push_back(std::move(std::get<0>(pair)));
    m_values.push_back(std::move(std::get<1>(pair)));
  }

  for (size_t i = 0; i < size() - 1; ++i) {
    if (m_indexes[i] == m_indexes[i + 1])
      throw MathException("Degenerate index values given in ParametricTable constructor");
  }
}

template <typename IndexType, typename ValueType>
size_t ParametricTable<IndexType, ValueType>::addPoint(IndexType index, ValueType value) {
  size_t insertLocation = std::distance(m_indexes.begin(), std::upper_bound(m_indexes.begin(), m_indexes.end(), index));
  m_indexes.insert(m_indexes.begin() + insertLocation, std::move(index));
  m_values.insert(m_values.begin() + insertLocation, std::move(value));
  return insertLocation;
}

template <typename IndexType, typename ValueType>
void ParametricTable<IndexType, ValueType>::clearPoints() {
  m_indexes.clear();
  m_values.clear();
}

template <typename IndexType, typename ValueType>
size_t ParametricTable<IndexType, ValueType>::size() const {
  return m_indexes.size();
}

template <typename IndexType, typename ValueType>
bool ParametricTable<IndexType, ValueType>::empty() const {
  return size() == 0;
}

template <typename IndexType, typename ValueType>
IndexType const& ParametricTable<IndexType, ValueType>::index(size_t i) const {
  return m_indexes.at(i);
}

template <typename IndexType, typename ValueType>
ValueType const& ParametricTable<IndexType, ValueType>::value(size_t i) const {
  return m_values.at(i);
}

template <typename IndexType, typename ValueType>
bool ParametricTable<IndexType, ValueType>::isInvertible() const {
  if (empty())
    return true;

  for (size_t i = 0; i < size() - 1; ++i) {
    if (m_values[i] > m_values[i + 1])
      return false;
  }

  return true;
}

template <typename IndexType, typename ValueType>
void ParametricTable<IndexType, ValueType>::invert() const {
  if (isInvertible())
    throw MathException("invert() called on non-invertible ParametricTable");

  for (size_t i = 0; i < size(); ++i)
    std::swap(m_indexes[i], m_values[i]);
}

template <typename IndexType, typename ValueType>
ValueType const& ParametricTable<IndexType, ValueType>::get(IndexType index) const {
  if (empty())
    throw MathException("get called on empty ParametricTable");

  auto i = std::lower_bound(m_indexes.begin(), m_indexes.end(), index);
  if (i != m_indexes.begin())
    --i;

  return m_values[std::distance(m_indexes.begin(), i)];
}

template <typename IndexType, typename ValueType>
auto ParametricTable<IndexType, ValueType>::indexes() const -> IndexList const & {
  return m_indexes;
}

template <typename IndexType, typename ValueType>
auto ParametricTable<IndexType, ValueType>::values() const -> ValueList const & {
  return m_values;
}

template <typename IndexType, typename ValueType>
ParametricFunction<IndexType, ValueType>::ParametricFunction(InterpolationMode interpolationMode, BoundMode boundMode)
  : m_interpolationMode(interpolationMode), m_boundMode(boundMode) {}

template <typename IndexType, typename ValueType>
template <typename OtherIndexType, typename OtherValueType>
ParametricFunction<IndexType, ValueType>::ParametricFunction(
    ParametricFunction<OtherIndexType, OtherValueType> const& parametricFunction)
  : Base(parametricFunction) {
  m_interpolationMode = parametricFunction.interpolationMode();
  m_boundMode = parametricFunction.boundMode();
}

template <typename IndexType, typename ValueType>
template <typename PairContainer>
ParametricFunction<IndexType, ValueType>::ParametricFunction(
    PairContainer indexValuePairs, InterpolationMode interpolationMode, BoundMode boundMode)
  : Base(indexValuePairs) {
  m_interpolationMode = interpolationMode;
  m_boundMode = boundMode;
}

template <typename IndexType, typename ValueType>
InterpolationMode ParametricFunction<IndexType, ValueType>::interpolationMode() const {
  return m_interpolationMode;
}

template <typename IndexType, typename ValueType>
void ParametricFunction<IndexType, ValueType>::setInterpolationMode(InterpolationMode interpolationType) {
  m_interpolationMode = interpolationType;
}

template <typename IndexType, typename ValueType>
BoundMode ParametricFunction<IndexType, ValueType>::boundMode() const {
  return m_boundMode;
}

template <typename IndexType, typename ValueType>
void ParametricFunction<IndexType, ValueType>::setBoundMode(BoundMode boundMode) {
  m_boundMode = boundMode;
}

template <typename IndexType, typename ValueType>
ValueType ParametricFunction<IndexType, ValueType>::interpolate(IndexType index) const {
  if (Base::empty())
    return ValueType();

  if (m_interpolationMode == InterpolationMode::HalfStep) {
    return parametricInterpolate2(Base::indexes(), Base::values(), index, StepWeightOperator<IndexType>(), m_boundMode);

  } else if (m_interpolationMode == InterpolationMode::Linear) {
    return parametricInterpolate2(
        Base::indexes(), Base::values(), index, LinearWeightOperator<IndexType>(), m_boundMode);

  } else if (m_interpolationMode == InterpolationMode::Cubic) {
    // ParametricFunction uses CubicWeights with linear extrapolation (not
    // configurable atm)
    return parametricInterpolate4(
        Base::indexes(), Base::values(), index, Cubic4WeightOperator<IndexType>(true), m_boundMode);

  } else {
    throw MathException("Unsupported interpolation type in ParametricFunction::interpolate");
  }
}

template <typename IndexType, typename ValueType>
ValueType ParametricFunction<IndexType, ValueType>::operator()(IndexType index) const {
  return interpolate(index);
}

}
