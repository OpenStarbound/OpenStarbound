#pragma once

#include <set>
#include <unordered_set>

#include "StarFlatHashSet.hpp"
#include "StarList.hpp"

namespace Star {

STAR_EXCEPTION(SetException, StarException);

template <typename BaseSet>
class SetMixin : public BaseSet {
public:
  typedef BaseSet Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;

  typedef typename Base::value_type value_type;

  using Base::Base;

  List<value_type> values() const;

  bool contains(value_type const& v) const;

  bool add(value_type const& v);

  // Like add, but always adds new value, potentially replacing another equal
  // (comparing equal, may not be actually equal) value.  Returns whether an
  // existing value was replaced.
  bool replace(value_type v);

  template <typename Container>
  void addAll(Container const& s);

  bool remove(value_type const& v);

  template <typename Container>
  void removeAll(Container const& s);

  value_type first();
  Maybe<value_type> maybeFirst();
  value_type takeFirst();
  Maybe<value_type> maybeTakeFirst();

  value_type last();
  Maybe<value_type> maybeLast();
  value_type takeLast();
  Maybe<value_type> maybeTakeLast();

  bool hasIntersection(SetMixin const& s) const;
};

template <typename BaseSet>
std::ostream& operator<<(std::ostream& os, SetMixin<BaseSet> const& set);

template <typename Value, typename Compare = std::less<Value>, typename Allocator = std::allocator<Value>>
class Set : public SetMixin<std::set<Value, Compare, Allocator>> {
public:
  typedef SetMixin<std::set<Value, Compare, Allocator>> Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;

  typedef typename Base::value_type value_type;

  template <typename Container>
  static Set from(Container const& c);

  using Base::Base;

  // Returns set of elements that are in this set and the given set.
  Set intersection(Set const& s) const;
  Set intersection(Set const& s, std::function<bool(Value const&, Value const&)> compare) const;

  // Returns elements in this set that are not in the given set
  Set difference(Set const& s) const;
  Set difference(Set const& s, std::function<bool(Value const&, Value const&)> compare) const;

  // Returns elements in either this set or the given set
  Set combination(Set const& s) const;
};

template <typename BaseSet>
class HashSetMixin : public SetMixin<BaseSet> {
public:
  typedef SetMixin<BaseSet> Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;

  typedef typename Base::value_type value_type;

  template <typename Container>
  static HashSetMixin from(Container const& c);

  using Base::Base;

