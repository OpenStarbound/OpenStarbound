#pragma once

#include "StarAlgorithm.hpp"

namespace Star {

// any and all

template <typename Iterator, typename Functor>
bool any(Iterator iterBegin, Iterator iterEnd, Functor const& f) {
  for (; iterBegin != iterEnd; iterBegin++)
    if (f(*iterBegin))
      return true;
  return false;
}

template <typename Iterator>
bool any(Iterator const& iterBegin, Iterator const& iterEnd) {
  typedef typename std::iterator_traits<Iterator>::value_type IteratorValue;
  std::function<bool(IteratorValue)> compare = [](IteratorValue const& i) { return (bool)i; };
  return any(iterBegin, iterEnd, compare);
}

template <typename Iterable, typename Functor>
bool any(Iterable const& iter, Functor const& f) {
  return any(std::begin(iter), std::end(iter), f);
}

template <typename Iterable>
bool any(Iterable const& iter) {
  typedef decltype(*std::begin(iter)) IteratorValue;
  std::function<bool(IteratorValue)> compare = [](IteratorValue const& i) { return (bool)i; };
  return any(std::begin(iter), std::end(iter), compare);
}

template <typename Iterator, typename Functor>
bool all(Iterator iterBegin, Iterator iterEnd, Functor const& f) {
  for (; iterBegin != iterEnd; iterBegin++)
    if (!f(*iterBegin))
      return false;
  return true;
}

template <typename Iterator>
bool all(Iterator const& iterBegin, Iterator const& iterEnd) {
  typedef typename std::iterator_traits<Iterator>::value_type IteratorValue;
  std::function<bool(IteratorValue)> compare = [](IteratorValue const& i) { return (bool)i; };
  return all(iterBegin, iterEnd, compare);
}

template <typename Iterable, typename Functor>
bool all(Iterable const& iter, Functor const& f) {
  return all(std::begin(iter), std::end(iter), f);
}

template <typename Iterable>
bool all(Iterable const& iter) {
  typedef decltype(*std::begin(iter)) IteratorValue;
  std::function<bool(IteratorValue)> compare = [](IteratorValue const& i) { return (bool)i; };
  return all(std::begin(iter), std::end(iter), compare);
}

// Python style container slicing

struct SliceIndex {
  SliceIndex() : index(0), given(false) {}
  SliceIndex(int i) : index(i), given(true) {}

  int index;
  bool given;
};

SliceIndex const SliceNil = SliceIndex();

// T must have operator[](int), size(), and
// push_back(typeof T::operator[](int()))
template <typename Res, typename In>
Res slice(In const& r, SliceIndex a, SliceIndex b = SliceIndex(), int j = 1) {
  int size = (int)r.size();
  int start, end;

  // Throw exception on j == 0?
  if (j == 0 || size == 0)
    return Res();

  if (!a.given) {
    if (j > 0)
      start = 0;
    else
      start = size - 1;
  } else if (a.index < 0) {
    if (-a.index > size - 1)
      start = 0;
    else
      start = size - -a.index;
  } else {
    if (a.index > size)
      start = size;
    else
      start = a.index;
  }

  if (!b.given) {
    if (j > 0)
      end = size;
    else
      end = -1;
  } else if (b.index < 0) {
    if (-b.index > size - 1) {
      end = -1;
    } else {
      end = size - -b.index;
    }
  } else {
    if (b.index > size - 1) {
      end = size;
    } else {
      end = b.index;
    }
  }

  if (start < end && j < 0)
    return Res();
  if (start > end && j > 0)
    return Res();

  Res returnSlice;
  int i;
  for (i = start; i < end; i += j)
    returnSlice.push_back(r[i]);

  return returnSlice;
}

template <typename T>
T slice(T const& r, SliceIndex a, SliceIndex b = SliceIndex(), int j = 1) {
  return slice<T, T>(r, a, b, j);
}

// ZIP

// Wraps a regular iterator and returns a singleton tuple, as well as
// supporting the iterator protocol that the zip iterator code expects.
template <typename IteratorT>
class ZipWrapperIterator {
private:
  IteratorT current;
  IteratorT last;
  bool atEnd;

public:
  typedef IteratorT Iterator;
  typedef decltype(*std::declval<Iterator>()) IteratorValue;
  typedef tuple<IteratorValue> value_type;

  ZipWrapperIterator() : atEnd(true) {}

