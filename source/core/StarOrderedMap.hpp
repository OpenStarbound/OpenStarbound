#ifndef STAR_ORDERED_MAP_HPP
#define STAR_ORDERED_MAP_HPP

#include "StarMap.hpp"

namespace Star {

// Wraps a normal map type and provides an element order independent of the
// underlying map order.
template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
class OrderedMapWrapper {
public:
  typedef Key key_type;
  typedef Value mapped_type;
  typedef pair<key_type const, mapped_type> value_type;

  typedef LinkedList<value_type, Allocator> OrderType;
  typedef Map<
      std::reference_wrapper<key_type const>, typename OrderType::iterator, MapArgs...,
      typename Allocator::template rebind<pair<std::reference_wrapper<key_type const> const, typename OrderType::iterator>>::other
    > MapType;

  typedef typename OrderType::iterator iterator;
  typedef typename OrderType::const_iterator const_iterator;

  typedef typename OrderType::reverse_iterator reverse_iterator;
  typedef typename OrderType::const_reverse_iterator const_reverse_iterator;

  typedef typename std::decay<mapped_type>::type* mapped_ptr;
  typedef typename std::decay<mapped_type>::type const* mapped_const_ptr;

  template <typename Collection>
  static OrderedMapWrapper from(Collection const& c);

  OrderedMapWrapper();

  OrderedMapWrapper(OrderedMapWrapper const& map);

  template <typename InputIterator>
  OrderedMapWrapper(InputIterator beg, InputIterator end);

  OrderedMapWrapper(initializer_list<value_type> list);

  List<key_type> keys() const;
  List<mapped_type> values() const;
  List<pair<key_type, mapped_type>> pairs() const;

  bool contains(key_type const& k) const;

  // Throws MapException if key not found
  mapped_type& get(key_type const& k);
  mapped_type const& get(key_type const& k) const;

  // Return def if key not found
  mapped_type value(key_type const& k, mapped_type d = mapped_type()) const;

  Maybe<mapped_type> maybe(key_type const& k) const;

  mapped_const_ptr ptr(key_type const& k) const;
  mapped_ptr ptr(key_type const& k);

  mapped_type& operator[](key_type const& k);

  OrderedMapWrapper& operator=(OrderedMapWrapper const& map);

  bool operator==(OrderedMapWrapper const& m) const;

  // Finds first value matching the given value and returns its key, throws
  // MapException if no such value is found.
  key_type keyOf(mapped_type const& v) const;

  // Finds all of the values matching the given value and returns their keys.
  List<key_type> keysOf(mapped_type const& v) const;

  pair<iterator, bool> insert(value_type const& v);
  pair<iterator, bool> insert(key_type k, mapped_type v);

  pair<iterator, bool> insertFront(value_type const& v);
  pair<iterator, bool> insertFront(key_type k, mapped_type v);

  // Add a key / value pair, throw if the key already exists
  mapped_type& add(key_type k, mapped_type v);

  // Set a key to a value, always override if it already exists
  mapped_type& set(key_type k, mapped_type v);

  // Appends all values of given map into this map.  If overwite is false, then
  // skips values that already exist in this map.  Returns false if any keys
  // previously existed.
  bool merge(OrderedMapWrapper const& m, bool overwrite = false);

  // Removes the item with key k and returns true if found, false otherwise.
  bool remove(key_type const& k);

  // Remove and return the value with the key k, throws MapException if not
  // found.
  mapped_type take(key_type const& k);

  Maybe<value_type> maybeTake(key_type const& k);

  const_iterator begin() const;
  const_iterator end() const;

  iterator begin();
  iterator end();

  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;

  reverse_iterator rbegin();
  reverse_iterator rend();

  size_t size() const;

  iterator erase(iterator i);
  size_t erase(key_type const& k);

  iterator find(key_type const& k);
  const_iterator find(key_type const& k) const;

  Maybe<size_t> indexOf(key_type const& k) const;

