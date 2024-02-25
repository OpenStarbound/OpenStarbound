#pragma once

#include "StarString.hpp"

namespace Star {

// Bi-directional map of unique sets of elements with quick map access from
// either the left or right element to the other side.  Every left side value
// must be unique from every other left side value and the same for the right
// side.
template <typename LeftT,
    typename RightT,
    typename LeftMapT = Map<LeftT, RightT const*>,
    typename RightMapT = Map<RightT, LeftT const*>>
class BiMap {
public:
  typedef LeftT Left;
  typedef RightT Right;
  typedef LeftMapT LeftMap;
  typedef RightMapT RightMap;

  typedef pair<Left, Right> value_type;

  struct BiMapIterator {
    BiMapIterator& operator++();
    BiMapIterator operator++(int);

    bool operator==(BiMapIterator const& rhs) const;
    bool operator!=(BiMapIterator const& rhs) const;

    pair<Left const&, Right const&> operator*() const;

    typename LeftMap::const_iterator iterator;
  };

  typedef BiMapIterator iterator;
  typedef iterator const_iterator;

  template <typename Collection>
  static BiMap from(Collection const& c);

  BiMap();
  BiMap(BiMap const& map);

  template <typename InputIterator>
  BiMap(InputIterator beg, InputIterator end);

  BiMap(std::initializer_list<value_type> list);

  List<Left> leftValues() const;
  List<Right> rightValues() const;
  List<value_type> pairs() const;

  bool hasLeftValue(Left const& left) const;
  bool hasRightValue(Right const& right) const;

  Right const& getRight(Left const& left) const;
  Left const& getLeft(Right const& right) const;

  Right valueRight(Left const& left, Right const& def = Right()) const;
  Left valueLeft(Right const& right, Left const& def = Left()) const;

  Maybe<Right> maybeRight(Left const& left) const;

  Maybe<Left> maybeLeft(Right const& right) const;

  Right takeRight(Left const& left);
  Left takeLeft(Right const& right);

  Maybe<Right> maybeTakeRight(Left const& left);
  Maybe<Left> maybeTakeLeft(Right const& right);

  Right const* rightPtr(Left const& left) const;
  Left const* leftPtr(Right const& right) const;

  BiMap& operator=(BiMap const& map);

  pair<iterator, bool> insert(value_type const& val);

  // Returns true if value was inserted, false if either the left or right side
  // already existed.
  bool insert(Left const& left, Right const& right);

  // Throws an exception if the pair cannot be inserted
  void add(Left const& left, Right const& right);
  void add(value_type const& value);

  // Overwrites the left / right mapping regardless of whether each side
  // already exists.
  void overwrite(Left const& left, Right const& right);
  void overwrite(value_type const& value);

  // Removes the pair with the given left side, returns true if this pair was
  // found, false otherwise.
  bool removeLeft(Left const& left);

  // Removes the pair with the given right side, returns true if this pair was
  // found, false otherwise.
  bool removeRight(Right const& right);

  const_iterator begin() const;
  const_iterator end() const;

  size_t size() const;

  void clear();

  bool empty() const;

