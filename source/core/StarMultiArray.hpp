#ifndef STAR_MULTI_ARRAY_HPP
#define STAR_MULTI_ARRAY_HPP

#include "StarArray.hpp"
#include "StarList.hpp"

namespace Star {

STAR_EXCEPTION(MultiArrayException, StarException);

// Multidimensional array class that wraps a vector as a simple contiguous N
// dimensional array.  Values are stored so that the highest dimension is the
// dimension with stride 0, and the lowest dimension has the largest stride.
//
// Due to usage of std::vector, ElementT = bool means that the user must use
// set() and get() rather than operator()
template <typename ElementT, size_t RankN>
class MultiArray {
public:
  typedef List<ElementT> Storage;

  typedef ElementT Element;
  static size_t const Rank = RankN;

  typedef Array<size_t, Rank> IndexArray;
  typedef Array<size_t, Rank> SizeArray;

  typedef typename Storage::iterator iterator;
  typedef typename Storage::const_iterator const_iterator;
  typedef Element value_type;

  MultiArray();
  template <typename... T>
  explicit MultiArray(size_t i, T... rest);
  explicit MultiArray(SizeArray const& shape);
  explicit MultiArray(SizeArray const& shape, Element const& c);

  SizeArray const& size() const;
  size_t size(size_t dimension) const;

  void clear();

  void resize(SizeArray const& shape);
  void resize(SizeArray const& shape, Element const& c);

  template <typename... T>
  void resize(size_t i, T... rest);

  void fill(Element const& element);

  // Does not preserve previous element position, array contents will be
  // invalid.
  void setSize(SizeArray const& shape);
  void setSize(SizeArray const& shape, Element const& c);

  template <typename... T>
  void setSize(size_t i, T... rest);

  Element& operator()(IndexArray const& index);
  Element const& operator()(IndexArray const& index) const;

  template <typename... T>
  Element& operator()(size_t i1, T... rest);
  template <typename... T>
  Element const& operator()(size_t i1, T... rest) const;

  // Throws exception if out of bounds
  Element& at(IndexArray const& index);
  Element const& at(IndexArray const& index) const;

  template <typename... T>
  Element& at(size_t i1, T... rest);
  template <typename... T>
  Element const& at(size_t i1, T... rest) const;

  // Throws an exception of out of bounds
  void set(IndexArray const& index, Element element);

  // Returns default element if out of bounds.
  Element get(IndexArray const& index, Element def = Element());

  // Auto-resizes array if out of bounds
  void setResize(IndexArray const& index, Element element);

  // Copy the given array element for element into this array.  The shape of
  // this array must be at least as large in every dimension as the source
  // array
  void copy(MultiArray const& source);
  void copy(MultiArray const& source, IndexArray const& sourceMin, IndexArray const& sourceMax, IndexArray const& targetMin);

  // op will be called with IndexArray and Element parameters.
  template <typename OpType>
  void forEach(IndexArray const& min, SizeArray const& size, OpType&& op);
  template <typename OpType>
  void forEach(IndexArray const& min, SizeArray const& size, OpType&& op) const;

  // Shortcut for calling forEach on the entire array
  template <typename OpType>
  void forEach(OpType&& op);
  template <typename OpType>
  void forEach(OpType&& op) const;

  template <typename OStream>
  void print(OStream& os) const;

  // Api for more direct access to elements.

  size_t count() const;

  Element const& atIndex(size_t index) const;
  Element& atIndex(size_t index);

  Element const* data() const;
  Element* data();

private:
  size_t storageIndex(IndexArray const& index) const;

  template <typename OStream>
  void subPrint(OStream& os, IndexArray index, size_t dim) const;

  template <typename OpType>
  void subForEach(IndexArray const& min, SizeArray const& size, OpType&& op, IndexArray& index, size_t offset, size_t dim) const;

  template <typename OpType>
  void subForEach(IndexArray const& min, SizeArray const& size, OpType&& op, IndexArray& index, size_t offset, size_t dim);

  void subCopy(MultiArray const& source, IndexArray const& sourceMin, IndexArray const& sourceMax,
      IndexArray const& targetMin, IndexArray& sourceIndex, IndexArray& targetIndex, size_t dim);

