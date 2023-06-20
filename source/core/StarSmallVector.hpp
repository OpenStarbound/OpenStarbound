#ifndef STAR_SMALL_VECTOR_HPP
#define STAR_SMALL_VECTOR_HPP

#include "StarAlgorithm.hpp"

namespace Star {

// A vector that is stack allocated up to a maximum size, becoming heap
// allocated when it grows beyond that size.  Always takes up stack space of
// MaxStackSize * sizeof(Element).
template <typename Element, size_t MaxStackSize>
class SmallVector {
public:
  typedef Element* iterator;
  typedef Element const* const_iterator;

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef Element value_type;

  typedef Element& reference;
  typedef Element const& const_reference;

  SmallVector();
  SmallVector(SmallVector const& other);
  SmallVector(SmallVector&& other);
  template <typename OtherElement, size_t OtherMaxStackSize>
  SmallVector(SmallVector<OtherElement, OtherMaxStackSize> const& other);
  template <class Iterator>
  SmallVector(Iterator first, Iterator last);
  SmallVector(size_t size, Element const& value = Element());
  SmallVector(initializer_list<Element> list);
  ~SmallVector();

  SmallVector& operator=(SmallVector const& other);
  SmallVector& operator=(SmallVector&& other);
  SmallVector& operator=(std::initializer_list<Element> list);

  size_t size() const;
  bool empty() const;
  void resize(size_t size, Element const& e = Element());
  void reserve(size_t capacity);

  reference at(size_t i);
  const_reference at(size_t i) const;

  reference operator[](size_t i);
  const_reference operator[](size_t i) const;

  const_iterator begin() const;
  const_iterator end() const;

  iterator begin();
  iterator end();

  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;

  reverse_iterator rbegin();
  reverse_iterator rend();

  // Pointer to internal data, always valid even if empty.
  Element const* ptr() const;
  Element* ptr();

  void push_back(Element e);
  void pop_back();

  iterator insert(iterator pos, Element e);
  template <typename Iterator>
  iterator insert(iterator pos, Iterator begin, Iterator end);
  iterator insert(iterator pos, initializer_list<Element> list);

  template <typename... Args>
  void emplace(iterator pos, Args&&... args);

  template <typename... Args>
  void emplace_back(Args&&... args);

  void clear();

  iterator erase(iterator pos);
  iterator erase(iterator begin, iterator end);

  bool operator==(SmallVector const& other) const;
  bool operator!=(SmallVector const& other) const;
  bool operator<(SmallVector const& other) const;

private:
  typename std::aligned_storage<MaxStackSize * sizeof(Element), alignof(Element)>::type m_stackElements;

  bool isHeapAllocated() const;

