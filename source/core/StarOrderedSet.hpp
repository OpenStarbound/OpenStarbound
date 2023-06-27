#ifndef STAR_ORDERED_SET_HPP
#define STAR_ORDERED_SET_HPP

#include <map>

#include "StarFlatHashMap.hpp"
#include "StarSet.hpp"
#include "StarList.hpp"

namespace Star {

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
class OrderedSetWrapper {
public:
  typedef Value value_type;

  typedef LinkedList<value_type, typename Allocator::template rebind<value_type>::other> OrderType;
  typedef Map<
      std::reference_wrapper<value_type const>, typename OrderType::const_iterator, Args...,
      typename Allocator::template rebind<pair<std::reference_wrapper<value_type const> const, typename OrderType::const_iterator>>::other
    > MapType;

  typedef typename OrderType::const_iterator const_iterator;
  typedef const_iterator iterator;

  typedef typename OrderType::const_reverse_iterator const_reverse_iterator;
  typedef const_reverse_iterator reverse_iterator;

  template <typename Collection>
  static OrderedSetWrapper from(Collection const& c);

  OrderedSetWrapper();
  OrderedSetWrapper(OrderedSetWrapper const& set);

  template <typename InputIterator>
  OrderedSetWrapper(InputIterator beg, InputIterator end);

  OrderedSetWrapper(initializer_list<value_type> list);

  OrderedSetWrapper& operator=(OrderedSetWrapper const& set);

  // Guaranteed to be in order.
  List<value_type> values() const;

  bool contains(value_type const& v) const;

  // add either adds the value to the back, or does not move it from its
  // current order.
  pair<iterator, bool> insert(value_type const& v);

  // like insert, but only returns whether the value was added or not.
  bool add(Value const& v);

  // Always replaces an existing value with a new value if it exists, and
  // always moves to the back.
  bool replace(Value const& v);

  // Either adds a value to the end of the order, or moves an existing value to
  // the back.
  bool addBack(Value const& v);

  // Either adds a value to the beginning of the order, or moves an existing
  // value to the beginning.
  bool addFront(Value const& v);

  template <typename Container>
  void addAll(Container const& c);

  iterator toFront(iterator i);

  iterator toBack(iterator i);

  bool remove(value_type const& v);

  template <typename Container>
  void removeAll(Container const& c);

  void clear();

  value_type const& first() const;
  value_type const& last() const;

  void removeFirst();
  void removeLast();

  value_type takeFirst();
  value_type takeLast();

  template <typename Compare>
  void sort(Compare comp);

  void sort();

  size_t empty() const;
  size_t size() const;

  const_iterator begin() const;
  const_iterator end() const;

  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;

  Maybe<size_t> indexOf(value_type const& v) const;

  value_type const& at(size_t i) const;
  value_type& at(size_t i);

