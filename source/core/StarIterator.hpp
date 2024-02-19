#ifndef STAR_ITERATOR_H
#define STAR_ITERATOR_H

#include <algorithm>

#include "StarException.hpp"

namespace Star {

STAR_EXCEPTION(IteratorException, StarException);

// Provides java style iterators for bidirectional list-like containers
// (SIterator and SMutableIterator) and forward only map-like containers
// (SMapIterator and SMutableMapIterator)
template <typename Container>
class SIterator {
public:
  typedef typename Container::const_iterator iterator;
  typedef decltype(*iterator()) value_ref;

  SIterator(Container const& c) : cont(c) {
    toFront();
  }

  void toFront() {
    curr = cont.begin();
    direction = 0;
  }

  void toBack() {
    curr = cont.end();
    direction = 0;
  }

  bool hasNext() const {
    return curr != cont.end();
  }

  bool hasPrevious() const {
    return curr != cont.begin();
  }

  value_ref value() const {
    if (direction == 1) {
      if (curr != cont.end() && cont.size() != 0)
        return *curr;
      else
        throw IteratorException("value() called on end()");
    } else if (direction == -1) {
      if (curr != cont.begin() && cont.size() != 0) {
        iterator back = curr;
        return *(--back);
      } else {
        throw IteratorException("value() called on begin()");
      }
    } else {
      throw IteratorException("value() called without previous next() or previous()");
    }
  }

  value_ref next() {
    if (hasNext()) {
      direction = -1;
      return *(curr++);
    }
    throw IteratorException("next() called on end");
  }

  value_ref previous() {
    if (hasPrevious()) {
      direction = 1;
      return *(--curr);
    }
    throw IteratorException("prev() called on beginning");
  }

  value_ref peekNext() const {
    SIterator t = *this;
    return t.next();
  }

  value_ref peekPrevious() const {
    SIterator t = *this;
    return t.previous();
  }

  size_t distFront() const {
    return std::distance(cont.begin(), curr);
  }

  size_t distBack() const {
    return std::distance(curr, cont.end());
  }

private:
  SIterator& operator=(iterator const& i) {
    return iterator::operator=(i);
  }
  Container const& cont;
  iterator curr;

  int direction;
};

template <typename Container>
SIterator<Container> makeSIterator(Container const& c) {
  return SIterator<Container>(c);
}

template <typename Container>
class SMutableIterator {
public:
  typedef typename Container::value_type value_type;
  typedef typename Container::iterator iterator;
  typedef decltype(*iterator()) value_ref;

  SMutableIterator(Container& c) : cont(c) {
    toFront();
  }

  void toFront() {
    curr = cont.begin();
    direction = 0;
  }

  void toBack() {
    curr = cont.end();
    direction = 0;
  }

  bool hasNext() const {
    return curr != cont.end();
  }

  bool hasPrevious() const {
    return curr != cont.begin();
  }

  void insert(value_type v) {
    curr = ++cont.insert(curr, std::move(v));
    direction = -1;
  }

  void remove() {
    if (direction == 1) {
      direction = 0;
      if (curr != cont.end() && cont.size() != 0)
        curr = cont.erase(curr);
      else
        throw IteratorException("remove() called on end()");
    } else if (direction == -1) {
      direction = 0;
      if (curr != cont.begin() && cont.size() != 0)
        curr = cont.erase(--curr);
      else
        throw IteratorException("remove() called on begin()");
    } else {
      throw IteratorException("remove() called without previous next() or previous()");
    }
  }

  value_ref value() const {
    if (direction == 1) {
      if (curr != cont.end() && cont.size() != 0)
        return *curr;
      else
        throw IteratorException("value() called on end()");
    } else if (direction == -1) {
      if (curr != cont.begin() && cont.size() != 0) {
        iterator back = curr;
        return *(--back);
      } else {
        throw IteratorException("value() called on begin()");
      }
    } else {
      throw IteratorException("value() called without previous next() or previous()");
    }
  }

  void setValue(value_type v) const {
    value() = std::move(v);
  }

  value_ref next() {
    if (curr == cont.end())
      throw IteratorException("next() called on end");
    direction = -1;
    return *curr++;
  }

  value_ref previous() {
    if (curr == cont.begin())
      throw IteratorException("previous() called on begin");
    direction = 1;
    return *--curr;
  }

  value_ref peekNext() const {
    SMutableIterator n = *this;
    return n.next();
  }

  value_ref peekPrevious() const {
    SMutableIterator n = *this;
    return n.previous();
  }

  size_t distFront() const {
    return std::distance(cont.begin(), curr);
  }

