#pragma once

#include "StarException.hpp"
#include "StarHash.hpp"

namespace Star {

STAR_EXCEPTION(InvalidMaybeAccessException, StarException);

template <typename T>
class Maybe {
public:
  typedef T* PointerType;
  typedef T const* PointerConstType;
  typedef T& RefType;
  typedef T const& RefConstType;

  Maybe();

  Maybe(T const& t);
  Maybe(T&& t);

  Maybe(Maybe const& rhs);
  Maybe(Maybe&& rhs);
  template <typename T2>
  Maybe(Maybe<T2> const& rhs);

  ~Maybe();

  Maybe& operator=(Maybe const& rhs);
  Maybe& operator=(Maybe&& rhs);
  template <typename T2>
  Maybe& operator=(Maybe<T2> const& rhs);

  bool isValid() const;
  bool isNothing() const;
  explicit operator bool() const;

  PointerConstType ptr() const;
  PointerType ptr();

  PointerConstType operator->() const;
  PointerType operator->();

  RefConstType operator*() const;
  RefType operator*();

  bool operator==(Maybe const& rhs) const;
  bool operator!=(Maybe const& rhs) const;
  bool operator<(Maybe const& rhs) const;

  RefConstType get() const;
  RefType get();

  // Get either the contents of this Maybe or the given default.
  T value(T def = T()) const;

  // Get either this value, or if this value is none the given value.
  Maybe orMaybe(Maybe const& other) const;

  // Takes the value out of this Maybe, leaving it Nothing.
  T take();

  // If this Maybe is set, assigns it to t and leaves this Maybe as Nothing.
  bool put(T& t);

  void set(T const& t);
  void set(T&& t);

  template <typename... Args>
  void emplace(Args&&... t);

  void reset();

  // Apply a function to the contained value if it is not Nothing.
  template <typename Function>
  void exec(Function&& function);

  // Functor map operator.  If this maybe is not Nothing, then applies the
  // given function to it and returns the result, otherwise returns Nothing (of
  // the type the function would normally return).
  template <typename Function>
  auto apply(Function&& function) const -> Maybe<typename std::decay<decltype(function(std::declval<T>()))>::type>;

  // Monadic bind operator.  Given function should return another Maybe.
  template <typename Function>
  auto sequence(Function function) const -> decltype(function(std::declval<T>()));

private:
  union {
    T m_data;
  };