  ZipWrapperIterator(Iterator current, Iterator last) : current(current), last(last) {
    atEnd = current == last;
  }

  ZipWrapperIterator operator++() {
    if (!atEnd) {
      ++current;
      atEnd = current == last;
    }

    return *this;
  }

  value_type operator*() const {
    return std::tuple<IteratorValue>(*current);
  }

  bool operator==(ZipWrapperIterator const& rhs) const {
    return (atEnd && rhs.atEnd) || (!atEnd && !rhs.atEnd && current == rhs.current && last == rhs.last);
  }

  bool operator!=(ZipWrapperIterator const& rhs) const {
    return !(*this == rhs);
  }

  explicit operator bool() const {
    return !atEnd;
  }

  ZipWrapperIterator begin() const {
    return *this;
  }

  ZipWrapperIterator end() const {
    return ZipWrapperIterator();
  }
};
template <typename IteratorT>
ZipWrapperIterator<IteratorT> makeZipWrapperIterator(IteratorT current, IteratorT end) {
  return ZipWrapperIterator<IteratorT>(current, end);
}

// Takes two ZipIterators / ZipTupleIterators and concatenates them into a
// single iterator that returns the concatenated tuple.
template <typename TailIteratorT, typename HeadIteratorT>
class ZipTupleIterator {
private:
  TailIteratorT tailIterator;
  HeadIteratorT headIterator;
  bool atEnd;

public:
  typedef TailIteratorT TailIterator;
  typedef HeadIteratorT HeadIterator;

  typedef decltype(*TailIterator()) TailType;
  typedef decltype(*HeadIterator()) HeadType;

  typedef decltype(std::tuple_cat(std::declval<TailType>(), std::declval<HeadType>())) value_type;

  ZipTupleIterator() : atEnd(true) {}

  ZipTupleIterator(TailIterator tailIterator, HeadIterator headIterator)
    : tailIterator(tailIterator), headIterator(headIterator) {
    atEnd = tailIterator == TailIterator() || headIterator == HeadIterator();
  }

  ZipTupleIterator operator++() {
    if (!atEnd) {
      ++tailIterator;
      ++headIterator;
      atEnd = tailIterator == TailIterator() || headIterator == HeadIterator();
    }

    return *this;
  }

  value_type operator*() const {
    return std::tuple_cat(*tailIterator, *headIterator);
  }

  bool operator==(ZipTupleIterator const& rhs) const {
    return (atEnd && rhs.atEnd)
        || (!atEnd && !rhs.atEnd && tailIterator == rhs.tailIterator && headIterator == rhs.headIterator);
  }

  bool operator!=(ZipTupleIterator const& rhs) const {
    return !(*this == rhs);
  }

  explicit operator bool() const {
    return !atEnd;
  }

  ZipTupleIterator begin() const {
    return *this;
  }

  ZipTupleIterator end() const {
    return ZipTupleIterator();
  }
};
template <typename HeadIteratorT, typename TailIteratorT>
ZipTupleIterator<HeadIteratorT, TailIteratorT> makeZipTupleIterator(HeadIteratorT head, TailIteratorT tail) {
  return ZipTupleIterator<HeadIteratorT, TailIteratorT>(head, tail);
}

template <typename Container, typename... Rest>
struct zipIteratorReturn {
  typedef ZipTupleIterator<typename zipIteratorReturn<Container>::type, typename zipIteratorReturn<Rest...>::type> type;
};

template <typename Container>
struct zipIteratorReturn<Container> {
  typedef ZipWrapperIterator<decltype(std::declval<Container>().begin())> type;
};

template <typename Container>
typename zipIteratorReturn<Container>::type zipIterator(Container& container) {
  return makeZipWrapperIterator(container.begin(), container.end());
}

template <typename Container, typename... Rest>
typename zipIteratorReturn<Container, Rest...>::type zipIterator(Container& container, Rest&... rest) {
  return makeZipTupleIterator(makeZipWrapperIterator(container.begin(), container.end()), zipIterator(rest...));
}

// END ZIP

// RANGE

namespace RangeHelper {

  template <typename Diff>
  typename std::enable_if<std::is_unsigned<Diff>::value, bool>::type checkIfDiffLessThanZero(Diff) {
    return false;
  }