  Storage m_data;
  SizeArray m_shape;
};

typedef MultiArray<int, 2> MultiArray2I;
typedef MultiArray<size_t, 2> MultiArray2S;
typedef MultiArray<unsigned, 2> MultiArray2U;
typedef MultiArray<float, 2> MultiArray2F;
typedef MultiArray<double, 2> MultiArray2D;

typedef MultiArray<int, 3> MultiArray3I;
typedef MultiArray<size_t, 3> MultiArray3S;
typedef MultiArray<unsigned, 3> MultiArray3U;
typedef MultiArray<float, 3> MultiArray3F;
typedef MultiArray<double, 3> MultiArray3D;

typedef MultiArray<int, 4> MultiArray4I;
typedef MultiArray<size_t, 4> MultiArray4S;
typedef MultiArray<unsigned, 4> MultiArray4U;
typedef MultiArray<float, 4> MultiArray4F;
typedef MultiArray<double, 4> MultiArray4D;

template <typename Element, size_t Rank>
std::ostream& operator<<(std::ostream& os, MultiArray<Element, Rank> const& array);

template <typename Element, size_t Rank>
MultiArray<Element, Rank>::MultiArray() {
  m_shape = SizeArray::filled(0);
}

template <typename Element, size_t Rank>
MultiArray<Element, Rank>::MultiArray(SizeArray const& shape) {
  setSize(shape);
}

template <typename Element, size_t Rank>
MultiArray<Element, Rank>::MultiArray(SizeArray const& shape, Element const& c) {
  setSize(shape, c);
}

template <typename Element, size_t Rank>
template <typename... T>
MultiArray<Element, Rank>::MultiArray(size_t i, T... rest) {
  setSize(SizeArray{i, rest...});
}

template <typename Element, size_t Rank>
typename MultiArray<Element, Rank>::SizeArray const& MultiArray<Element, Rank>::size() const {
  return m_shape;
}

template <typename Element, size_t Rank>
size_t MultiArray<Element, Rank>::size(size_t dimension) const {
  return m_shape[dimension];
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::clear() {
  setSize(SizeArray::filled(0));
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::resize(SizeArray const& shape) {
  if (m_data.empty()) {
    setSize(shape);
    return;
  }

  bool equal = true;
  for (size_t i = 0; i < Rank; ++i)
    equal = equal && (m_shape[i] == shape[i]);

  if (equal)
    return;

  MultiArray newArray(shape);
  newArray.copy(*this);
  std::swap(*this, newArray);
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::resize(SizeArray const& shape, Element const& c) {
  if (m_data.empty()) {
    setSize(shape, c);
    return;
  }

  bool equal = true;
  for (size_t i = 0; i < Rank; ++i)
    equal = equal && (m_shape[i] == shape[i]);

  if (equal)
    return;

  MultiArray newArray(shape, c);
  newArray.copy(*this);
  *this = std::move(newArray);
}

template <typename Element, size_t Rank>
template <typename... T>
void MultiArray<Element, Rank>::resize(size_t i, T... rest) {
  resize(SizeArray{i, rest...});
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::fill(Element const& element) {
  std::fill(m_data.begin(), m_data.end(), element);
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::setSize(SizeArray const& shape) {
  size_t storageSize = 1;
  for (size_t i = 0; i < Rank; ++i) {
    m_shape[i] = shape[i];
    storageSize *= shape[i];
  }

  m_data.resize(storageSize);
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::setSize(SizeArray const& shape, Element const& c) {
  size_t storageSize = 1;
  for (size_t i = 0; i < Rank; ++i) {
    m_shape[i] = shape[i];
    storageSize *= shape[i];
  }
  m_data.resize(storageSize, c);
}

template <typename Element, size_t Rank>
template <typename... T>
void MultiArray<Element, Rank>::setSize(size_t i, T... rest) {
  setSize({i, rest...});
}

template <typename Element, size_t Rank>
Element& MultiArray<Element, Rank>::operator()(IndexArray const& index) {
  return m_data[storageIndex(index)];
}

template <typename Element, size_t Rank>
Element const& MultiArray<Element, Rank>::operator()(IndexArray const& index) const {
  return m_data[storageIndex(index)];
}

template <typename Element, size_t Rank>
template <typename... T>
Element& MultiArray<Element, Rank>::operator()(size_t i1, T... rest) {
  return m_data[storageIndex(IndexArray(i1, rest...))];
}

template <typename Element, size_t Rank>
template <typename... T>
Element const& MultiArray<Element, Rank>::operator()(size_t i1, T... rest) const {
  return m_data[storageIndex(IndexArray(i1, rest...))];
}

template <typename Element, size_t Rank>
Element const& MultiArray<Element, Rank>::at(IndexArray const& index) const {
  for (size_t i = Rank; i != 0; --i) {
    if (index[i - 1] >= m_shape[i - 1])
      throw MultiArrayException(strf("Out of bounds on MultiArray::at({})", index));
  }

  return m_data[storageIndex(index)];
}

template <typename Element, size_t Rank>
Element& MultiArray<Element, Rank>::at(IndexArray const& index) {
  for (size_t i = Rank; i != 0; --i) {
    if (index[i - 1] >= m_shape[i - 1])
      throw MultiArrayException(strf("Out of bounds on MultiArray::at({})", index));
  }

  return m_data[storageIndex(index)];
}

template <typename Element, size_t Rank>
template <typename... T>
Element& MultiArray<Element, Rank>::at(size_t i1, T... rest) {
  return at(IndexArray(i1, rest...));
}

template <typename Element, size_t Rank>
template <typename... T>
Element const& MultiArray<Element, Rank>::at(size_t i1, T... rest) const {
  return at(IndexArray(i1, rest...));
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::set(IndexArray const& index, Element element) {
  for (size_t i = Rank; i != 0; --i) {
    if (index[i - 1] >= m_shape[i - 1])
      throw MultiArrayException(strf("Out of bounds on MultiArray::set({})", index));
  }

  m_data[storageIndex(index)] = move(element);
}

template <typename Element, size_t Rank>
Element MultiArray<Element, Rank>::get(IndexArray const& index, Element def) {
  for (size_t i = Rank; i != 0; --i) {
    if (index[i - 1] >= m_shape[i - 1])
      return move(def);
  }

  return m_data[storageIndex(index)];
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::setResize(IndexArray const& index, Element element) {
  SizeArray newShape;
  for (size_t i = 0; i < Rank; ++i)
    newShape[i] = std::max(m_shape[i], index[i] + 1);
  resize(newShape);

  m_data[storageIndex(index)] = move(element);
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::copy(MultiArray const& source) {
  IndexArray max;
  for (size_t i = 0; i < Rank; ++i)
    max[i] = std::min(size(i), source.size(i));

  copy(source, IndexArray::filled(0), max, IndexArray::filled(0));
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::copy(MultiArray const& source, IndexArray const& sourceMin, IndexArray const& sourceMax, IndexArray const& targetMin) {
  IndexArray sourceIndex;
  IndexArray targetIndex;
  subCopy(source, sourceMin, sourceMax, targetMin, sourceIndex, targetIndex, 0);
}

template <typename Element, size_t Rank>
template <typename OpType>
void MultiArray<Element, Rank>::forEach(IndexArray const& min, SizeArray const& size, OpType&& op) {
  IndexArray index;
  subForEach(min, size, forward<OpType>(op), index, 0, 0);
}

template <typename Element, size_t Rank>
template <typename OpType>
void MultiArray<Element, Rank>::forEach(IndexArray const& min, SizeArray const& size, OpType&& op) const {
  IndexArray index;
  subForEach(min, size, forward<OpType>(op), index, 0, 0);
}

template <typename Element, size_t Rank>
template <typename OpType>
void MultiArray<Element, Rank>::forEach(OpType&& op) {
  forEach(IndexArray::filled(0), size(), forward<OpType>(op));
}

template <typename Element, size_t Rank>
template <typename OpType>
void MultiArray<Element, Rank>::forEach(OpType&& op) const {
  forEach(IndexArray::filled(0), size(), forward<OpType>(op));
}

template <typename Element, size_t Rank>
template <typename OStream>
void MultiArray<Element, Rank>::print(OStream& os) const {
  subPrint(os, IndexArray(), 0);
}

template <typename Element, size_t Rank>
size_t MultiArray<Element, Rank>::count() const {
  return m_data.size();
}

template <typename Element, size_t Rank>
Element const& MultiArray<Element, Rank>::atIndex(size_t index) const {
  return m_data[index];
}

template <typename Element, size_t Rank>
Element& MultiArray<Element, Rank>::atIndex(size_t index) {
  return m_data[index];
}

template <typename Element, size_t Rank>
Element const* MultiArray<Element, Rank>::data() const {
  return m_data.ptr();
}

template <typename Element, size_t Rank>
Element* MultiArray<Element, Rank>::data() {
  return m_data.ptr();
}

template <typename Element, size_t Rank>
size_t MultiArray<Element, Rank>::storageIndex(IndexArray const& index) const {
  size_t loc = index[0];
  starAssert(index[0] < m_shape[0]);
  for (size_t i = 1; i < Rank; ++i) {
    loc = loc * m_shape[i] + index[i];
    starAssert(index[i] < m_shape[i]);
  }
  return loc;
}

template <typename Element, size_t Rank>
template <typename OStream>
void MultiArray<Element, Rank>::subPrint(OStream& os, IndexArray index, size_t dim) const {
  if (dim == Rank - 1) {
    for (size_t i = 0; i < m_shape[dim]; ++i) {
      index[dim] = i;
      os << m_data[storageIndex(index)] << ' ';
    }
    os << std::endl;
  } else {
    for (size_t i = 0; i < m_shape[dim]; ++i) {
      index[dim] = i;
      subPrint(os, index, dim + 1);
    }
    os << std::endl;
  }
}

template <typename Element, size_t Rank>
template <typename OpType>
void MultiArray<Element, Rank>::subForEach(IndexArray const& min, SizeArray const& size, OpType&& op, IndexArray& index, size_t offset, size_t dim) {
  size_t minIndex = min[dim];
  size_t maxIndex = minIndex + size[dim];
  for (size_t i = minIndex; i < maxIndex; ++i) {
    index[dim] = i;
    if (dim == Rank - 1)
      op(index, m_data[offset + i]);
    else
      subForEach(min, size, forward<OpType>(op), index, (offset + i) * m_shape[dim + 1], dim + 1);
  }
}

template <typename Element, size_t Rank>
template <typename OpType>
void MultiArray<Element, Rank>::subForEach(IndexArray const& min, SizeArray const& size, OpType&& op, IndexArray& index, size_t offset, size_t dim) const {
  size_t minIndex = min[dim];
  size_t maxIndex = minIndex + size[dim];
  for (size_t i = minIndex; i < maxIndex; ++i) {
    index[dim] = i;
    if (dim == Rank - 1)
      op(index, m_data[offset + i]);
    else
      subForEach(min, size, forward<OpType>(op), index, (offset + i) * m_shape[dim + 1], dim + 1);
  }
}

template <typename Element, size_t Rank>
void MultiArray<Element, Rank>::subCopy(MultiArray const& source, IndexArray const& sourceMin, IndexArray const& sourceMax,
    IndexArray const& targetMin, IndexArray& sourceIndex, IndexArray& targetIndex, size_t dim) {
  size_t w = sourceMax[dim] - sourceMin[dim];
  if (dim < Rank - 1) {
    for (size_t i = 0; i < w; ++i) {
      sourceIndex[dim] = i + sourceMin[dim];
      targetIndex[dim] = i + targetMin[dim];
      subCopy(source, sourceMin, sourceMax, targetMin, sourceIndex, targetIndex, dim + 1);
    }
  } else {
    sourceIndex[dim] = sourceMin[dim];
    targetIndex[dim] = targetMin[dim];
    size_t sourceStorageStart = source.storageIndex(sourceIndex);
    size_t targetStorageStart = storageIndex(targetIndex);
    for (size_t i = 0; i < w; ++i)
      m_data[targetStorageStart + i] = source.m_data[sourceStorageStart + i];
  }
}

template <typename Element, size_t Rank>
std::ostream& operator<<(std::ostream& os, MultiArray<Element, Rank> const& array) {
  array.print(os);
  return os;
}

}

template <typename Element, size_t Rank>
struct fmt::formatter<Star::MultiArray<Element, Rank>> : ostream_formatter {};

#endif