  OrderedSetWrapper intersection(OrderedSetWrapper const& s) const;
  OrderedSetWrapper difference(OrderedSetWrapper const& s) const;

private:
  MapType m_map;
  OrderType m_order;
};

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
std::ostream& operator<<(std::ostream& os, OrderedSetWrapper<Map, Value, Allocator, Args...> const& set);

template <typename Value, typename Compare = std::less<Value>, typename Allocator = std::allocator<Value>>
using OrderedSet = OrderedSetWrapper<std::map, Value, Allocator, Compare>;

template <typename Value, typename Hash = Star::hash<Value>, typename Equals = std::equal_to<Value>, typename Allocator = std::allocator<Value>>
using OrderedHashSet = OrderedSetWrapper<FlatHashMap, Value, Allocator, Hash, Equals>;

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
template <typename Collection>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::from(Collection const& c) -> OrderedSetWrapper {
  return OrderedSetWrapper(c.begin(), c.end());
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
OrderedSetWrapper<Map, Value, Allocator, Args...>::OrderedSetWrapper() {}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
OrderedSetWrapper<Map, Value, Allocator, Args...>::OrderedSetWrapper(OrderedSetWrapper const& set) {
  for (auto const& p : set)
    add(p);
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
template <typename InputIterator>
OrderedSetWrapper<Map, Value, Allocator, Args...>::OrderedSetWrapper(InputIterator beg, InputIterator end) {
  while (beg != end) {
    add(*beg);
    ++beg;
  }
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
OrderedSetWrapper<Map, Value, Allocator, Args...>::OrderedSetWrapper(initializer_list<value_type> list) {
  for (value_type const& v : list)
    add(v);
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::operator=(OrderedSetWrapper const& set) -> OrderedSetWrapper& {
  if (this != &set) {
    clear();
    for (auto const& p : set)
      add(p);
  }

  return *this;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::values() const -> List<value_type> {
  List<value_type> values;
  for (auto p : *this)
    values.append(move(p));
  return values;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
bool OrderedSetWrapper<Map, Value, Allocator, Args...>::contains(value_type const& v) const {
  return m_map.find(v) != m_map.end();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::insert(value_type const& v) -> pair<iterator, bool> {
  auto i = m_map.find(v);
  if (i == m_map.end()) {
    auto orderIt = m_order.insert(m_order.end(), v);
    m_map.insert(typename MapType::value_type(std::cref(*orderIt), orderIt));
    return {orderIt, true};
  }
  return {i->second, false};
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
bool OrderedSetWrapper<Map, Value, Allocator, Args...>::add(Value const& v) {
  return insert(v).second;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
bool OrderedSetWrapper<Map, Value, Allocator, Args...>::replace(Value const& v) {
  bool replaced = remove(v);
  add(v);
  return replaced;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
bool OrderedSetWrapper<Map, Value, Allocator, Args...>::addBack(Value const& v) {
  auto i = m_map.find(v);
  if (i != m_map.end()) {
    m_order.splice(m_order.end(), m_order, i->second);
    return false;
  } else {
    iterator orderIt = m_order.insert(m_order.end(), v);
    m_map.insert(typename MapType::value_type(std::cref(*orderIt), orderIt));
    return true;
  }
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
bool OrderedSetWrapper<Map, Value, Allocator, Args...>::addFront(Value const& v) {
  auto i = m_map.find(v);
  if (i != m_map.end()) {
    m_order.splice(m_order.begin(), m_order, i->second);
    return false;
  } else {
    iterator orderIt = m_order.insert(m_order.end(), v);
    m_map.insert(typename MapType::value_type(std::cref(*orderIt), orderIt));
    return true;
  }
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
template <typename Container>
void OrderedSetWrapper<Map, Value, Allocator, Args...>::addAll(Container const& c) {
  for (auto const& v : c)
    add(v);
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::toFront(iterator i) -> iterator {
  m_order.splice(m_order.begin(), m_order, i);
  return m_order.begin();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::toBack(iterator i) -> iterator {
  m_order.splice(m_order.end(), m_order, i);
  return prev(m_order.end());
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
bool OrderedSetWrapper<Map, Value, Allocator, Args...>::remove(value_type const& v) {
  auto i = m_map.find(v);
  if (i != m_map.end()) {
    auto orderIt = i->second;
    m_map.erase(i);
    m_order.erase(orderIt);
    return true;
  }
  return false;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
template <typename Container>
void OrderedSetWrapper<Map, Value, Allocator, Args...>::removeAll(Container const& c) {
  for (auto const& v : c)
    remove(v);
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
void OrderedSetWrapper<Map, Value, Allocator, Args...>::clear() {
  m_map.clear();
  m_order.clear();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::first() const -> value_type const& {
  if (empty())
    throw SetException("first() called on empty OrderedSet");
  return *begin();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::last() const -> value_type const& {
  if (empty())
    throw SetException("last() called on empty OrderedSet");
  return *(prev(end()));
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
void OrderedSetWrapper<Map, Value, Allocator, Args...>::removeFirst() {
  if (empty())
    throw SetException("OrderedSet::removeFirst() called on empty OrderedSet");

  auto i = m_order.begin();
  m_map.erase(*i);
  m_order.erase(i);
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
void OrderedSetWrapper<Map, Value, Allocator, Args...>::removeLast() {
  if (empty())
    throw SetException("OrderedSet::removeLast() called on empty OrderedSet");

  auto i = m_order.end();
  --i;
  m_map.erase(*i);
  m_order.erase(i);
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::takeFirst() -> value_type {
  if (empty())
    throw SetException("OrderedSet::takeFirst() called on empty OrderedSet");

  auto i = m_order.begin();
  m_map.erase(*i);
  value_type v = *i;
  m_order.erase(i);
  return v;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::takeLast() -> value_type {
  if (empty())
    throw SetException("OrderedSet::takeLast() called on empty OrderedSet");

  auto i = m_order.end();
  --i;
  m_map.erase(*i);
  value_type v = *i;
  m_order.erase(i);
  return v;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
template <typename Compare>
void OrderedSetWrapper<Map, Value, Allocator, Args...>::sort(Compare comp) {
  m_order.sort(comp);
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
void OrderedSetWrapper<Map, Value, Allocator, Args...>::sort() {
  m_order.sort();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
size_t OrderedSetWrapper<Map, Value, Allocator, Args...>::empty() const {
  return m_map.empty();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
size_t OrderedSetWrapper<Map, Value, Allocator, Args...>::size() const {
  return m_map.size();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::begin() const -> const_iterator {
  return m_order.begin();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::end() const -> const_iterator {
  return m_order.end();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::rbegin() const -> const_reverse_iterator {
  return m_order.rbegin();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::rend() const -> const_reverse_iterator {
  return m_order.rend();
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
Maybe<size_t> OrderedSetWrapper<Map, Value, Allocator, Args...>::indexOf(value_type const& v) const {
  auto i = m_map.find(v);
  if (i == m_map.end())
    return {};

  return std::distance(begin(), const_iterator(i->second));
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::at(size_t i) const -> value_type const& {
  auto it = begin();
  std::advance(it, i);
  return *it;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::at(size_t i) -> value_type& {
  auto it = begin();
  std::advance(it, i);
  return *it;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::intersection(OrderedSetWrapper const& s) const -> OrderedSetWrapper {
  OrderedSetWrapper ret;
  for (auto const& e : s) {
    if (contains(e))
      ret.add(e);
  }
  return ret;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
auto OrderedSetWrapper<Map, Value, Allocator, Args...>::difference(OrderedSetWrapper const& s) const -> OrderedSetWrapper {
  OrderedSetWrapper ret;
  for (auto const& e : *this) {
    if (!s.contains(e))
      ret.add(e);
  }
  return ret;
}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
std::ostream& operator<<(std::ostream& os, OrderedSetWrapper<Map, Value, Allocator, Args...> const& set) {
  os << "(";
  for (auto i = set.begin(); i != set.end(); ++i) {
    if (i != set.begin())
      os << ", ";
    os << *i;
  }
  os << ")";
  return os;
}

}

template <template <typename...> class Map, typename Value, typename Allocator, typename... Args>
struct fmt::formatter<Star::OrderedSetWrapper<Map, Value, Allocator, Args...>> : ostream_formatter {};

#endif