  Element* m_begin;
  Element* m_end;
  Element* m_capacity;
};

template <typename Element, size_t MaxStackSize>
SmallVector<Element, MaxStackSize>::SmallVector() {
  m_begin = (Element*)&m_stackElements;
  m_end = m_begin;
  m_capacity = m_begin + MaxStackSize;
}

template <typename Element, size_t MaxStackSize>
SmallVector<Element, MaxStackSize>::~SmallVector() {
  clear();
  if (isHeapAllocated()) {
    free(m_begin, (m_capacity - m_begin) * sizeof(Element));
  }
}

template <typename Element, size_t MaxStackSize>
SmallVector<Element, MaxStackSize>::SmallVector(SmallVector const& other)
  : SmallVector() {
  insert(begin(), other.begin(), other.end());
}

template <typename Element, size_t MaxStackSize>
SmallVector<Element, MaxStackSize>::SmallVector(SmallVector&& other)
  : SmallVector() {
    for (auto& e : other)
      emplace_back(move(e));
}

template <typename Element, size_t MaxStackSize>
template <typename OtherElement, size_t OtherMaxStackSize>
SmallVector<Element, MaxStackSize>::SmallVector(SmallVector<OtherElement, OtherMaxStackSize> const& other)
  : SmallVector() {
    for (auto const& e : other)
      emplace_back(e);
}

template <typename Element, size_t MaxStackSize>
template <class Iterator>
SmallVector<Element, MaxStackSize>::SmallVector(Iterator first, Iterator last)
  : SmallVector() {
    insert(begin(), first, last);
}

template <typename Element, size_t MaxStackSize>
SmallVector<Element, MaxStackSize>::SmallVector(size_t size, Element const& value)
  : SmallVector() {
    resize(size, value);
}

template <typename Element, size_t MaxStackSize>
SmallVector<Element, MaxStackSize>::SmallVector(initializer_list<Element> list)
  : SmallVector() {
    for (auto const& e : list)
      emplace_back(e);
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::operator=(SmallVector const& other) -> SmallVector& {
  if (this == &other)
    return *this;

  resize(other.size());
  for (size_t i = 0; i < size(); ++i)
    operator[](i) = other[i];

  return *this;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::operator=(SmallVector&& other) -> SmallVector& {
  resize(other.size());
  for (size_t i = 0; i < size(); ++i)
    operator[](i) = move(other[i]);

  return *this;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::operator=(std::initializer_list<Element> list) -> SmallVector& {
  resize(list.size());
  for (size_t i = 0; i < size(); ++i)
    operator[](i) = move(list[i]);
  return *this;
}

template <typename Element, size_t MaxStackSize>
size_t SmallVector<Element, MaxStackSize>::size() const {
  return m_end - m_begin;
}

template <typename Element, size_t MaxStackSize>
bool SmallVector<Element, MaxStackSize>::empty() const {
  return m_begin == m_end;
}

template <typename Element, size_t MaxStackSize>
void SmallVector<Element, MaxStackSize>::resize(size_t size, Element const& e) {
  reserve(size);

  for (size_t i = this->size(); i > size; --i)
    pop_back();
  for (size_t i = this->size(); i < size; ++i)
    emplace_back(e);
}

template <typename Element, size_t MaxStackSize>
void SmallVector<Element, MaxStackSize>::reserve(size_t newCapacity) {
  size_t oldCapacity = m_capacity - m_begin;
  if (newCapacity > oldCapacity) {
    newCapacity = max(oldCapacity * 2, newCapacity);
    auto newMem = (Element*)Star::malloc(newCapacity * sizeof(Element));
    if (!newMem)
      throw MemoryException::format("Could not set new SmallVector capacity %s\n", newCapacity);

    size_t size = m_end - m_begin;
    auto oldMem = m_begin;
    auto oldHeapAllocated = isHeapAllocated();

    // We assume that move constructors can never throw.
    for (size_t i = 0; i < size; ++i) {
      new (&newMem[i]) Element(move(oldMem[i]));
    }

    m_begin = newMem;
    m_end = m_begin + size;
    m_capacity = m_begin + newCapacity;

    auto freeOldMem = finally([=]() {
      if (oldHeapAllocated)
          Star::free(oldMem, oldCapacity * sizeof(Element));
    });

    for (size_t i = 0; i < size; ++i) {
      oldMem[i].~Element();
    }
  }
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::at(size_t i) -> reference {
  if (i >= size())
    throw OutOfRangeException::format("out of range in SmallVector::at(%s)", i);
  return m_begin[i];
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::at(size_t i) const -> const_reference {
  if (i >= size())
    throw OutOfRangeException::format("out of range in SmallVector::at(%s)", i);
  return m_begin[i];
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::operator[](size_t i) -> reference {
  starAssert(i < size());
  return m_begin[i];
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::operator[](size_t i) const -> const_reference {
  starAssert(i < size());
  return m_begin[i];
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::begin() const -> const_iterator {
  return m_begin;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::end() const -> const_iterator {
  return m_end;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::begin() -> iterator {
  return m_begin;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::end() -> iterator {
  return m_end;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::rbegin() const -> const_reverse_iterator {
    return const_reverse_iterator(end());
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::rend() const -> const_reverse_iterator {
    return const_reverse_iterator(begin());
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::rbegin() -> reverse_iterator {
    return reverse_iterator(end());
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::rend() -> reverse_iterator {
    return reverse_iterator(begin());
}

template <typename Element, size_t MaxStackSize>
Element const* SmallVector<Element, MaxStackSize>::ptr() const {
  return m_begin;
}

template <typename Element, size_t MaxStackSize>
Element* SmallVector<Element, MaxStackSize>::ptr() {
  return m_begin;
}

template <typename Element, size_t MaxStackSize>
void SmallVector<Element, MaxStackSize>::push_back(Element e) {
  emplace_back(move(e));
}

template <typename Element, size_t MaxStackSize>
void SmallVector<Element, MaxStackSize>::pop_back() {
  if (m_begin == m_end)
    throw OutOfRangeException("SmallVector::pop_back called on empty SmallVector");
  --m_end;
  m_end->~Element();
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::insert(iterator pos, Element e) -> iterator {
  emplace(pos, move(e));
  return pos;
}

template <typename Element, size_t MaxStackSize>
template <typename Iterator>
auto SmallVector<Element, MaxStackSize>::insert(iterator pos, Iterator begin, Iterator end) -> iterator {
  size_t toAdd = std::distance(begin, end);
  size_t startIndex = pos - m_begin;
  size_t endIndex = startIndex + toAdd;
  size_t toShift = size() - startIndex;

  resize(size() + toAdd);

  for (size_t i = toShift; i != 0; --i)
    operator[](endIndex + i - 1) = move(operator[](startIndex + i - 1));

  for (size_t i = 0; i != toAdd; ++i)
    operator[](startIndex + i) = *begin++;

  return pos;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::insert(iterator pos, initializer_list<Element> list) -> iterator {
  return insert(pos, list.begin(), list.end());
}

template <typename Element, size_t MaxStackSize>
template <typename... Args>
void SmallVector<Element, MaxStackSize>::emplace(iterator pos, Args&&... args) {
  size_t index = pos - m_begin;
  emplace_back(Element());
  for (size_t i = size() - 1; i != index; --i)
    operator[](i) = move(operator[](i - 1));
  operator[](index) = Element(forward<Args>(args)...);
}

template <typename Element, size_t MaxStackSize>
template <typename... Args>
void SmallVector<Element, MaxStackSize>::emplace_back(Args&&... args) {
  if (m_end == m_capacity)
    reserve(size() + 1);
  new (m_end) Element(forward<Args>(args)...);
  ++m_end;
}

template <typename Element, size_t MaxStackSize>
void SmallVector<Element, MaxStackSize>::clear() {
  while (m_begin != m_end)
    pop_back();
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::erase(iterator pos) -> iterator {
  size_t index = pos - ptr();
  for (size_t i = index; i < size() - 1; ++i)
    operator[](i) = move(operator[](i + 1));
  pop_back();
  return pos;
}

template <typename Element, size_t MaxStackSize>
auto SmallVector<Element, MaxStackSize>::erase(iterator begin, iterator end) -> iterator {
  size_t startIndex = begin - ptr();
  size_t endIndex = end - ptr();
  size_t toRemove = endIndex - startIndex;
  for (size_t i = endIndex; i < size(); ++i)
    operator[](startIndex + (i - endIndex)) = move(operator[](i));
  resize(size() - toRemove);
  return begin;
}

template <typename Element, size_t MaxStackSize>
bool SmallVector<Element, MaxStackSize>::operator==(SmallVector const& other) const {
  if (this == &other)
    return true;

  if (size() != other.size())
    return false;

  for (size_t i = 0; i < size(); ++i) {
    if (operator[](i) != other[i])
      return false;
  }
  return true;
}

template <typename Element, size_t MaxStackSize>
bool SmallVector<Element, MaxStackSize>::operator!=(SmallVector const& other) const {
  return !operator==(other);
}

template <typename Element, size_t MaxStackSize>
bool SmallVector<Element, MaxStackSize>::operator<(SmallVector const& other) const {
  for (size_t i = 0; i < size(); ++i) {
    if (i >= other.size())
      return false;

    Element const& a = operator[](i);
    Element const& b = other[i];

    if (a < b)
      return true;
    else if (b < a)
      return false;
  }

  return size() < other.size();
}

template <typename Element, size_t MaxStackSize>
bool SmallVector<Element, MaxStackSize>::isHeapAllocated() const {
  return m_begin != (Element*)&m_stackElements;
}

}

#endif