  bool operator==(BiMap const& m) const;

private:
  LeftMap m_leftMap;
  RightMap m_rightMap;
};

template <typename Left, typename Right, typename LeftHash = Star::hash<Left>, typename RightHash = Star::hash<Right>>
using BiHashMap = BiMap<Left, Right, StableHashMap<Left, Right const*, LeftHash>, StableHashMap<Right, Left const*, RightHash>>;

// Case insensitive Enum <-> String map
template <typename EnumType>
using EnumMap = BiMap<EnumType,
    String,
    Map<EnumType, String const*>,
    StableHashMap<String, EnumType const*, CaseInsensitiveStringHash, CaseInsensitiveStringCompare>>;

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
auto BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMapIterator::operator++() -> BiMapIterator & {
  ++iterator;
  return *this;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
auto BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMapIterator::operator++(int) -> BiMapIterator {
  BiMapIterator last{iterator};
  ++iterator;
  return last;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMapIterator::operator==(BiMapIterator const& rhs) const {
  return iterator == rhs.iterator;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMapIterator::operator!=(BiMapIterator const& rhs) const {
  return iterator != rhs.iterator;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
pair<LeftT const&, RightT const&> BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMapIterator::operator*() const {
  return {iterator->first, *iterator->second};
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
template <typename Collection>
BiMap<LeftT, RightT, LeftMapT, RightMapT> BiMap<LeftT, RightT, LeftMapT, RightMapT>::from(Collection const& c) {
  return BiMap(c.begin(), c.end());
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMap() {}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMap(BiMap const& map)
  : BiMap(map.begin(), map.end()) {}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
template <typename InputIterator>
BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMap(InputIterator beg, InputIterator end) {
  while (beg != end) {
    insert(*beg);
    ++beg;
  }
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
BiMap<LeftT, RightT, LeftMapT, RightMapT>::BiMap(std::initializer_list<value_type> list) {
  for (value_type const& v : list) {
    if (!insert(v.first, v.second))
      throw MapException::format("Repeat pair in BiMap initializer_list construction: ({}, {})", outputAny(v.first), outputAny(v.second));
  }
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
List<LeftT> BiMap<LeftT, RightT, LeftMapT, RightMapT>::leftValues() const {
  return m_leftMap.keys();
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
List<RightT> BiMap<LeftT, RightT, LeftMapT, RightMapT>::rightValues() const {
  return m_rightMap.keys();
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
auto BiMap<LeftT, RightT, LeftMapT, RightMapT>::pairs() const -> List<value_type> {
  List<value_type> values;
  for (auto const& p : *this)
    values.append(p);
  return values;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::hasLeftValue(Left const& left) const {
  return m_leftMap.contains(left);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::hasRightValue(Right const& right) const {
  return m_rightMap.contains(right);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
RightT const& BiMap<LeftT, RightT, LeftMapT, RightMapT>::getRight(Left const& left) const {
  return *m_leftMap.get(left);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
LeftT const& BiMap<LeftT, RightT, LeftMapT, RightMapT>::getLeft(Right const& right) const {
  return *m_rightMap.get(right);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
RightT BiMap<LeftT, RightT, LeftMapT, RightMapT>::valueRight(Left const& left, Right const& def) const {
  return maybeRight(left).value(def);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
LeftT BiMap<LeftT, RightT, LeftMapT, RightMapT>::valueLeft(Right const& right, Left const& def) const {
  return maybeLeft(right).value(def);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
Maybe<RightT> BiMap<LeftT, RightT, LeftMapT, RightMapT>::maybeRight(Left const& left) const {
  auto i = m_leftMap.find(left);
  if (i != m_leftMap.end())
    return *i->second;
  return {};
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
Maybe<LeftT> BiMap<LeftT, RightT, LeftMapT, RightMapT>::maybeLeft(Right const& right) const {
  auto i = m_rightMap.find(right);
  if (i != m_rightMap.end())
    return *i->second;
  return {};
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
RightT BiMap<LeftT, RightT, LeftMapT, RightMapT>::takeRight(Left const& left) {
  if (auto right = maybeTakeRight(left))
    return right.take();
  throw MapException::format("No such key in BiMap::takeRight", outputAny(left));
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
LeftT BiMap<LeftT, RightT, LeftMapT, RightMapT>::takeLeft(Right const& right) {
  if (auto left = maybeTakeLeft(right))
    return left.take();
  throw MapException::format("No such key in BiMap::takeLeft", outputAny(right));
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
Maybe<RightT> BiMap<LeftT, RightT, LeftMapT, RightMapT>::maybeTakeRight(Left const& left) {
  if (auto rightPtr = m_leftMap.maybeTake(left).value()) {
    Right right = *rightPtr;
    m_rightMap.remove(*rightPtr);
    return right;
  } else {
    return {};
  }
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
Maybe<LeftT> BiMap<LeftT, RightT, LeftMapT, RightMapT>::maybeTakeLeft(Right const& right) {
  if (auto leftPtr = m_rightMap.maybeTake(right).value()) {
    Left left = *leftPtr;
    m_leftMap.remove(*leftPtr);
    return left;
  } else {
    return {};
  }
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
RightT const* BiMap<LeftT, RightT, LeftMapT, RightMapT>::rightPtr(Left const& left) const {
  return m_leftMap.value(left);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
LeftT const* BiMap<LeftT, RightT, LeftMapT, RightMapT>::leftPtr(Right const& right) const {
  return m_rightMap.value(right);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
BiMap<LeftT, RightT, LeftMapT, RightMapT>& BiMap<LeftT, RightT, LeftMapT, RightMapT>::operator=(BiMap const& map) {
  if (this != &map) {
    clear();
    for (auto const& p : map)
      insert(p);
  }
  return *this;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
auto BiMap<LeftT, RightT, LeftMapT, RightMapT>::insert(value_type const& val) -> pair<iterator, bool> {
  auto leftRes = m_leftMap.insert(make_pair(val.first, nullptr));
  if (!leftRes.second)
    return {BiMapIterator{leftRes.first}, false};

  auto rightRes = m_rightMap.insert(make_pair(val.second, nullptr));
  starAssert(rightRes.second == true);
  leftRes.first->second = &rightRes.first->first;
  rightRes.first->second = &leftRes.first->first;
  return {BiMapIterator{leftRes.first}, true};
};

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::insert(Left const& left, Right const& right) {
  return insert(make_pair(left, right)).second;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
void BiMap<LeftT, RightT, LeftMapT, RightMapT>::add(Left const& left, Right const& right) {
  if (m_leftMap.contains(left))
    throw MapException(strf("BiMap already contains left side value '{}'", outputAny(left)));

  if (m_rightMap.contains(right))
    throw MapException(strf("BiMap already contains right side value '{}'", outputAny(right)));

  insert(left, right);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
void BiMap<LeftT, RightT, LeftMapT, RightMapT>::add(value_type const& value) {
  add(value.first, value.second);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
void BiMap<LeftT, RightT, LeftMapT, RightMapT>::overwrite(Left const& left, Right const& right) {
  removeLeft(left);
  removeRight(right);
  insert(left, right);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
void BiMap<LeftT, RightT, LeftMapT, RightMapT>::overwrite(value_type const& value) {
  return overwrite(value.first, value.second);
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::removeLeft(Left const& left) {
  if (auto right = m_leftMap.value(left)) {
    m_rightMap.remove(*right);
    m_leftMap.remove(left);
    return true;
  }

  return false;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::removeRight(Right const& right) {
  if (auto left = m_rightMap.value(right)) {
    m_leftMap.remove(*left);
    m_rightMap.remove(right);
    return true;
  }

  return false;
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
auto BiMap<LeftT, RightT, LeftMapT, RightMapT>::begin() const -> const_iterator {
  return BiMapIterator{m_leftMap.begin()};
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
auto BiMap<LeftT, RightT, LeftMapT, RightMapT>::end() const -> const_iterator {
  return BiMapIterator{m_leftMap.end()};
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
size_t BiMap<LeftT, RightT, LeftMapT, RightMapT>::size() const {
  return m_leftMap.size();
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
void BiMap<LeftT, RightT, LeftMapT, RightMapT>::clear() {
  m_leftMap.clear();
  m_rightMap.clear();
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::empty() const {
  return m_leftMap.empty();
}

template <typename LeftT, typename RightT, typename LeftMapT, typename RightMapT>
bool BiMap<LeftT, RightT, LeftMapT, RightMapT>::operator==(BiMap const& m) const {
  if (&m == this)
    return true;

  if (size() != m.size())
    return false;

  for (auto const& pair : *this) {
    if (auto p = m.rightPtr(pair.first))
      if (!p || *p != pair.second)
        return false;
  }

  return true;
}

}