  bool m_initialized;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, Maybe<T> const& v);

template <typename T>
struct hash<Maybe<T>> {
  size_t operator()(Maybe<T> const& m) const;
  hash<T> hasher;
};

template <typename T>
Maybe<T>::Maybe()
  : m_initialized(false) {}

template <typename T>
Maybe<T>::Maybe(T const& t)
  : Maybe() {
  new (&m_data) T(t);
  m_initialized = true;
}

template <typename T>
Maybe<T>::Maybe(T&& t)
  : Maybe() {
  new (&m_data) T(std::forward<T>(t));
  m_initialized = true;
}

template <typename T>
Maybe<T>::Maybe(Maybe const& rhs)
  : Maybe() {
  if (rhs.m_initialized) {
    new (&m_data) T(rhs.m_data);
    m_initialized = true;
  }
}

template <typename T>
Maybe<T>::Maybe(Maybe&& rhs)
  : Maybe() {
  if (rhs.m_initialized) {
    new (&m_data) T(std::move(rhs.m_data));
    m_initialized = true;
    rhs.reset();
  }
}

template <typename T>
template <typename T2>
Maybe<T>::Maybe(Maybe<T2> const& rhs)
  : Maybe() {
  if (rhs) {
    new (&m_data) T(*rhs);
    m_initialized = true;
  }
}

template <typename T>
Maybe<T>::~Maybe() {
  reset();
}

template <typename T>
Maybe<T>& Maybe<T>::operator=(Maybe const& rhs) {
  if (&rhs == this)
    return *this;

  if (rhs)
    emplace(*rhs);
  else
    reset();

  return *this;
}

template <typename T>
template <typename T2>
Maybe<T>& Maybe<T>::operator=(Maybe<T2> const& rhs) {
  if (rhs)
    emplace(*rhs);
  else
    reset();

  return *this;
}

template <typename T>
Maybe<T>& Maybe<T>::operator=(Maybe&& rhs) {
  if (&rhs == this)
    return *this;

  if (rhs)
    emplace(rhs.take());
  else
    reset();

  return *this;
}

template <typename T>
bool Maybe<T>::isValid() const {
  return m_initialized;
}

template <typename T>
bool Maybe<T>::isNothing() const {
  return !m_initialized;
}

template <typename T>
Maybe<T>::operator bool() const {
  return m_initialized;
}

template <typename T>
auto Maybe<T>::ptr() const -> PointerConstType {
  if (m_initialized)
    return &m_data;
  return nullptr;
}

template <typename T>
auto Maybe<T>::ptr() -> PointerType {
  if (m_initialized)
    return &m_data;
  return nullptr;
}

template <typename T>
auto Maybe<T>::operator-> () const -> PointerConstType {
  if (!m_initialized)
    throw InvalidMaybeAccessException();

  return &m_data;
}

template <typename T>
auto Maybe<T>::operator->() -> PointerType {
  if (!m_initialized)
    throw InvalidMaybeAccessException();

  return &m_data;
}

template <typename T>
auto Maybe<T>::operator*() const -> RefConstType {
  return get();
}

template <typename T>
auto Maybe<T>::operator*() -> RefType {
  return get();
}

template <typename T>
bool Maybe<T>::operator==(Maybe const& rhs) const {
  if (!m_initialized && !rhs.m_initialized)
    return true;
  if (m_initialized && rhs.m_initialized)
    return get() == rhs.get();
  return false;
}

template <typename T>
bool Maybe<T>::operator!=(Maybe const& rhs) const {
  return !operator==(rhs);
}

template <typename T>
bool Maybe<T>::operator<(Maybe const& rhs) const {
  if (m_initialized && rhs.m_initialized)
    return get() < rhs.get();
  if (!m_initialized && rhs.m_initialized)
    return true;
  return false;
}

template <typename T>
auto Maybe<T>::get() const -> RefConstType {
  if (!m_initialized)
    throw InvalidMaybeAccessException();

  return m_data;
}

template <typename T>
auto Maybe<T>::get() -> RefType {
  if (!m_initialized)
    throw InvalidMaybeAccessException();

  return m_data;
}

template <typename T>
T Maybe<T>::value(T def) const {
  if (m_initialized)
    return m_data;
  else
    return def;
}

template <typename T>
Maybe<T> Maybe<T>::orMaybe(Maybe const& other) const {
  if (m_initialized)
    return *this;
  else
    return other;
}

template <typename T>
T Maybe<T>::take() {
  if (!m_initialized)
    throw InvalidMaybeAccessException();

  T val(std::move(m_data));

  reset();

  return val;
}

template <typename T>
bool Maybe<T>::put(T& t) {
  if (m_initialized) {
    t = std::move(m_data);

    reset();

    return true;
  } else {
    return false;
  }
}

template <typename T>
void Maybe<T>::set(T const& t) {
  emplace(t);
}

template <typename T>
void Maybe<T>::set(T&& t) {
  emplace(std::forward<T>(t));
}

template <typename T>
template <typename... Args>
void Maybe<T>::emplace(Args&&... t) {
  reset();

  new (&m_data) T(std::forward<Args>(t)...);
  m_initialized = true;
}

template <typename T>
void Maybe<T>::reset() {
  if (m_initialized) {
    m_initialized = false;
    m_data.~T();
  }
}

template <typename T>
template <typename Function>
auto Maybe<T>::apply(Function&& function) const
    -> Maybe<typename std::decay<decltype(function(std::declval<T>()))>::type> {
  if (!isValid())
    return {};
  return function(get());
}

template <typename T>
template <typename Function>
void Maybe<T>::exec(Function&& function) {
  if (isValid())
    function(get());
}

template <typename T>
template <typename Function>
auto Maybe<T>::sequence(Function function) const -> decltype(function(std::declval<T>())) {
  if (!isValid())
    return {};
  return function(get());
}

template <typename T>
std::ostream& operator<<(std::ostream& os, Maybe<T> const& v) {
  if (v)
    return os << "Just (" << *v << ")";
  else
    return os << "Nothing";
}

template <typename T>
size_t hash<Maybe<T>>::operator()(Maybe<T> const& m) const {
  if (!m)
    return 0;
  else
    return hasher(*m);
}

}

template <typename T>
struct fmt::formatter<Star::Maybe<T>> : ostream_formatter {};
