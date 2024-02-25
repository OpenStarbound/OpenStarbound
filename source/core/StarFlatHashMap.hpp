#pragma once

#include <type_traits>

#include "StarFlatHashTable.hpp"
#include "StarHash.hpp"

namespace Star {

template <typename Key, typename Mapped, typename Hash = hash<Key>, typename Equals = std::equal_to<Key>, typename Allocator = std::allocator<Key>>
class FlatHashMap {
public:
  typedef Key key_type;
  typedef Mapped mapped_type;
  typedef pair<key_type const, mapped_type> value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef Hash hasher;
  typedef Equals key_equal;
  typedef Allocator allocator_type;
  typedef value_type& reference;
  typedef value_type const& const_reference;
  typedef value_type* pointer;
  typedef value_type const* const_pointer;

private:
  typedef pair<key_type, mapped_type> TableValue;

  struct GetKey {
    key_type const& operator()(TableValue const& value) const;
  };

  typedef FlatHashTable<TableValue, key_type, GetKey, Hash, Equals, typename std::allocator_traits<Allocator>::template rebind_alloc<TableValue>> Table;

public:
  struct const_iterator {
    typedef std::forward_iterator_tag iterator_category;
    typedef typename FlatHashMap::value_type const value_type;
    typedef ptrdiff_t difference_type;
    typedef value_type* pointer;
    typedef value_type& reference;

    bool operator==(const_iterator const& rhs) const;
    bool operator!=(const_iterator const& rhs) const;

    const_iterator& operator++();
    const_iterator operator++(int);

    value_type& operator*() const;
    value_type* operator->() const;

    typename Table::const_iterator inner;
  };

  struct iterator {
    typedef std::forward_iterator_tag iterator_category;
    typedef typename FlatHashMap::value_type value_type;
    typedef ptrdiff_t difference_type;
    typedef value_type* pointer;
    typedef value_type& reference;

    bool operator==(iterator const& rhs) const;
    bool operator!=(iterator const& rhs) const;

    iterator& operator++();
    iterator operator++(int);

    value_type& operator*() const;
    value_type* operator->() const;

    operator const_iterator() const;

    typename Table::iterator inner;
  };

  FlatHashMap();
  explicit FlatHashMap(size_t bucketCount, hasher const& hash = hasher(),
      key_equal const& equal = key_equal(), allocator_type const& alloc = allocator_type());
  FlatHashMap(size_t bucketCount, allocator_type const& alloc);
  FlatHashMap(size_t bucketCount, hasher const& hash, allocator_type const& alloc);
  explicit FlatHashMap(allocator_type const& alloc);

  template <typename InputIt>
  FlatHashMap(InputIt first, InputIt last, size_t bucketCount = 0,
      hasher const& hash = hasher(), key_equal const& equal = key_equal(),
      allocator_type const& alloc = allocator_type());
  template <typename InputIt>
  FlatHashMap(InputIt first, InputIt last, size_t bucketCount, allocator_type const& alloc);
  template <typename InputIt>
  FlatHashMap(InputIt first, InputIt last, size_t bucketCount,
      hasher const& hash, allocator_type const& alloc);

  FlatHashMap(FlatHashMap const& other);
  FlatHashMap(FlatHashMap const& other, allocator_type const& alloc);
  FlatHashMap(FlatHashMap&& other);
  FlatHashMap(FlatHashMap&& other, allocator_type const& alloc);

  FlatHashMap(initializer_list<value_type> init, size_t bucketCount = 0,
      hasher const& hash = hasher(), key_equal const& equal = key_equal(),
      allocator_type const& alloc = allocator_type());
  FlatHashMap(initializer_list<value_type> init, size_t bucketCount, allocator_type const& alloc);
  FlatHashMap(initializer_list<value_type> init, size_t bucketCount, hasher const& hash,
      allocator_type const& alloc);

  FlatHashMap& operator=(FlatHashMap const& other);
  FlatHashMap& operator=(FlatHashMap&& other);
  FlatHashMap& operator=(initializer_list<value_type> init);

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  const_iterator cbegin() const;
  const_iterator cend() const;

  size_t empty() const;
  size_t size() const;
  void clear();

  pair<iterator, bool> insert(value_type const& value);
  template <typename T, typename = typename std::enable_if<std::is_constructible<TableValue, T&&>::value>::type>
  pair<iterator, bool> insert(T&& value);
  iterator insert(const_iterator hint, value_type const& value);
  template <typename T, typename = typename std::enable_if<std::is_constructible<TableValue, T&&>::value>::type>
  iterator insert(const_iterator hint, T&& value);
  template <typename InputIt>
  void insert(InputIt first, InputIt last);
  void insert(initializer_list<value_type> init);