  HashSetMixin intersection(HashSetMixin const& s) const;
  HashSetMixin difference(HashSetMixin const& s) const;
  HashSetMixin combination(HashSetMixin const& s) const;
};

template <typename Value, typename Hash = hash<Value>, typename Equals = std::equal_to<Value>, typename Allocator = std::allocator<Value>>
using HashSet = HashSetMixin<FlatHashSet<Value, Hash, Equals, Allocator>>;

template <typename Value, typename Hash = hash<Value>, typename Equals = std::equal_to<Value>, typename Allocator = std::allocator<Value>>
using StableHashSet = HashSetMixin<std::unordered_set<Value, Hash, Equals, Allocator>>;

template <typename BaseSet>
auto SetMixin<BaseSet>::values() const -> List<value_type> {
  return List<value_type>(Base::begin(), Base::end());
}

template <typename BaseSet>
bool SetMixin<BaseSet>::contains(value_type const& v) const {
  return Base::find(v) != Base::end();
}

template <typename BaseSet>
bool SetMixin<BaseSet>::add(value_type const& v) {
  return Base::insert(v).second;
}

template <typename BaseSet>
bool SetMixin<BaseSet>::replace(value_type v) {
  bool replaced = remove(v);
  Base::insert(std::move(v));
  return replaced;
}

template <typename BaseSet>
template <typename Container>
void SetMixin<BaseSet>::addAll(Container const& s) {
  return Base::insert(s.begin(), s.end());
}

template <typename BaseSet>
bool SetMixin<BaseSet>::remove(value_type const& v) {
  return Base::erase(v) != 0;
}

template <typename BaseSet>
template <typename Container>
void SetMixin<BaseSet>::removeAll(Container const& s) {
  for (auto const& v : s)
    remove(v);
}

template <typename BaseSet>
auto SetMixin<BaseSet>::first() -> value_type {
  if (Base::empty())
    throw SetException("first called on empty set");
  return *Base::begin();
}

template <typename BaseSet>
auto SetMixin<BaseSet>::maybeFirst() -> Maybe<value_type> {
  if (Base::empty())
    return {};
  return *Base::begin();
}

template <typename BaseSet>
auto SetMixin<BaseSet>::takeFirst() -> value_type {
  if (Base::empty())
    throw SetException("takeFirst called on empty set");
  auto i = Base::begin();
  value_type v = std::move(*i);
  Base::erase(i);
  return v;
}

template <typename BaseSet>
auto SetMixin<BaseSet>::maybeTakeFirst() -> Maybe<value_type> {
  if (Base::empty())
    return {};
  auto i = Base::begin();
  value_type v = std::move(*i);
  Base::erase(i);
  return std::move(v);
}

template <typename BaseSet>
auto SetMixin<BaseSet>::last() -> value_type {
  if (Base::empty())
    throw SetException("last called on empty set");
  return *prev(Base::end());
}

template <typename BaseSet>
auto SetMixin<BaseSet>::maybeLast() -> Maybe<value_type> {
  if (Base::empty())
    return {};
  return *prev(Base::end());
}

template <typename BaseSet>
auto SetMixin<BaseSet>::takeLast() -> value_type {
  if (Base::empty())
    throw SetException("takeLast called on empty set");
  auto i = prev(Base::end());
  value_type v = std::move(*i);
  Base::erase(i);
  return v;
}

template <typename BaseSet>
auto SetMixin<BaseSet>::maybeTakeLast() -> Maybe<value_type> {
  if (Base::empty())
    return {};
  auto i = prev(Base::end());
  value_type v = std::move(*i);
  Base::erase(i);
  return std::move(v);
}

template <typename BaseSet>
bool SetMixin<BaseSet>::hasIntersection(SetMixin const& s) const {
  for (auto const& v : s) {
    if (contains(v)) {
      return true;
    }
  }
  return false;
}

template <typename BaseSet>
std::ostream& operator<<(std::ostream& os, SetMixin<BaseSet> const& set) {
  os << "(";
  for (auto i = set.begin(); i != set.end(); ++i) {
    if (i != set.begin())
      os << ", ";
    os << *i;
  }
  os << ")";
  return os;
}

template <typename Value, typename Compare, typename Allocator>
template <typename Container>
Set<Value, Compare, Allocator> Set<Value, Compare, Allocator>::from(Container const& c) {
  return Set(c.begin(), c.end());
}

template <typename Value, typename Compare, typename Allocator>
Set<Value, Compare, Allocator> Set<Value, Compare, Allocator>::intersection(Set const& s) const {
  Set res;
  std::set_intersection(Base::begin(), Base::end(), s.begin(), s.end(), std::inserter(res, res.end()));
  return res;
}

template <typename Value, typename Compare, typename Allocator>
Set<Value, Compare, Allocator> Set<Value, Compare, Allocator>::intersection(Set const& s, std::function<bool(Value const&, Value const&)> compare) const {
  Set res;
  std::set_intersection(Base::begin(), Base::end(), s.begin(), s.end(), std::inserter(res, res.end()), compare);
  return res;
}

template <typename Value, typename Compare, typename Allocator>
Set<Value, Compare, Allocator> Set<Value, Compare, Allocator>::difference(Set const& s) const {
  Set res;
  std::set_difference(Base::begin(), Base::end(), s.begin(), s.end(), std::inserter(res, res.end()));
  return res;
}

template <typename Value, typename Compare, typename Allocator>
Set<Value, Compare, Allocator> Set<Value, Compare, Allocator>::difference(Set const& s, std::function<bool(Value const&, Value const&)> compare) const {
  Set res;
  std::set_difference(Base::begin(), Base::end(), s.begin(), s.end(), std::inserter(res, res.end()), compare);
  return res;
}

template <typename Value, typename Compare, typename Allocator>
Set<Value, Compare, Allocator> Set<Value, Compare, Allocator>::combination(Set const& s) const {
  Set ret(*this);
  ret.addAll(s);
  return ret;
}

template <typename BaseMap>
template <typename Container>
HashSetMixin<BaseMap> HashSetMixin<BaseMap>::from(Container const& c) {
  return HashSetMixin(c.begin(), c.end());
}

template <typename BaseMap>
HashSetMixin<BaseMap> HashSetMixin<BaseMap>::intersection(HashSetMixin const& s) const {
  // Can't use std::set_intersection, since not sorted, naive version is fine.
  HashSetMixin ret;
  for (auto const& e : s) {
    if (contains(e))
      ret.add(e);
  }
  return ret;
}

template <typename BaseMap>
HashSetMixin<BaseMap> HashSetMixin<BaseMap>::difference(HashSetMixin const& s) const {
  // Can't use std::set_difference, since not sorted, naive version is fine.
  HashSetMixin ret;
  for (auto const& e : *this) {
    if (!s.contains(e))
      ret.add(e);
  }
  return ret;
}

template <typename BaseMap>
HashSetMixin<BaseMap> HashSetMixin<BaseMap>::combination(HashSetMixin const& s) const {
  HashSetMixin ret(*this);
  ret.addAll(s);
  return ret;
}

}
