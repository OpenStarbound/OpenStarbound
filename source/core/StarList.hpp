#ifndef STAR_LIST_HPP
#define STAR_LIST_HPP

#include <vector>
#include <deque>
#include <list>

#include "StarException.hpp"
#include "StarStaticVector.hpp"
#include "StarSmallVector.hpp"
#include "StarPythonic.hpp"
#include "StarMaybe.hpp"
#include "StarFormat.hpp"

namespace Star {

template <typename BaseList>
class ListMixin : public BaseList {
public:
  typedef BaseList Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  ListMixin();
  ListMixin(Base const& list);
  ListMixin(Base&& list);
  ListMixin(value_type const* p, size_t count);
  template <typename InputIterator>
  ListMixin(InputIterator beg, InputIterator end);
  explicit ListMixin(size_t len, const_reference s1 = value_type());
  ListMixin(initializer_list<value_type> list);

  void append(value_type e);

  template <typename Container>
  void appendAll(Container&& list);

  template <class... Args>
  reference emplaceAppend(Args&&... args);

  reference first();
  const_reference first() const;

  reference last();
  const_reference last() const;

  Maybe<value_type> maybeFirst();
  Maybe<value_type> maybeLast();

  void removeLast();
  value_type takeLast();

  Maybe<value_type> maybeTakeLast();

  // Limit the size of the list by removing elements from the back until the
  // size is the maximumSize or less.
  void limitSizeBack(size_t maximumSize);

  size_t count() const;

  bool contains(const_reference e) const;
  // Remove all equal to element, returns number removed.
  size_t remove(const_reference e);

  template <typename Filter>
  void filter(Filter&& filter);

  template <typename Comparator>
  void insertSorted(value_type e, Comparator&& comparator);
  void insertSorted(value_type e);

  // Returns true if this *sorted* list contains the given element.
  template <typename Comparator>
  bool containsSorted(value_type const& e, Comparator&& comparator);
  bool containsSorted(value_type e);

  template <typename Function>
  void exec(Function&& function);

  template <typename Function>
  void exec(Function&& function) const;

  template <typename Function>
  void transform(Function&& function);

  template <typename Function>
  bool any(Function&& function) const;
  bool any() const;

  template <typename Function>
  bool all(Function&& function) const;
  bool all() const;
};

template <typename List>
class ListHasher {
public:
  size_t operator()(List const& l) const;

private:
  hash<typename List::value_type> elemHasher;
};

template <typename BaseList>
class RandomAccessListMixin : public BaseList {
public:
  typedef BaseList Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  using Base::Base;

  template <typename Comparator>
  void sort(Comparator&& comparator);
  void sort();

  void reverse();

  // Returns first index of given element, NPos if not found.
  size_t indexOf(const_reference e, size_t from = 0) const;
  // Returns last index of given element, NPos if not found.
  size_t lastIndexOf(const_reference e, size_t til = NPos) const;

  const_reference at(size_t n) const;
  reference at(size_t n);

  const_reference operator[](size_t n) const;
  reference operator[](size_t n);

  // Does not throw if n is beyond end of list, instead returns def
  value_type get(size_t n, value_type def = value_type()) const;

  value_type takeAt(size_t i);

  // Same as at, but wraps around back to the beginning
  // (throws if list is empty)
  const_reference wrap(size_t n) const;
  reference wrap(size_t n);

  // Does not throw if list is empty
  value_type wrap(size_t n, value_type def) const;

  void eraseAt(size_t index);
  // Erases region from begin to end, not including end.
  void eraseAt(size_t begin, size_t end);

  void insertAt(size_t pos, value_type e);

  template <typename Container>
  void insertAllAt(size_t pos, Container const& l);

  // Ensures that list is large enough to hold pos elements.
  void set(size_t pos, value_type e);

  void swap(size_t i, size_t j);
  // same as insert(to, takeAt(from))
  void move(size_t from, size_t to);
};

template <typename BaseList>
class FrontModifyingListMixin : public BaseList {
public:
  typedef BaseList Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  using Base::Base;

