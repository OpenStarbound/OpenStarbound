#ifndef STAR_REF_PTR_HPP
#define STAR_REF_PTR_HPP

#include "StarException.hpp"
#include "StarHash.hpp"

namespace Star {

// Reference counted ptr for intrusive reference counted types.  Calls
// unqualified refPtrIncRef and refPtrDecRef functions to manage the reference
// count.
template <typename T>
class RefPtr {
public:
  typedef T element_type;

  RefPtr();
  explicit RefPtr(T* p, bool addRef = true);

  RefPtr(RefPtr const& r);
  RefPtr(RefPtr&& r);

  template <typename T2>
  RefPtr(RefPtr<T2> const& r);
  template <typename T2>
  RefPtr(RefPtr<T2>&& r);

  ~RefPtr();

  RefPtr& operator=(RefPtr const& r);
  RefPtr& operator=(RefPtr&& r);

  template <typename T2>
  RefPtr& operator=(RefPtr<T2> const& r);
  template <typename T2>
  RefPtr& operator=(RefPtr<T2>&& r);

  void reset();

  void reset(T* r, bool addRef = true);

  T& operator*() const;
  T* operator->() const;
  T* get() const;

  explicit operator bool() const;

private:
  template <typename T2>
  friend class RefPtr;

  T* m_ptr;
};

template <typename T, typename U>
bool operator==(RefPtr<T> const& a, RefPtr<U> const& b);

template <typename T, typename U>
bool operator!=(RefPtr<T> const& a, RefPtr<U> const& b);

template <typename T>
bool operator==(RefPtr<T> const& a, T* b);

template <typename T>
bool operator!=(RefPtr<T> const& a, T* b);

template <typename T>
bool operator==(T* a, RefPtr<T> const& b);

template <typename T>
bool operator!=(T* a, RefPtr<T> const& b);

template <typename T, typename U>
bool operator<(RefPtr<T> const& a, RefPtr<U> const& b);

template <typename Type1, typename Type2>
bool is(RefPtr<Type2> const& p);

template <typename Type1, typename Type2>
bool is(RefPtr<Type2 const> const& p);

template <typename Type1, typename Type2>
RefPtr<Type1> as(RefPtr<Type2> const& p);

template <typename Type1, typename Type2>
RefPtr<Type1 const> as(RefPtr<Type2 const> const& p);

template <typename T, typename... Args>
RefPtr<T> make_ref(Args&&... args);

template <typename T>
struct hash<RefPtr<T>> {
  size_t operator()(RefPtr<T> const& a) const;

