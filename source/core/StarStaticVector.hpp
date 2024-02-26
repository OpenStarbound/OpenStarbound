#pragma once

#include "StarException.hpp"
#include "StarFormat.hpp"

namespace Star {

STAR_EXCEPTION(StaticVectorSizeException, StarException);

// Stack allocated vector of elements with a dynamic size which must be less
// than a given maximum.  Acts like a vector with a built-in allocator of a
// maximum size, throws bad_alloc on attempting to resize beyond the maximum
// size.
template <typename Element, size_t MaxSize>
class StaticVector {
public:
  typedef Element* iterator;
  typedef Element const* const_iterator;

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef Element value_type;

  typedef Element& reference;
  typedef Element const& const_reference;

  static constexpr size_t MaximumSize = MaxSize;

  StaticVector();
  StaticVector(StaticVector const& other);
  StaticVector(StaticVector&& other);
  template <typename OtherElement, size_t OtherMaxSize>
  StaticVector(StaticVector<OtherElement, OtherMaxSize> const& other);
  template <class Iterator>
  StaticVector(Iterator first, Iterator last);
  StaticVector(size_t size, Element const& value = Element());
  StaticVector(initializer_list<Element> list);
  ~StaticVector();

  StaticVector& operator=(StaticVector const& other);
  StaticVector& operator=(StaticVector&& other);
  StaticVector& operator=(std::initializer_list<Element> list);

  size_t size() const;
  bool empty() const;
  void resize(size_t size, Element const& e = Element());

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