  void prepend(value_type e);

  template <typename Container>
  void prependAll(Container&& list);

  template <class... Args>
  reference emplacePrepend(Args&&... args);

  void removeFirst();
  value_type takeFirst();

  // Limit the size of the list by removing elements from the front until the
  // size is the maximumSize or less.
  void limitSizeFront(size_t maximumSize);
};

template <typename Element, typename Allocator = std::allocator<Element>>
class List : public RandomAccessListMixin<ListMixin<std::vector<Element, Allocator>>> {
public:
  typedef RandomAccessListMixin<ListMixin<std::vector<Element, Allocator>>> Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  template <typename Container>
  static List from(Container const& c);

  using Base::Base;

  // Pointer to contiguous storage, returns nullptr if empty
  value_type* ptr();
  value_type const* ptr() const;

  List slice(SliceIndex a = SliceIndex(), SliceIndex b = SliceIndex(), int i = 1) const;

  template <typename Filter>
  List filtered(Filter&& filter) const;

  template <typename Comparator>
  List sorted(Comparator&& comparator) const;
  List sorted() const;

  template <typename Function>
  auto transformed(Function&& function);

  template <typename Function>
  auto transformed(Function&& function) const;
};

template <typename Element, typename Allocator>
struct hash<List<Element, Allocator>> : public ListHasher<List<Element, Allocator>> {};

template <typename Element, size_t MaxSize>
class StaticList : public RandomAccessListMixin<ListMixin<StaticVector<Element, MaxSize>>> {
public:
  typedef RandomAccessListMixin<ListMixin<StaticVector<Element, MaxSize>>> Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  template <typename Container>
  static StaticList from(Container const& c);

  using Base::Base;

  StaticList slice(SliceIndex a = SliceIndex(), SliceIndex b = SliceIndex(), int i = 1) const;

  template <typename Filter>
  StaticList filtered(Filter&& filter) const;

  template <typename Comparator>
  StaticList sorted(Comparator&& comparator) const;
  StaticList sorted() const;

  template <typename Function>
  auto transformed(Function&& function);

  template <typename Function>
  auto transformed(Function&& function) const;
};

template <typename Element, size_t MaxStackSize>
struct hash<StaticList<Element, MaxStackSize>> : public ListHasher<StaticList<Element, MaxStackSize>> {};

template <typename Element, size_t MaxStackSize>
class SmallList : public RandomAccessListMixin<ListMixin<SmallVector<Element, MaxStackSize>>> {
public:
  typedef RandomAccessListMixin<ListMixin<SmallVector<Element, MaxStackSize>>> Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  template <typename Container>
  static SmallList from(Container const& c);

  using Base::Base;

  SmallList slice(SliceIndex a = SliceIndex(), SliceIndex b = SliceIndex(), int i = 1) const;

  template <typename Filter>
  SmallList filtered(Filter&& filter) const;

  template <typename Comparator>
  SmallList sorted(Comparator&& comparator) const;
  SmallList sorted() const;

  template <typename Function>
  auto transformed(Function&& function);

  template <typename Function>
  auto transformed(Function&& function) const;
};

template <typename Element, size_t MaxStackSize>
struct hash<SmallList<Element, MaxStackSize>> : public ListHasher<SmallList<Element, MaxStackSize>> {};

template <typename Element, typename Allocator = std::allocator<Element>>
class Deque : public FrontModifyingListMixin<RandomAccessListMixin<ListMixin<std::deque<Element, Allocator>>>> {
public:
  typedef FrontModifyingListMixin<RandomAccessListMixin<ListMixin<std::deque<Element, Allocator>>>> Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  template <typename Container>
  static Deque from(Container const& c);

  using Base::Base;

  Deque slice(SliceIndex a = SliceIndex(), SliceIndex b = SliceIndex(), int i = 1) const;

  template <typename Filter>
  Deque filtered(Filter&& filter) const;

  template <typename Comparator>
  Deque sorted(Comparator&& comparator) const;
  Deque sorted() const;