  template <typename Diff>
  typename std::enable_if<!std::is_unsigned<Diff>::value, bool>::type checkIfDiffLessThanZero(Diff diff) {
    return diff < 0;
  }
}

STAR_EXCEPTION(RangeException, StarException);

template <typename Value, typename Diff = int>
class RangeIterator {
  using iterator_category = std::random_access_iterator_tag;
  using value_type = Value;
  using difference_type = Diff;
  using pointer = Value*;
  using reference = Value&;

public:
  RangeIterator() : m_start(), m_end(), m_diff(1), m_current(), m_stop(true) {}

  RangeIterator(Value min, Value max, Diff diff)
    : m_start(min), m_end(max), m_diff(diff), m_current(min), m_stop(false) {
    sanity();
  }

  RangeIterator(Value min, Value max) : m_start(min), m_end(max), m_diff(1), m_current(min), m_stop(false) {
    sanity();
  }

  RangeIterator(Value max) : m_start(), m_end(max), m_diff(1), m_current(), m_stop(false) {
    sanity();
  }

  RangeIterator(RangeIterator const& rhs) {
    copy(rhs);
  }

  RangeIterator& operator=(RangeIterator const& rhs) {
    copy(rhs);
    return *this;
  }

  RangeIterator& operator+=(Diff steps) {
    if ((applySteps(m_current, m_diff * steps) >= m_end) != (RangeHelper::checkIfDiffLessThanZero<Diff>(m_diff))) {
      if (!m_stop) {
        Diff stepsLeft = stepsBetween(m_current, m_end);
        m_current = applySteps(m_current, stepsLeft * m_diff);
        m_stop = true;
      }
    } else {
      m_current = applySteps(m_current, steps * m_diff);
    }
    return *this;
  }

  RangeIterator operator-=(Diff steps) {
    m_stop = false;
    sanity();

    if (applySteps(m_current, -(m_diff * steps)) < m_start)
      m_current = m_start;
    else
      m_current = applySteps(m_current, -(m_diff * steps));

    return *this;
  }

  Value operator*() const {
    return m_current;
  }

  Value const* operator->() const {
    return &m_current;
  }

  Value operator[](unsigned rhs) const {
    // Should return at maximum, the value that this iterator will normally
    // reach when at end().
    rhs = std::min(rhs, stepsBetween(m_start, m_end) + 1);
    return m_start + rhs * m_diff;
  }

  RangeIterator& operator++() {
    return operator+=(1);
  }

  RangeIterator& operator--() {
    return operator-=(1);
  }

  RangeIterator operator++(int) {
    RangeIterator tmp(*this);
    ++(*this);
    return tmp;
  }

  RangeIterator operator--(int) {
    RangeIterator tmp(*this);
    --(*this);
    return tmp;
  }

  RangeIterator operator+(Diff steps) const {
    RangeIterator copy(*this);
    copy += steps;
    return copy;
  }

  RangeIterator operator-(Diff steps) const {
    RangeIterator copy(*this);
    copy -= steps;
    return copy;
  }

  int operator-(RangeIterator const& rhs) const {
    if (!sameClass(rhs))
      throw RangeException("Attempted to subtract incompatible ranges.");

    return stepsBetween(rhs.m_current, m_current);
  }

  friend RangeIterator operator+(Diff lhs, RangeIterator const& rhs) {
    return rhs + lhs;
  }

  friend RangeIterator operator-(Diff lhs, RangeIterator const& rhs) {
    return rhs - lhs;
  }

  bool operator==(RangeIterator const& rhs) const {
    return (sameClass(rhs) && m_current == rhs.m_current && m_stop == rhs.m_stop);
  }

  bool operator!=(RangeIterator const& rhs) const {
    return !(*this == rhs);
  }

  bool operator<(RangeIterator const& rhs) const {
    return std::tie(m_start, m_end, m_diff, m_current) < std::tie(rhs.m_start, rhs.m_end, rhs.m_diff, rhs.m_current);
  }

  bool operator<=(RangeIterator const& rhs) const {
    return (*this == rhs) || (*this < rhs);
  }

  bool operator>=(RangeIterator const& rhs) const {
    return !(*this < rhs);
  }

  bool operator>(RangeIterator const& rhs) const {
    return !(*this <= rhs);
  }

  RangeIterator begin() const {
    return RangeIterator(m_start, m_end, m_diff);
  }