  bool operator==(StaticVector const& other) const;
  bool operator!=(StaticVector const& other) const;
  bool operator<(StaticVector const& other) const;

private:
  size_t m_size;
  typename std::aligned_storage<MaxSize * sizeof(Element), alignof(Element)>::type m_elements;
};

template <typename Element, size_t MaxSize>
StaticVector<Element, MaxSize>::StaticVector()
  : m_size(0) {}

template <typename Element, size_t MaxSize>
StaticVector<Element, MaxSize>::~StaticVector() {
  clear();
}

template <typename Element, size_t MaxSize>
StaticVector<Element, MaxSize>::StaticVector(StaticVector const& other)
  : StaticVector() {
  insert(begin(), other.begin(), other.end());
}

template <typename Element, size_t MaxSize>
StaticVector<Element, MaxSize>::StaticVector(StaticVector&& other)
  : StaticVector() {
  for (auto& e : other)
    emplace_back(std::move(e));
}

template <typename Element, size_t MaxSize>
template <typename OtherElement, size_t OtherMaxSize>
StaticVector<Element, MaxSize>::StaticVector(StaticVector<OtherElement, OtherMaxSize> const& other)
  : StaticVector() {
  for (auto const& e : other)
    emplace_back(e);
}

template <typename Element, size_t MaxSize>
template <class Iterator>
StaticVector<Element, MaxSize>::StaticVector(Iterator first, Iterator last)
  : StaticVector() {
  insert(begin(), first, last);
}

template <typename Element, size_t MaxSize>
StaticVector<Element, MaxSize>::StaticVector(size_t size, Element const& value)
  : StaticVector() {
  resize(size, value);
}

template <typename Element, size_t MaxSize>
StaticVector<Element, MaxSize>::StaticVector(initializer_list<Element> list)
  : StaticVector() {
  for (auto const& e : list)
    emplace_back(e);
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::operator=(StaticVector const& other) -> StaticVector& {
  if (this == &other)
    return *this;

  resize(other.size());
  for (size_t i = 0; i < m_size; ++i)
    operator[](i) = other[i];

  return *this;
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::operator=(StaticVector&& other) -> StaticVector& {
  resize(other.size());
  for (size_t i = 0; i < m_size; ++i)
    operator[](i) = std::move(other[i]);

  return *this;
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::operator=(std::initializer_list<Element> list) -> StaticVector& {
  resize(list.size());
  for (size_t i = 0; i < m_size; ++i)
    operator[](i) = std::move(list[i]);
  return *this;
}

template <typename Element, size_t MaxSize>
size_t StaticVector<Element, MaxSize>::size() const {
  return m_size;
}

template <typename Element, size_t MaxSize>
bool StaticVector<Element, MaxSize>::empty() const {
  return m_size == 0;
}

template <typename Element, size_t MaxSize>
void StaticVector<Element, MaxSize>::resize(size_t size, Element const& e) {
  if (size > MaxSize)
    throw StaticVectorSizeException::format("StaticVector::resize({}) out of range {}", m_size + size, MaxSize);

  for (size_t i = m_size; i > size; --i)
    pop_back();
  for (size_t i = m_size; i < size; ++i)
    emplace_back(e);
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::at(size_t i) -> reference {
  if (i >= m_size)
    throw OutOfRangeException::format("out of range in StaticVector::at({})", i);
  return ptr()[i];
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::at(size_t i) const -> const_reference {
  if (i >= m_size)
    throw OutOfRangeException::format("out of range in StaticVector::at({})", i);
  return ptr()[i];
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::operator[](size_t i) -> reference {
  starAssert(i < m_size);
  return ptr()[i];
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::operator[](size_t i) const -> const_reference {
  starAssert(i < m_size);
  return ptr()[i];
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::begin() const -> const_iterator {
  return ptr();
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::end() const -> const_iterator {
  return ptr() + m_size;
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::begin() -> iterator {
  return ptr();
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::end() -> iterator {
  return ptr() + m_size;
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::rbegin() const -> const_reverse_iterator {
  return const_reverse_iterator(end());
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::rend() const -> const_reverse_iterator {
  return const_reverse_iterator(begin());
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::rbegin() -> reverse_iterator {
  return reverse_iterator(end());
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::rend() -> reverse_iterator {
  return reverse_iterator(begin());
}

template <typename Element, size_t MaxSize>
Element const* StaticVector<Element, MaxSize>::ptr() const {
  return (Element const*)&m_elements;
}

template <typename Element, size_t MaxSize>
Element* StaticVector<Element, MaxSize>::ptr() {
  return (Element*)&m_elements;
}

template <typename Element, size_t MaxSize>
void StaticVector<Element, MaxSize>::push_back(Element e) {
  emplace_back(std::move(e));
}

template <typename Element, size_t MaxSize>
void StaticVector<Element, MaxSize>::pop_back() {
  if (m_size == 0)
    throw OutOfRangeException("StaticVector::pop_back called on empty StaticVector");
  --m_size;
  (ptr() + m_size)->~Element();
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::insert(iterator pos, Element e) -> iterator {
  emplace(pos, std::move(e));
  return pos;
}

template <typename Element, size_t MaxSize>
template <typename Iterator>
auto StaticVector<Element, MaxSize>::insert(iterator pos, Iterator begin, Iterator end) -> iterator {
  size_t toAdd = std::distance(begin, end);
  size_t startIndex = pos - ptr();
  size_t endIndex = startIndex + toAdd;
  size_t toShift = m_size - startIndex;

  resize(m_size + toAdd);

  for (size_t i = toShift; i != 0; --i)
    operator[](endIndex + i - 1) = std::move(operator[](startIndex + i - 1));

  for (size_t i = 0; i != toAdd; ++i)
    operator[](startIndex + i) = *begin++;

  return pos;
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::insert(iterator pos, initializer_list<Element> list) -> iterator {
  return insert(pos, list.begin(), list.end());
}

template <typename Element, size_t MaxSize>
template <typename... Args>
void StaticVector<Element, MaxSize>::emplace(iterator pos, Args&&... args) {
  size_t index = pos - ptr();
  resize(m_size + 1);
  for (size_t i = m_size - 1; i != index; --i)
    operator[](i) = std::move(operator[](i - 1));
  operator[](index) = Element(std::forward<Args>(args)...);
}

template <typename Element, size_t MaxSize>
template <typename... Args>
void StaticVector<Element, MaxSize>::emplace_back(Args&&... args) {
  if (m_size + 1 > MaxSize)
    throw StaticVectorSizeException::format("StaticVector::emplace_back would extend StaticVector beyond size {}", MaxSize);

  m_size += 1;
  new (ptr() + m_size - 1) Element(std::forward<Args>(args)...);
}

template <typename Element, size_t MaxSize>
void StaticVector<Element, MaxSize>::clear() {
  while (m_size != 0)
    pop_back();
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::erase(iterator pos) -> iterator {
  size_t index = pos - ptr();
  for (size_t i = index; i < m_size - 1; ++i)
    operator[](i) = std::move(operator[](i + 1));
  resize(m_size - 1);
  return pos;
}

template <typename Element, size_t MaxSize>
auto StaticVector<Element, MaxSize>::erase(iterator begin, iterator end) -> iterator {
  size_t startIndex = begin - ptr();
  size_t endIndex = end - ptr();
  size_t toRemove = endIndex - startIndex;
  for (size_t i = endIndex; i < m_size; ++i)
    operator[](startIndex + (i - endIndex)) = std::move(operator[](i));
  resize(m_size - toRemove);
  return begin;
}

template <typename Element, size_t MaxSize>
bool StaticVector<Element, MaxSize>::operator==(StaticVector const& other) const {
  if (this == &other)
    return true;

  if (m_size != other.m_size)
    return false;
  for (size_t i = 0; i < m_size; ++i) {
    if (operator[](i) != other[i])
      return false;
  }
  return true;
}

template <typename Element, size_t MaxSize>
bool StaticVector<Element, MaxSize>::operator!=(StaticVector const& other) const {
  return !operator==(other);
}

template <typename Element, size_t MaxSize>
bool StaticVector<Element, MaxSize>::operator<(StaticVector const& other) const {
  for (size_t i = 0; i < m_size; ++i) {
    if (i >= other.size())
      return false;

    Element const& a = operator[](i);
    Element const& b = other[i];

    if (a < b)
      return true;
    else if (b < a)
      return false;
  }

  return m_size < other.size();
}

}