  template <typename Function>
  auto transformed(Function&& function);

  template <typename Function>
  auto transformed(Function&& function) const;
};

template <typename Element, typename Allocator>
struct hash<Deque<Element, Allocator>> : public ListHasher<Deque<Element, Allocator>> {};

template <typename Element, typename Allocator = std::allocator<Element>>
class LinkedList : public FrontModifyingListMixin<ListMixin<std::list<Element, Allocator>>> {
public:
  typedef FrontModifyingListMixin<ListMixin<std::list<Element, Allocator>>> Base;

  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;

  template <typename Container>
  static LinkedList from(Container const& c);

  using Base::Base;

  void appendAll(LinkedList list);
  void prependAll(LinkedList list);

  template <typename Container>
  void appendAll(Container&& list);
  template <typename Container>
  void prependAll(Container&& list);

  template <typename Filter>
  LinkedList filtered(Filter&& filter) const;

  template <typename Comparator>
  LinkedList sorted(Comparator&& comparator) const;
  LinkedList sorted() const;

  template <typename Function>
  auto transformed(Function&& function);

  template <typename Function>
  auto transformed(Function&& function) const;
};

template <typename Element, typename Allocator>
struct hash<LinkedList<Element, Allocator>> : public ListHasher<LinkedList<Element, Allocator>> {};

template <typename BaseList>
std::ostream& operator<<(std::ostream& os, ListMixin<BaseList> const& list);

template <typename... Containers>
struct ListZipTypes {
  typedef tuple<typename std::decay<Containers>::type::value_type...> Tuple;
  typedef List<Tuple> Result;
};

template <typename... Containers>
typename ListZipTypes<Containers...>::Result zip(Containers&&... args);

template <typename Container>
struct ListEnumerateTypes {
  typedef pair<typename std::decay<Container>::type::value_type, size_t> Pair;
  typedef List<Pair> Result;
};

template <typename Container>
typename ListEnumerateTypes<Container>::Result enumerate(Container&& container);

template <typename BaseList>
ListMixin<BaseList>::ListMixin()
  : Base() {}

template <typename BaseList>
ListMixin<BaseList>::ListMixin(Base const& list)
  : Base(list) {}

template <typename BaseList>
ListMixin<BaseList>::ListMixin(Base&& list)
  : Base(std::move(list)) {}

template <typename BaseList>
ListMixin<BaseList>::ListMixin(size_t len, const_reference s1)
  : Base(len, s1) {}

template <typename BaseList>
ListMixin<BaseList>::ListMixin(value_type const* p, size_t count)
  : Base(p, p + count) {}

template <typename BaseList>
template <typename InputIterator>
ListMixin<BaseList>::ListMixin(InputIterator beg, InputIterator end)
  : Base(beg, end) {}

template <typename BaseList>
ListMixin<BaseList>::ListMixin(initializer_list<value_type> list) {
  // In case underlying class type doesn't support initializer_list
  for (auto& e : list)
    append(std::move(e));
}

template <typename BaseList>
void ListMixin<BaseList>::append(value_type e) {
  Base::push_back(std::move(e));
}

template <typename BaseList>
template <typename Container>
void ListMixin<BaseList>::appendAll(Container&& list) {
  for (auto& e : list) {
    if (std::is_rvalue_reference<Container&&>::value)
      Base::push_back(std::move(e));
    else
      Base::push_back(e);
  }
}

template <typename BaseList>
template <class... Args>
auto ListMixin<BaseList>::emplaceAppend(Args&&... args) -> reference {
  Base::emplace_back(std::forward<Args>(args)...);
  return *prev(Base::end());
}

template <typename BaseList>
auto ListMixin<BaseList>::first() -> reference {
  if (Base::empty())
    throw OutOfRangeException("first() called on empty list");
  return *Base::begin();
}

template <typename BaseList>
auto ListMixin<BaseList>::first() const -> const_reference {
  if (Base::empty())
    throw OutOfRangeException("first() called on empty list");
  return *Base::begin();
}

template <typename BaseList>
auto ListMixin<BaseList>::last() -> reference {
  if (Base::empty())
    throw OutOfRangeException("last() called on empty list");
  return *prev(Base::end());
}

template <typename BaseList>
auto ListMixin<BaseList>::last() const -> const_reference {
  if (Base::empty())
    throw OutOfRangeException("last() called on empty list");
  return *prev(Base::end());
}

template <typename BaseList>
auto ListMixin<BaseList>::maybeFirst() -> Maybe<value_type> {
  if (Base::empty())
    return {};
  return *Base::begin();
}

template <typename BaseList>
auto ListMixin<BaseList>::maybeLast() -> Maybe<value_type> {
  if (Base::empty())
    return {};
  return *prev(Base::end());
}

template <typename BaseList>
void ListMixin<BaseList>::removeLast() {
  if (Base::empty())
    throw OutOfRangeException("removeLast() called on empty list");
  Base::pop_back();
}

template <typename BaseList>
auto ListMixin<BaseList>::takeLast() -> value_type {
  value_type e = std::move(last());
  Base::pop_back();
  return e;
}

template <typename BaseList>
auto ListMixin<BaseList>::maybeTakeLast() -> Maybe<value_type> {
  if (Base::empty())
    return {};
  value_type e = std::move(last());
  Base::pop_back();
  return e;
}

template <typename BaseList>
void ListMixin<BaseList>::limitSizeBack(size_t maximumSize) {
  while (Base::size() > maximumSize)
    Base::pop_back();
}

template <typename BaseList>
size_t ListMixin<BaseList>::count() const {
  return Base::size();
}

template <typename BaseList>
bool ListMixin<BaseList>::contains(const_reference e) const {
  for (auto const& r : *this) {
    if (r == e)
      return true;
  }
  return false;
}

template <typename BaseList>
size_t ListMixin<BaseList>::remove(const_reference e) {
  size_t removed = 0;
  auto i = Base::begin();
  while (i != Base::end()) {
    if (*i == e) {
      ++removed;
      i = Base::erase(i);
    } else {
      ++i;
    }
  }
  return removed;
}

template <typename BaseList>
template <typename Filter>
void ListMixin<BaseList>::filter(Filter&& filter) {
  Star::filter(*this, std::forward<Filter>(filter));
}

template <typename BaseList>
template <typename Comparator>
void ListMixin<BaseList>::insertSorted(value_type e, Comparator&& comparator) {
  auto i = std::upper_bound(Base::begin(), Base::end(), e, std::forward<Comparator>(comparator));
  Base::insert(i, std::move(e));
}

template <typename BaseList>
void ListMixin<BaseList>::insertSorted(value_type e) {
  auto i = std::upper_bound(Base::begin(), Base::end(), e);
  Base::insert(i, std::move(e));
}

template <typename BaseList>
template <typename Comparator>
bool ListMixin<BaseList>::containsSorted(value_type const& e, Comparator&& comparator) {
  auto range = std::equal_range(Base::begin(), Base::end(), e, std::forward<Comparator>(comparator));
  return range.first != range.second;
}

template <typename BaseList>
bool ListMixin<BaseList>::containsSorted(value_type e) {
  auto range = std::equal_range(Base::begin(), Base::end(), e);
  return range.first != range.second;
}

template <typename BaseList>
template <typename Function>
void ListMixin<BaseList>::exec(Function&& function) {
  for (auto& e : *this)
    function(e);
}

template <typename BaseList>
template <typename Function>
void ListMixin<BaseList>::exec(Function&& function) const {
  for (auto const& e : *this)
    function(e);
}

template <typename BaseList>
template <typename Function>
void ListMixin<BaseList>::transform(Function&& function) {
  for (auto& e : *this)
    e = function(e);
}

template <typename BaseList>
template <typename Function>
bool ListMixin<BaseList>::any(Function&& function) const {
  return Star::any(*this, std::forward<Function>(function));
}

template <typename BaseList>
bool ListMixin<BaseList>::any() const {
  return Star::any(*this);
}

template <typename BaseList>
template <typename Function>
bool ListMixin<BaseList>::all(Function&& function) const {
  return Star::all(*this, std::forward<Function>(function));
}

template <typename BaseList>
bool ListMixin<BaseList>::all() const {
  return Star::all(*this);
}

template <typename BaseList>
template <typename Comparator>
void RandomAccessListMixin<BaseList>::sort(Comparator&& comparator) {
  Star::sort(*this, std::forward<Comparator>(comparator));
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::sort() {
  Star::sort(*this);
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::reverse() {
  Star::reverse(*this);
}

template <typename BaseList>
size_t RandomAccessListMixin<BaseList>::indexOf(const_reference e, size_t from) const {
  for (size_t i = from; i < Base::size(); ++i)
    if (operator[](i) == e)
      return i;
  return NPos;
}

template <typename BaseList>
size_t RandomAccessListMixin<BaseList>::lastIndexOf(const_reference e, size_t til) const {
  size_t index = NPos;
  size_t end = std::min(Base::size(), til);
  for (size_t i = 0; i < end; ++i) {
    if (operator[](i) == e)
      index = i;
  }
  return index;
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::at(size_t n) const -> const_reference {
  if (n >= Base::size())
    throw OutOfRangeException(strf("out of range list::at({})", n));
  return operator[](n);
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::at(size_t n) -> reference {
  if (n >= Base::size())
    throw OutOfRangeException(strf("out of range list::at({})", n));
  return operator[](n);
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::operator[](size_t n) const -> const_reference {
  starAssert(n < Base::size());
  return Base::operator[](n);
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::operator[](size_t n) -> reference {
  starAssert(n < Base::size());
  return Base::operator[](n);
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::get(size_t n, value_type def) const -> value_type {
  if (n >= BaseList::size())
    return def;
  return operator[](n);
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::takeAt(size_t i) -> value_type {
  value_type e = at(i);
  Base::erase(Base::begin() + i);
  return e;
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::wrap(size_t n) const -> const_reference {
  if (BaseList::empty())
    throw OutOfRangeException();
  else
    return operator[](n % BaseList::size());
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::wrap(size_t n) -> reference {
  if (BaseList::empty())
    throw OutOfRangeException();
  else
    return operator[](n % BaseList::size());
}

template <typename BaseList>
auto RandomAccessListMixin<BaseList>::wrap(size_t n, value_type def) const -> value_type {
  if (BaseList::empty())
    return def;
  else
    return operator[](n % BaseList::size());
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::eraseAt(size_t i) {
  starAssert(i < Base::size());
  Base::erase(Base::begin() + i);
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::eraseAt(size_t b, size_t e) {
  starAssert(b < Base::size() && e <= Base::size());
  Base::erase(Base::begin() + b, Base::begin() + e);
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::insertAt(size_t pos, value_type e) {
  starAssert(pos <= Base::size());
  Base::insert(Base::begin() + pos, std::move(e));
}

template <typename BaseList>
template <typename Container>
void RandomAccessListMixin<BaseList>::insertAllAt(size_t pos, Container const& l) {
  starAssert(pos <= Base::size());
  Base::insert(Base::begin() + pos, l.begin(), l.end());
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::set(size_t pos, value_type e) {
  if (pos >= Base::size())
    Base::resize(pos + 1);
  operator[](pos) = std::move(e);
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::swap(size_t i, size_t j) {
  std::swap(operator[](i), operator[](j));
}

template <typename BaseList>
void RandomAccessListMixin<BaseList>::move(size_t from, size_t to) {
  Base::insert(to, takeAt(from));
}

template <typename BaseList>
void FrontModifyingListMixin<BaseList>::prepend(value_type e) {
  Base::push_front(std::move(e));
}

template <typename BaseList>
template <typename Container>
void FrontModifyingListMixin<BaseList>::prependAll(Container&& list) {
  for (auto i = std::rbegin(list); i != std::rend(list); ++i) {
    if (std::is_rvalue_reference<Container&&>::value)
      Base::push_front(std::move(*i));
    else
      Base::push_front(*i);
  }
}

template <typename BaseList>
template <class... Args>
auto FrontModifyingListMixin<BaseList>::emplacePrepend(Args&&... args) -> reference {
  Base::emplace_front(std::forward<Args>(args)...);
  return *Base::begin();
}

template <typename BaseList>
void FrontModifyingListMixin<BaseList>::removeFirst() {
  if (Base::empty())
    throw OutOfRangeException("removeFirst() called on empty list");
  Base::pop_front();
}

template <typename BaseList>
auto FrontModifyingListMixin<BaseList>::takeFirst() -> value_type {
  value_type e = std::move(Base::first());
  Base::pop_front();
  return e;
}

template <typename BaseList>
void FrontModifyingListMixin<BaseList>::limitSizeFront(size_t maximumSize) {
  while (Base::size() > maximumSize)
    Base::pop_front();
}

template <typename Element, typename Allocator>
template <typename Container>
List<Element, Allocator> List<Element, Allocator>::from(Container const& c) {
  return List(c.begin(), c.end());
}

template <typename Element, typename Allocator>
auto List<Element, Allocator>::ptr() -> value_type * {
  return Base::data();
}

template <typename Element, typename Allocator>
auto List<Element, Allocator>::ptr() const -> value_type const * {
  return Base::data();
}

template <typename Element, typename Allocator>
auto List<Element, Allocator>::slice(SliceIndex a, SliceIndex b, int i) const -> List {
  return Star::slice(*this, a, b, i);
}

template <typename Element, typename Allocator>
template <typename Filter>
auto List<Element, Allocator>::filtered(Filter&& filter) const -> List {
  List list(*this);
  list.filter(std::forward<Filter>(filter));
  return list;
}

template <typename Element, typename Allocator>
template <typename Comparator>
auto List<Element, Allocator>::sorted(Comparator&& comparator) const -> List {
  List list(*this);
  list.sort(std::forward<Comparator>(comparator));
  return list;
}

template <typename Element, typename Allocator>
List<Element, Allocator> List<Element, Allocator>::sorted() const {
  List list(*this);
  list.sort();
  return list;
}

template <typename Element, typename Allocator>
template <typename Function>
auto List<Element, Allocator>::transformed(Function&& function) {
  List<typename std::decay<decltype(std::declval<Function>()(std::declval<reference>()))>::type> res;
  res.reserve(Base::size());
  transformInto(res, *this, std::forward<Function>(function));
  return res;
}

template <typename Element, typename Allocator>
template <typename Function>
auto List<Element, Allocator>::transformed(Function&& function) const {
  List<typename std::decay<decltype(std::declval<Function>()(std::declval<const_reference>()))>::type> res;
  res.reserve(Base::size());
  transformInto(res, *this, std::forward<Function>(function));
  return res;
}

template <typename Element, size_t MaxSize>
template <typename Container>
StaticList<Element, MaxSize> StaticList<Element, MaxSize>::from(Container const& c) {
  return StaticList(c.begin(), c.end());
}

template <typename Element, size_t MaxSize>
auto StaticList<Element, MaxSize>::slice(SliceIndex a, SliceIndex b, int i) const -> StaticList {
  return Star::slice(*this, a, b, i);
}

template <typename Element, size_t MaxSize>
template <typename Filter>
auto StaticList<Element, MaxSize>::filtered(Filter&& filter) const -> StaticList {
  StaticList list(*this);
  list.filter(forward<Filter>(filter));
  return list;
}

template <typename Element, size_t MaxSize>
template <typename Comparator>
auto StaticList<Element, MaxSize>::sorted(Comparator&& comparator) const -> StaticList {
  StaticList list(*this);
  list.sort(std::forward<Comparator>(comparator));
  return list;
}

template <typename Element, size_t MaxSize>
StaticList<Element, MaxSize> StaticList<Element, MaxSize>::sorted() const {
  StaticList list(*this);
  list.sort();
  return list;
}

template <typename Element, size_t MaxSize>
template <typename Function>
auto StaticList<Element, MaxSize>::transformed(Function&& function) {
  StaticList<typename std::decay<decltype(std::declval<Function>()(std::declval<reference>()))>::type, MaxSize> res;
  transformInto(res, *this, std::forward<Function>(function));
  return res;
}

template <typename Element, size_t MaxSize>
template <typename Function>
auto StaticList<Element, MaxSize>::transformed(Function&& function) const {
  StaticList<typename std::decay<decltype(std::declval<Function>()(std::declval<const_reference>()))>::type, MaxSize> res;
  transformInto(res, *this, std::forward<Function>(function));
  return res;
}

template <typename Element, size_t MaxStackSize>
template <typename Container>
SmallList<Element, MaxStackSize> SmallList<Element, MaxStackSize>::from(Container const& c) {
  return SmallList(c.begin(), c.end());
}

template <typename Element, size_t MaxStackSize>
auto SmallList<Element, MaxStackSize>::slice(SliceIndex a, SliceIndex b, int i) const -> SmallList {
  return Star::slice(*this, a, b, i);
}

template <typename Element, size_t MaxStackSize>
template <typename Filter>
auto SmallList<Element, MaxStackSize>::filtered(Filter&& filter) const -> SmallList {
  SmallList list(*this);
  list.filter(std::forward<Filter>(filter));
  return list;
}

template <typename Element, size_t MaxStackSize>
template <typename Comparator>
auto SmallList<Element, MaxStackSize>::sorted(Comparator&& comparator) const -> SmallList {
  SmallList list(*this);
  list.sort(std::forward<Comparator>(comparator));
  return list;
}

template <typename Element, size_t MaxStackSize>
SmallList<Element, MaxStackSize> SmallList<Element, MaxStackSize>::sorted() const {
  SmallList list(*this);
  list.sort();
  return list;
}

template <typename Element, size_t MaxStackSize>
template <typename Function>
auto SmallList<Element, MaxStackSize>::transformed(Function&& function) {
  SmallList<typename std::decay<decltype(std::declval<Function>()(std::declval<reference>()))>::type, MaxStackSize> res;
  transformInto(res, *this, std::forward<Function>(function));
  return res;
}

template <typename Element, size_t MaxStackSize>
template <typename Function>
auto SmallList<Element, MaxStackSize>::transformed(Function&& function) const {
  SmallList<typename std::decay<decltype(std::declval<Function>()(std::declval<const_reference>()))>::type, MaxStackSize> res;
  transformInto(res, *this, std::forward<Function>(function));
  return res;
}

template <typename Element, typename Allocator>
template <typename Container>
Deque<Element, Allocator> Deque<Element, Allocator>::from(Container const& c) {
  return Deque(c.begin(), c.end());
}

template <typename Element, typename Allocator>
Deque<Element, Allocator> Deque<Element, Allocator>::slice(SliceIndex a, SliceIndex b, int i) const {
  return Star::slice(*this, a, b, i);
}

template <typename Element, typename Allocator>
template <typename Filter>
Deque<Element, Allocator> Deque<Element, Allocator>::filtered(Filter&& filter) const {
  Deque l(*this);
  l.filter(std::forward<Filter>(filter));
  return l;
}

template <typename Element, typename Allocator>
template <typename Comparator>
Deque<Element, Allocator> Deque<Element, Allocator>::sorted(Comparator&& comparator) const {
  Deque l(*this);
  l.sort(std::forward<Comparator>(comparator));
  return l;
}

template <typename Element, typename Allocator>
Deque<Element, Allocator> Deque<Element, Allocator>::sorted() const {
  Deque l(*this);
  l.sort();
  return l;
}

template <typename Element, typename Allocator>
template <typename Function>
auto Deque<Element, Allocator>::transformed(Function&& function) {
  return Star::transform<Deque<decltype(std::declval<Function>()(std::declval<reference>()))>>(*this, std::forward<Function>(function));
}

template <typename Element, typename Allocator>
template <typename Function>
auto Deque<Element, Allocator>::transformed(Function&& function) const {
  return Star::transform<Deque<decltype(std::declval<Function>()(std::declval<const_reference>()))>>(*this, std::forward<Function>(function));
}

template <typename Element, typename Allocator>
template <typename Container>
LinkedList<Element, Allocator> LinkedList<Element, Allocator>::from(Container const& c) {
  return LinkedList(c.begin(), c.end());
}

template <typename Element, typename Allocator>
void LinkedList<Element, Allocator>::appendAll(LinkedList list) {
  Base::splice(Base::end(), list);
}

template <typename Element, typename Allocator>
void LinkedList<Element, Allocator>::prependAll(LinkedList list) {
  Base::splice(Base::begin(), list);
}

template <typename Element, typename Allocator>
template <typename Container>
void LinkedList<Element, Allocator>::appendAll(Container&& list) {
  for (auto& e : list) {
    if (std::is_rvalue_reference<Container&&>::value)
      Base::push_back(std::move(e));
    else
      Base::push_back(e);
  }
}

template <typename Element, typename Allocator>
template <typename Container>
void LinkedList<Element, Allocator>::prependAll(Container&& list) {
  for (auto i = std::rbegin(list); i != std::rend(list); ++i) {
    if (std::is_rvalue_reference<Container&&>::value)
      Base::push_front(std::move(*i));
    else
      Base::push_front(*i);
  }
}

template <typename Element, typename Allocator>
template <typename Filter>
LinkedList<Element, Allocator> LinkedList<Element, Allocator>::filtered(Filter&& filter) const {
  LinkedList list(*this);
  list.filter(std::forward<Filter>(filter));
  return list;
}

template <typename Element, typename Allocator>
template <typename Comparator>
LinkedList<Element, Allocator> LinkedList<Element, Allocator>::sorted(Comparator&& comparator) const {
  LinkedList l(*this);
  l.sort(std::forward<Comparator>(comparator));
  return l;
}

template <typename Element, typename Allocator>
LinkedList<Element, Allocator> LinkedList<Element, Allocator>::sorted() const {
  LinkedList l(*this);
  l.sort();
  return l;
}

template <typename Element, typename Allocator>
template <typename Function>
auto LinkedList<Element, Allocator>::transformed(Function&& function) {
  return Star::transform<LinkedList<decltype(std::declval<Function>()(std::declval<reference>()))>>(*this, std::forward<Function>(function));
}

template <typename Element, typename Allocator>
template <typename Function>
auto LinkedList<Element, Allocator>::transformed(Function&& function) const {
  return Star::transform<LinkedList<decltype(std::declval<Function>()(std::declval<const_reference>()))>>(*this, std::forward<Function>(function));
}

template <typename BaseList>
std::ostream& operator<<(std::ostream& os, ListMixin<BaseList> const& list) {
  os << "(";
  for (auto i = list.begin(); i != list.end(); ++i) {
    if (i != list.begin())
      os << ", ";
    os << *i;
  }
  os << ")";
  return os;
}

template <typename List>
size_t ListHasher<List>::operator()(List const& l) const {
  size_t h = 0;
  for (auto const& e : l)
    hashCombine(h, elemHasher(e));
  return h;
}

template <typename... Containers>
typename ListZipTypes<Containers...>::Result zip(Containers&&... args) {
  typename ListZipTypes<Containers...>::Result res;
  for (auto el : zipIterator(args...))
    res.push_back(std::move(el));

  return res;
}

template <typename Container>
typename ListEnumerateTypes<Container>::Result enumerate(Container&& container) {
  typename ListEnumerateTypes<Container>::Result res;
  for (auto el : enumerateIterator(container))
    res.push_back(std::move(el));

  return res;
}

}

template <typename BaseList>
struct fmt::formatter<Star::ListMixin<BaseList>> : ostream_formatter {};

#endif