  RangeIterator end() const {
    Diff steps = stepsBetween(m_start, m_end);
    RangeIterator res(m_start, m_end, m_diff);
    res += steps;
    return res;
  }

private:
  void copy(RangeIterator const& copy) {
    m_start = copy.m_start;
    m_end = copy.m_end;
    m_diff = copy.m_diff;
    m_current = copy.m_current;
    m_stop = copy.m_stop;
    sanity();
  }

  void sanity() {
    if (m_diff == 0)
      throw RangeException("Invalid difference in range function.");

    if ((m_end < m_start) != (RangeHelper::checkIfDiffLessThanZero<Diff>(m_diff))) {
      if (RangeHelper::checkIfDiffLessThanZero<Diff>(m_diff))
        throw RangeException("Start cannot be less than end if diff is negative.");
      throw RangeException("Max cannot be less than min.");
    }

    if (m_end == m_start)
      m_stop = true;
  }

  bool sameClass(RangeIterator const& rhs) const {
    return m_start == rhs.m_start && m_end == rhs.m_end && m_diff == rhs.m_diff;
  }

  Diff stepsBetween(Value start, Value end) const {
    return ((Diff)end - (Diff)start) / m_diff;
  }

  Value applySteps(Value start, Diff travel) const {
    return (Value)((Diff)start + travel);
  }

  Value m_start;
  Value m_end;
  Diff m_diff;

  Value m_current;

  bool m_stop;
};

template <typename Numeric, typename Diff>
RangeIterator<Numeric, Diff> range(Numeric min, Numeric max, Diff diff) {
  return RangeIterator<Numeric, Diff>(min, max, diff);
}

template <typename Numeric, typename Diff = int>
RangeIterator<Numeric, Diff> range(Numeric max) {
  return RangeIterator<Numeric, Diff>(max);
}

template <typename Numeric, typename Diff = int>
RangeIterator<Numeric, Diff> range(Numeric min, Numeric max) {
  return RangeIterator<Numeric, Diff>(min, max);
}

template <typename Numeric, typename Diff>
RangeIterator<Numeric, Diff> rangeInclusive(Numeric min, Numeric max, Diff diff) {
  return RangeIterator<Numeric, Diff>(min, (Numeric)((Diff)max + 1), diff);
}

template <typename Numeric, typename Diff = int>
RangeIterator<Numeric, Diff> rangeInclusive(Numeric max) {
  return RangeIterator<Numeric, Diff>((Numeric)((Diff)max + 1));
}

template <typename Numeric, typename Diff = int>
RangeIterator<Numeric, Diff> rangeInclusive(Numeric min, Numeric max) {
  return RangeIterator<Numeric, Diff>(min, (Numeric)((Diff)max + 1));
}

// END RANGE

// Wraps a forward-iterator to produce {value, index} pairs, similar to
// python's enumerate()
template <typename Iterator>
struct EnumerateIterator {
private:
  Iterator current;
  Iterator last;
  size_t index;
  bool atEnd;

public:
  typedef decltype(*std::declval<Iterator>()) IteratorValue;
  typedef pair<IteratorValue&, size_t> value_type;

  EnumerateIterator() : index(0), atEnd(true) {}

  EnumerateIterator(Iterator begin, Iterator end) : current(begin), last(end), index(0) {
    atEnd = current == last;
  }

  EnumerateIterator begin() const {
    return *this;
  }

  EnumerateIterator end() const {
    return EnumerateIterator();
  }

  EnumerateIterator operator++() {
    if (!atEnd) {
      ++current;
      ++index;

      atEnd = current == last;
    }

    return *this;
  }

  value_type operator*() const {
    return {*current, index};
  }

  bool operator==(EnumerateIterator const& rhs) const {
    return (atEnd && rhs.atEnd) || (!atEnd && !rhs.atEnd && current == rhs.current && last == rhs.last);
  }

  bool operator!=(EnumerateIterator const& rhs) const {
    return !(*this == rhs);
  }

  explicit operator bool() const {
    return !atEnd;
  }
};

template <typename Iterable>
EnumerateIterator<decltype(std::declval<Iterable>().begin())> enumerateIterator(Iterable& list) {
  return EnumerateIterator<decltype(std::declval<Iterable>().begin())>(list.begin(), list.end());
}

template <typename ResultContainer, typename Iterable>
ResultContainer enumerateConstruct(Iterable&& list) {
  ResultContainer res;
  for (auto el : enumerateIterator(list))
    res.push_back(std::move(el));

  return res;
}

}