  hash<T*> hasher;
};

// Base class for RefPtr that is NOT thread safe.  This can have a performance
// benefit over shared_ptr in single threaded contexts.
class RefCounter {
public:
  friend void refPtrIncRef(RefCounter* p);
  friend void refPtrDecRef(RefCounter* p);

protected:
  RefCounter();
  virtual ~RefCounter() = default;

private:
  size_t m_refCounter;
};

template <typename T>
RefPtr<T>::RefPtr()
  : m_ptr(nullptr) {}

template <typename T>
RefPtr<T>::RefPtr(T* p, bool addRef)
  : m_ptr(nullptr) {
  reset(p, addRef);
}

template <typename T>
RefPtr<T>::RefPtr(RefPtr const& r)
  : RefPtr(r.m_ptr) {}

template <typename T>
RefPtr<T>::RefPtr(RefPtr&& r) {
  m_ptr = r.m_ptr;
  r.m_ptr = nullptr;
}

template <typename T>
template <typename T2>
RefPtr<T>::RefPtr(RefPtr<T2> const& r)
  : RefPtr(r.m_ptr) {}

template <typename T>
template <typename T2>
RefPtr<T>::RefPtr(RefPtr<T2>&& r) {
  m_ptr = r.m_ptr;
  r.m_ptr = nullptr;
}

template <typename T>
RefPtr<T>::~RefPtr() {
  if (m_ptr)
    refPtrDecRef(m_ptr);
}

template <typename T>
RefPtr<T>& RefPtr<T>::operator=(RefPtr const& r) {
  reset(r.m_ptr);
  return *this;
}

template <typename T>
RefPtr<T>& RefPtr<T>::operator=(RefPtr&& r) {
  if (m_ptr)
    refPtrDecRef(m_ptr);

  m_ptr = r.m_ptr;
  r.m_ptr = nullptr;
  return *this;
}

template <typename T>
template <typename T2>
RefPtr<T>& RefPtr<T>::operator=(RefPtr<T2> const& r) {
  reset(r.m_ptr);
  return *this;
}

template <typename T>
template <typename T2>
RefPtr<T>& RefPtr<T>::operator=(RefPtr<T2>&& r) {
  if (m_ptr)
    refPtrDecRef(m_ptr);

  m_ptr = r.m_ptr;
  r.m_ptr = nullptr;
  return *this;
}

template <typename T>
void RefPtr<T>::reset() {
  reset(nullptr);
}

template <typename T>
void RefPtr<T>::reset(T* r, bool addRef) {
  if (m_ptr == r)
    return;

  if (m_ptr)
    refPtrDecRef(m_ptr);

  m_ptr = r;

  if (m_ptr && addRef)
    refPtrIncRef(m_ptr);
}

template <typename T>
T& RefPtr<T>::operator*() const {
  return *m_ptr;
}

template <typename T>
T* RefPtr<T>::operator->() const {
  return m_ptr;
}

template <typename T>
T* RefPtr<T>::get() const {
  return m_ptr;
}

template <typename T>
RefPtr<T>::operator bool() const {
  return m_ptr != nullptr;
}

template <typename T, typename U>
bool operator==(RefPtr<T> const& a, RefPtr<U> const& b) {
  return a.get() == b.get();
}

template <typename T, typename U>
bool operator!=(RefPtr<T> const& a, RefPtr<U> const& b) {
  return a.get() != b.get();
}

template <typename T>
bool operator==(RefPtr<T> const& a, T* b) {
  return a.get() == b;
}

template <typename T>
bool operator!=(RefPtr<T> const& a, T* b) {
  return a.get() != b;
}

template <typename T>
bool operator==(T* a, RefPtr<T> const& b) {
  return a == b.get();
}

template <typename T>
bool operator!=(T* a, RefPtr<T> const& b) {
  return a != b.get();
}

template <typename T, typename U>
bool operator<(RefPtr<T> const& a, RefPtr<U> const& b) {
  return a.get() < b.get();
}

template <typename Type1, typename Type2>
bool is(RefPtr<Type2> const& p) {
  return (bool)dynamic_cast<Type1*>(p.get());
}

template <typename Type1, typename Type2>
bool is(RefPtr<Type2 const> const& p) {
  return (bool)dynamic_cast<Type1 const*>(p.get());
}

template <typename Type1, typename Type2>
RefPtr<Type1> as(RefPtr<Type2> const& p) {
  return RefPtr<Type1>(dynamic_cast<Type1*>(p.get()));
}

template <typename Type1, typename Type2>
RefPtr<Type1 const> as(RefPtr<Type2 const> const& p) {
  return RefPtr<Type1>(dynamic_cast<Type1 const*>(p.get()));
}

template <typename T, typename... Args>
RefPtr<T> make_ref(Args&&... args) {
  return RefPtr<T>(new T(forward<Args>(args)...));
}

template <typename T>
size_t hash<RefPtr<T>>::operator()(RefPtr<T> const& a) const {
  return hasher(a.get());
}

inline void refPtrIncRef(RefCounter* p) {
  ++p->m_refCounter;
}

inline void refPtrDecRef(RefCounter* p) {
  if (--p->m_refCounter == 0)
    delete p;
}

inline RefCounter::RefCounter()
  : m_refCounter(0) {}

}

#endif