  key_type const& keyAt(size_t i) const;
  mapped_type const& valueAt(size_t i) const;
  mapped_type& valueAt(size_t i);

  value_type takeFirst();
  void removeFirst();

  value_type const& first() const;

  key_type const& firstKey() const;
  mapped_type& firstValue();
  mapped_type const& firstValue() const;

  iterator insert(iterator pos, value_type v);

  void clear();

  bool empty() const;

  iterator toBack(iterator i);
  void toBack(key_type const& k);

  iterator toFront(iterator i);
  void toFront(key_type const& k);

  template <typename Compare>
  void sort(Compare comp);

  void sortByKey();
  void sortByValue();

private:
  MapType m_map;
  OrderType m_order;
};

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
std::ostream& operator<<(std::ostream& os, OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...> const& m);

template <typename Key, typename Value, typename Compare = std::less<Key>, typename Allocator = std::allocator<pair<Key const, Value>>>
using OrderedMap = OrderedMapWrapper<std::map, Key, Value, Allocator, Compare>;

template <typename Key, typename Value, typename Hash = Star::hash<Key>, typename Equals = std::equal_to<Key>, typename Allocator = std::allocator<pair<Key const, Value>>>
using OrderedHashMap = OrderedMapWrapper<FlatHashMap, Key, Value, Allocator, Hash, Equals>;

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
template <typename Collection>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::from(Collection const& c) -> OrderedMapWrapper {
  return OrderedMapWrapper(c.begin(), c.end());
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::OrderedMapWrapper() {}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::OrderedMapWrapper(OrderedMapWrapper const& map) {
  for (auto const& p : map)
    insert(p);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
template <typename InputIterator>
OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::OrderedMapWrapper(InputIterator beg, InputIterator end) {
  while (beg != end) {
    insert(*beg);
    ++beg;
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::OrderedMapWrapper(initializer_list<value_type> list) {
  for (value_type v : list)
    insert(move(v));
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::keys() const -> List<key_type> {
  List<key_type> keys;
  for (auto const& p : *this)
    keys.append(p.first);
  return keys;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::values() const -> List<mapped_type> {
  List<mapped_type> values;
  for (auto const& p : *this)
    values.append(p.second);
  return values;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::pairs() const -> List<pair<key_type, mapped_type>> {
  List<pair<key_type, mapped_type>> plist;
  for (auto const& p : *this)
    plist.append(p.second);
  return plist;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
bool OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::contains(key_type const& k) const {
  return m_map.find(k) != m_map.end();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::get(key_type const& k) -> mapped_type& {
  auto i = m_map.find(k);
  if (i == m_map.end())
    throw MapException(strf("Key '{}' not found in OrderedMap::get()", outputAny(k)));

  return i->second->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::get(key_type const& k) const -> mapped_type const& {
  return const_cast<OrderedMapWrapper*>(this)->get(k);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::value(key_type const& k, mapped_type d) const -> mapped_type {
  auto i = m_map.find(k);
  if (i == m_map.end())
    return move(d);
  else
    return i->second->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::maybe(key_type const& k) const -> Maybe<mapped_type> {
  auto i = find(k);
  if (i == end())
    return {};
  else
    return i->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::ptr(key_type const& k) const -> mapped_const_ptr {
  auto i = find(k);
  if (i == end())
    return nullptr;
  else
    return &i->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::ptr(key_type const& k) -> mapped_ptr {
  iterator i = find(k);
  if (i == end())
    return nullptr;
  else
    return &i->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::operator[](key_type const& k) -> mapped_type& {
  auto i = m_map.find(k);
  if (i == m_map.end()) {
    iterator orderIt = m_order.insert(m_order.end(), value_type(k, mapped_type()));
    i = m_map.insert(typename MapType::value_type(std::cref(orderIt->first), orderIt)).first;
    return orderIt->second;
  } else {
    return i->second->second;
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::operator=(OrderedMapWrapper const& map) -> OrderedMapWrapper& {
  if (this != &map) {
    clear();
    for (auto const& p : map)
      insert(p);
  }

  return *this;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
bool OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::operator==(OrderedMapWrapper const& m) const {
  return this == &m || mapsEqual(*this, m);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::keyOf(mapped_type const& v) const -> key_type {
  for (const_iterator i = begin(); i != end(); ++i) {
    if (i->second == v)
      return i->first;
  }
  throw MapException(strf("Value '{}' not found in OrderedMap::keyOf()", outputAny(v)));
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::keysOf(mapped_type const& v) const -> List<key_type> {
  List<key_type> keys;
  for (iterator i = begin(); i != end(); ++i) {
    if (i->second == v)
      keys.append(i->first);
  }
  return keys;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::insert(value_type const& v) -> pair<iterator, bool> {
  auto i = m_map.find(v.first);
  if (i == m_map.end()) {
    iterator orderIt = m_order.insert(m_order.end(), v);
    m_map.insert(i, typename MapType::value_type(std::cref(orderIt->first), orderIt));
    return std::make_pair(orderIt, true);
  } else {
    return std::make_pair(i->second, false);
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::insert(key_type k, mapped_type v) -> pair<iterator, bool> {
  return insert(value_type(move(k), move(v)));
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::insertFront(value_type const& v) -> pair<iterator, bool> {
  auto i = m_map.find(v.first);
  if (i == m_map.end()) {
    iterator orderIt = m_order.insert(m_order.begin(), v);
    m_map.insert(i, typename MapType::value_type(std::cref(orderIt->first), orderIt));
    return std::make_pair(orderIt, true);
  } else {
    return std::make_pair(i->second, false);
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::insertFront(key_type k, mapped_type v) -> pair<iterator, bool> {
  return insertFront(value_type(move(k), move(v)));
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::add(key_type k, mapped_type v) -> mapped_type& {
  auto pair = insert(value_type(move(k), move(v)));
  if (!pair.second)
    throw MapException(strf("Entry with key '{}' already present.", outputAny(k)));
  else
    return pair.first->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::set(key_type k, mapped_type v) -> mapped_type& {
  auto i = find(k);
  if (i != end()) {
    i->second = move(v);
    return i->second;
  } else {
    return insert(value_type(move(k), move(v))).first->second;
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
bool OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::merge(OrderedMapWrapper const& m, bool overwrite) {
  return mapMerge(*this, m, overwrite);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
bool OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::remove(key_type const& k) {
  auto i = m_map.find(k);
  if (i != m_map.end()) {
    auto orderIt = i->second;
    m_map.erase(i);

    m_order.erase(orderIt);
    return true;
  } else {
    return false;
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::take(key_type const& k) -> mapped_type {
  auto i = m_map.find(k);
  if (i != m_map.end()) {
    auto orderIt = i->second;
    m_map.erase(i);

    mapped_type v = orderIt->second;
    m_order.erase(i->second);
    return v;
  } else {
    throw MapException(strf("Key '{}' not found in OrderedMap::take()", outputAny(k)));
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::maybeTake(key_type const& k) -> Maybe<value_type> {
  iterator i = find(k);
  if (i != end()) {
    value_type v = *i;
    erase(i);
    return v;
  }

  return {};
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::begin() const -> const_iterator {
  return m_order.begin();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::end() const -> const_iterator {
  return m_order.end();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::begin() -> iterator {
  return m_order.begin();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::end() -> iterator {
  return m_order.end();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::rbegin() const -> const_reverse_iterator {
  return m_order.rbegin();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::rend() const -> const_reverse_iterator {
  return m_order.rend();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::rbegin() -> reverse_iterator {
  return m_order.rbegin();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::rend() -> reverse_iterator {
  return m_order.rend();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::size() const -> size_t {
  return m_map.size();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::erase(iterator i) -> iterator {
  m_map.erase(i->first);
  return m_order.erase(i);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::erase(key_type const& k) -> size_t {
  if (remove(k))
    return 1;
  return 0;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::find(key_type const& k) -> iterator {
  auto i = m_map.find(k);
  if (i == m_map.end())
    return m_order.end();
  else
    return i->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::find(key_type const& k) const -> const_iterator {
  auto i = m_map.find(k);
  if (i == m_map.end())
    return m_order.end();
  else
    return i->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::indexOf(key_type const& k) const -> Maybe<size_t> {
  typename MapType::const_iterator i = m_map.find(k);
  if (i == m_map.end())
    return {};

  return std::distance(begin(), const_iterator(i->second));
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::keyAt(size_t i) const -> key_type const& {
  if (i >= size())
    throw MapException(strf("index {} out of range in OrderedMap::at()", i));

  auto it = begin();
  std::advance(it, i);
  return it->first;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::valueAt(size_t i) const -> mapped_type const& {
  return const_cast<OrderedMapWrapper*>(this)->valueAt(i);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::valueAt(size_t i) -> mapped_type& {
  if (i >= size())
    throw MapException(strf("index {} out of range in OrderedMap::valueAt()", i));

  auto it = m_order.begin();
  std::advance(it, i);
  return it->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::takeFirst() -> value_type {
  if (empty())
    throw MapException("OrderedMap::takeFirst() called on empty OrderedMap");

  iterator i = m_order.begin();
  m_map.remove(i->first);
  value_type v = *i;
  m_order.erase(i);
  return v;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
void OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::removeFirst() {
  erase(begin());
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::first() const -> value_type const& {
  if (empty())
    throw MapException("OrderedMap::takeFirst() called on empty OrderedMap");

  return *m_order.begin();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::firstValue() -> mapped_type& {
  return begin()->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::firstValue() const -> mapped_type const& {
  return begin()->second;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::firstKey() const -> key_type const& {
  return begin()->first;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::insert(iterator pos, value_type v) -> iterator {
  auto i = m_map.find(v.first);
  if (i == m_map.end()) {
    iterator orderIt = m_order.insert(pos, move(v));
    m_map.insert(typename MapType::value_type(std::cref(orderIt->first), orderIt));
    return orderIt;
  } else {
    i->second->second = move(v.second);
    m_order.splice(pos, m_order, i->second);
    return i->second;
  }
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
void OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::clear() {
  m_map.clear();
  m_order.clear();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
bool OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::empty() const {
  return size() == 0;
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::toBack(iterator i) -> iterator {
  m_order.splice(m_order.end(), m_order, i);
  return prev(m_order.end());
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
auto OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::toFront(iterator i) -> iterator {
  m_order.splice(m_order.begin(), m_order, i);
  return m_order.begin();
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
void OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::toBack(key_type const& k) {
  auto i = m_map.find(k);
  if (i == m_map.end())
    throw MapException(strf("Key not found in OrderedMap::toBack('{}')", outputAny(k)));

  toBack(i->second);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
void OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::toFront(key_type const& k) {
  auto i = m_map.find(k);
  if (i == m_map.end())
    throw MapException(strf("Key not found in OrderedMap::toFront('{}')", outputAny(k)));

  toFront(i->second);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
template <typename Compare>
void OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::sort(Compare comp) {
  m_order.sort(comp);
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
void OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::sortByKey() {
  sort([](value_type const& a, value_type const& b) {
      return a.first < b.first;
    });
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
void OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>::sortByValue() {
  sort([](value_type const& a, value_type const& b) {
      return a.second < b.second;
    });
}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
std::ostream& operator<<(std::ostream& os, OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...> const& m) {
  printMap(os, m);
  return os;
}

}

template <template <typename...> class Map, typename Key, typename Value, typename Allocator, typename... MapArgs>
struct fmt::formatter<Star::OrderedMapWrapper<Map, Key, Value, Allocator, MapArgs...>> : ostream_formatter {};

#endif