  template <typename... Args>
  pair<iterator, bool> emplace(Args&&... args);
  template <typename... Args>
  iterator emplace_hint(const_iterator hint, Args&&... args);

  iterator erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);
  size_t erase(key_type const& key);

  mapped_type& at(key_type const& key);
  mapped_type const& at(key_type const& key) const;

  mapped_type& operator[](key_type const& key);
  mapped_type& operator[](key_type&& key);

  size_t count(key_type const& key) const;
  const_iterator find(key_type const& key) const;
  iterator find(key_type const& key);
  pair<iterator, iterator> equal_range(key_type const& key);
  pair<const_iterator, const_iterator> equal_range(key_type const& key) const;

  void reserve(size_t capacity);

  bool operator==(FlatHashMap const& rhs) const;
  bool operator!=(FlatHashMap const& rhs) const;

private:
  Table m_table;
};

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::GetKey::operator()(TableValue const& value) const -> key_type const& {
  return value.first;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
bool FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::const_iterator::operator==(const_iterator const& rhs) const {
  return inner == rhs.inner;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
bool FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::const_iterator::operator!=(const_iterator const& rhs) const {
  return inner != rhs.inner;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::const_iterator::operator++() -> const_iterator& {
  ++inner;
  return *this;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::const_iterator::operator++(int) -> const_iterator {
  const_iterator copy(*this);
  ++*this;
  return copy;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::const_iterator::operator*() const -> value_type& {
  return *operator->();
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::const_iterator::operator->() const -> value_type* {
  return (value_type*)(&*inner);
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
bool FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::iterator::operator==(iterator const& rhs) const {
  return inner == rhs.inner;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
bool FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::iterator::operator!=(iterator const& rhs) const {
  return inner != rhs.inner;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::iterator::operator++() -> iterator& {
  ++inner;
  return *this;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::iterator::operator++(int) -> iterator {
  iterator copy(*this);
  operator++();
  return copy;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::iterator::operator*() const -> value_type& {
  return *operator->();
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::iterator::operator->() const -> value_type* {
  return (value_type*)(&*inner);
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::iterator::operator typename FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::const_iterator() const {
  return const_iterator{inner};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap()
  : FlatHashMap(0) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(size_t bucketCount, hasher const& hash,
    key_equal const& equal, allocator_type const& alloc)
  : m_table(bucketCount, GetKey(), hash, equal, alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(size_t bucketCount, allocator_type const& alloc)
  : FlatHashMap(bucketCount, hasher(), key_equal(), alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(size_t bucketCount, hasher const& hash,
    allocator_type const& alloc)
  : FlatHashMap(bucketCount, hash, key_equal(), alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(allocator_type const& alloc)
  : FlatHashMap(0, hasher(), key_equal(), alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename InputIt>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(InputIt first, InputIt last, size_t bucketCount,
    hasher const& hash, key_equal const& equal, allocator_type const& alloc)
  : FlatHashMap(bucketCount, hash, equal, alloc) {
  insert(first, last);
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename InputIt>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(InputIt first, InputIt last, size_t bucketCount,
    allocator_type const& alloc)
  : FlatHashMap(first, last, bucketCount, hasher(), key_equal(), alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename InputIt>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(InputIt first, InputIt last, size_t bucketCount,
    hasher const& hash, allocator_type const& alloc)
  : FlatHashMap(first, last, bucketCount, hash, key_equal(), alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(FlatHashMap const& other)
  : FlatHashMap(other, other.m_table.getAllocator()) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(FlatHashMap const& other, allocator_type const& alloc)
  : FlatHashMap(alloc) {
  operator=(other);
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(FlatHashMap&& other)
  : FlatHashMap(std::move(other), other.m_table.getAllocator()) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(FlatHashMap&& other, allocator_type const& alloc)
  : FlatHashMap(alloc) {
  operator=(std::move(other));
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(initializer_list<value_type> init, size_t bucketCount, hasher const& hash,
    key_equal const& equal, allocator_type const& alloc)
  : FlatHashMap(bucketCount, hash, equal, alloc) {
  operator=(init);
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(initializer_list<value_type> init, size_t bucketCount,
    allocator_type const& alloc)
  : FlatHashMap(init, bucketCount, hasher(), key_equal(), alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::FlatHashMap(initializer_list<value_type> init, size_t bucketCount, hasher const& hash,
    allocator_type const& alloc)
  : FlatHashMap(init, bucketCount, hash, key_equal(), alloc) {}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::operator=(FlatHashMap const& other) -> FlatHashMap& {
  m_table.clear();
  m_table.reserve(other.size());
  for (auto const& p : other)
    m_table.insert(p);
  return *this;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::operator=(FlatHashMap&& other) -> FlatHashMap& {
  m_table = std::move(other.m_table);
  return *this;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::operator=(initializer_list<value_type> init) -> FlatHashMap& {
  clear();
  insert(init.begin(), init.end());
  return *this;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::begin() -> iterator {
  return iterator{m_table.begin()};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::end() -> iterator {
  return iterator{m_table.end()};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::begin() const -> const_iterator {
  return const_iterator{m_table.begin()};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::end() const -> const_iterator {
  return const_iterator{m_table.end()};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::cbegin() const -> const_iterator {
  return begin();
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::cend() const -> const_iterator {
  return end();
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
size_t FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::empty() const {
  return m_table.empty();
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
size_t FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::size() const {
  return m_table.size();
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
void FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::clear() {
  m_table.clear();
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::insert(value_type const& value) -> pair<iterator, bool> {
  auto res = m_table.insert(TableValue(value));
  return {iterator{res.first}, res.second};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename T, typename>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::insert(T&& value) -> pair<iterator, bool> {
  auto res = m_table.insert(TableValue(std::forward<T&&>(value)));
  return {iterator{res.first}, res.second};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::insert(const_iterator hint, value_type const& value) -> iterator {
  return insert(hint, TableValue(value));
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename T, typename>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::insert(const_iterator, T&& value) -> iterator {
  return insert(std::forward<T&&>(value)).first;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename InputIt>
void FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::insert(InputIt first, InputIt last) {
  m_table.reserve(m_table.size() + std::distance(first, last));
  for (auto i = first; i != last; ++i)
    m_table.insert(*i);
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
void FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::insert(initializer_list<value_type> init) {
  insert(init.begin(), init.end());
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename... Args>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::emplace(Args&&... args) -> pair<iterator, bool> {
  return insert(TableValue(std::forward<Args>(args)...));
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
template <typename... Args>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::emplace_hint(const_iterator hint, Args&&... args) -> iterator {
  return insert(hint, TableValue(std::forward<Args>(args)...));
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::erase(const_iterator pos) -> iterator {
  return iterator{m_table.erase(pos.inner)};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::erase(const_iterator first, const_iterator last) -> iterator {
  return iterator{m_table.erase(first.inner, last.inner)};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
size_t FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::erase(key_type const& key) {
  auto i = m_table.find(key);
  if (i != m_table.end()) {
    m_table.erase(i);
    return 1;
  }
  return 0;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::at(key_type const& key) -> mapped_type& {
  auto i = m_table.find(key);
  if (i == m_table.end())
    throw std::out_of_range("no such key in FlatHashMap");
  return i->second;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::at(key_type const& key) const -> mapped_type const& {
  auto i = m_table.find(key);
  if (i == m_table.end())
    throw std::out_of_range("no such key in FlatHashMap");
  return i->second;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::operator[](key_type const& key) -> mapped_type& {
  auto i = m_table.find(key);
  if (i != m_table.end())
    return i->second;
  return m_table.insert({key, mapped_type()}).first->second;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::operator[](key_type&& key) -> mapped_type& {
  auto i = m_table.find(key);
  if (i != m_table.end())
    return i->second;
  return m_table.insert({std::move(key), mapped_type()}).first->second;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
size_t FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::count(key_type const& key) const {
  if (m_table.find(key) != m_table.end())
    return 1;
  else
    return 0;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::find(key_type const& key) const -> const_iterator {
  return const_iterator{m_table.find(key)};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::find(key_type const& key) -> iterator {
  return iterator{m_table.find(key)};
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::equal_range(key_type const& key) -> pair<iterator, iterator> {
  auto i = find(key);
  if (i != end()) {
    auto j = i;
    ++j;
    return {i, j};
  } else {
    return {i, i};
  }
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
auto FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::equal_range(key_type const& key) const -> pair<const_iterator, const_iterator> {
  auto i = find(key);
  if (i != end()) {
    auto j = i;
    ++j;
    return {i, j};
  } else {
    return {i, i};
  }
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
void FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::reserve(size_t capacity) {
  m_table.reserve(capacity);
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
bool FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::operator==(FlatHashMap const& rhs) const {
  return m_table == rhs.m_table;
}

template <typename Key, typename Mapped, typename Hash, typename Equals, typename Allocator>
bool FlatHashMap<Key, Mapped, Hash, Equals, Allocator>::operator!=(FlatHashMap const& rhs) const {
  return m_table != rhs.m_table;
}

}