  size_t distBack() const {
    return std::distance(curr, cont.end());
  }

private:
  SMutableIterator& operator=(iterator const& i) {
    return iterator::operator=(i);
  }

  Container& cont;
  iterator curr;

  // -1 means remove() will remove --cur, +1 means ++cur, 0 means remove() is
  // invalid.
  int direction;
};

template <typename Container>
SMutableIterator<Container> makeSMutableIterator(Container& c) {
  return SMutableIterator<Container>(c);
}

template <typename Container>
class SMapIterator {
public:
  typedef typename Container::key_type key_type;
  typedef typename Container::mapped_type mapped_type;

  typedef typename Container::const_iterator iterator;
  typedef decltype(*iterator()) value_ref;

  SMapIterator(Container const& c) : cont(c) {
    toFront();
  }

  void toFront() {
    curr = cont.end();
  }

  void toBack() {
    curr = cont.end();
    if (curr != cont.begin())
      --curr;
  }

  bool hasNext() const {
    iterator end = cont.end();
    if (curr == end)
      return cont.begin() != end;
    else
      return ++iterator(curr) != end;
  }

  key_type const& key() const {
    if (curr != cont.end()) {
      return curr->first;
    } else {
      throw IteratorException("key() called on begin()");
    }
  }

  mapped_type const& value() const {
    if (curr != cont.end()) {
      return curr->second;
    } else {
      throw IteratorException("value() called on begin()");
    }
  }

  value_ref const& next() {
    if (hasNext()) {
      if (curr == cont.end())
        curr = cont.begin();
      else
        ++curr;
      return *curr;
    }
    throw IteratorException("next() called on end");
  }

  value_ref peekNext() const {
    SMapIterator t = *this;
    return t.next();
  }

  size_t distFront() const {
    return std::distance(cont.begin(), curr);
  }

  size_t distBack() const {
    return std::distance(curr, cont.end()) - 1;
  }

protected:
  SMapIterator& operator=(iterator const& i) {
    return iterator::operator=(i);
  }
  Container const& cont;
  iterator curr;
};

template <typename Container>
SMapIterator<Container> makeSMapIterator(Container const& c) {
  return SMapIterator<Container>(c);
}

template <typename Container>
class SMutableMapIterator {
public:
  typedef typename Container::key_type key_type;
  typedef typename Container::mapped_type mapped_type;

  typedef typename Container::iterator iterator;
  typedef decltype(*iterator()) value_ref;

  SMutableMapIterator(Container& c) : cont(c) {
    toFront();
  }

  void toFront() {
    curr = cont.end();
    remCalled = false;
  }

  void toBack() {
    curr = cont.end();
    if (curr != cont.begin())
      --curr;
  }

  bool hasNext() const {
    iterator end = cont.end();
    if (curr == end)
      return cont.begin() != end && !remCalled;
    else if (remCalled)
      return curr != end;
    else
      return ++iterator(curr) != end;
  }

  key_type const& key() const {
    if (remCalled)
      throw IteratorException("key() called after remove()");
    else if (curr != cont.end())
      return curr->first;
    else
      throw IteratorException("key() called on begin()");
  }

  mapped_type& value() const {
    if (remCalled)
      throw IteratorException("value() called after remove()");
    else if (curr != cont.end())
      return curr->second;
    else
      throw IteratorException("value() called on begin()");
  }

  value_ref next() {
    if (hasNext()) {
      if (curr == cont.end())
        curr = cont.begin();
      else if (remCalled)
        remCalled = false;
      else
        ++curr;

      return *curr;
    } else {
      throw IteratorException("next() called on end");
    }
  }

  value_ref peekNext() const {
    SMutableMapIterator t = *this;
    return t.next();
  }

  void remove() {
    if (remCalled) {
      throw IteratorException("remove() called twice");
    } else if (curr == cont.end()) {
      throw IteratorException("remove() called at front");
    } else {
      if (curr == cont.begin()) {
        cont.erase(curr);
        curr = cont.end();
      } else {
        curr = cont.erase(curr);
        remCalled = true;
      }
    }
  }

  size_t distFront() const {
    if (curr == cont.end())
      return 0;
    else
      return std::distance(cont.begin(), curr) - (remCalled ? 1 : 0);
  }

  size_t distBack() const {
    if (curr == cont.end())
      return cont.size();
    else
      return std::distance(curr, cont.end()) - 1 + (remCalled ? 1 : 0);
  }

private:
  SMutableMapIterator& operator=(iterator const& i) {
    return iterator::operator=(i);
  }

  Container& cont;
  iterator curr;
  bool remCalled;
};

template <typename Container>
SMutableMapIterator<Container> makeSMutableMapIterator(Container& c) {
  return SMutableMapIterator<Container>(c);
}

}

#endif
