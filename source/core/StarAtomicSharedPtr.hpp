#ifndef STAR_ATOMIC_SHARED_PTR_HPP
#define STAR_ATOMIC_SHARED_PTR_HPP

#include "StarThread.hpp"

namespace Star {

// Thread safe shared_ptr such that is is possible to safely access the
// contents of the shared_ptr while other threads might be updating it.  Makes
// it possible to safely do Read Copy Update.
template <typename T>
class AtomicSharedPtr {
public:
  typedef shared_ptr<T> SharedPtr;
  typedef weak_ptr<T> WeakPtr;

  AtomicSharedPtr();
  AtomicSharedPtr(AtomicSharedPtr const& p);
  AtomicSharedPtr(AtomicSharedPtr&& p);
  AtomicSharedPtr(SharedPtr p);

  SharedPtr load() const;
  WeakPtr weak() const;
  void store(SharedPtr p);
  void reset();

  explicit operator bool() const;
  bool unique() const;

  SharedPtr operator->() const;

  AtomicSharedPtr& operator=(AtomicSharedPtr const& p);
  AtomicSharedPtr& operator=(AtomicSharedPtr&& p);
  AtomicSharedPtr& operator=(SharedPtr p);

private:
  SharedPtr m_ptr;
  mutable SpinLock m_lock;
};

template <typename T>
AtomicSharedPtr<T>::AtomicSharedPtr() {}

template <typename T>
AtomicSharedPtr<T>::AtomicSharedPtr(AtomicSharedPtr const& p)
  : m_ptr(p.load()) {}

template <typename T>
AtomicSharedPtr<T>::AtomicSharedPtr(AtomicSharedPtr&& p)
  : m_ptr(move(p.m_ptr)) {}

template <typename T>
AtomicSharedPtr<T>::AtomicSharedPtr(SharedPtr p)
  : m_ptr(move(p)) {}

template <typename T>
auto AtomicSharedPtr<T>::load() const -> SharedPtr {
  SpinLocker locker(m_lock);
  return m_ptr;
}

template <typename T>
auto AtomicSharedPtr<T>::weak() const -> WeakPtr {
  SpinLocker locker(m_lock);
  return WeakPtr(m_ptr);
}

template <typename T>
void AtomicSharedPtr<T>::store(SharedPtr p) {
  SpinLocker locker(m_lock);
  m_ptr = move(p);
}

template <typename T>
void AtomicSharedPtr<T>::reset() {
  SpinLocker locker(m_lock);
  m_ptr.reset();
}

template <typename T>
AtomicSharedPtr<T>::operator bool() const {
  SpinLocker locker(m_lock);
  return (bool)m_ptr;
}

template <typename T>
bool AtomicSharedPtr<T>::unique() const {
  SpinLocker locker(m_lock);
  return m_ptr.unique();
}

template <typename T>
auto AtomicSharedPtr<T>::operator-> () const -> SharedPtr {
  SpinLocker locker(m_lock);
  return m_ptr;
}

template <typename T>
AtomicSharedPtr<T>& AtomicSharedPtr<T>::operator=(AtomicSharedPtr const& p) {
  SpinLocker locker(m_lock);
  m_ptr = p.load();
  return *this;
}

template <typename T>
AtomicSharedPtr<T>& AtomicSharedPtr<T>::operator=(AtomicSharedPtr&& p) {
  SpinLocker locker(m_lock);
  m_ptr = move(p.m_ptr);
  return *this;
}

template <typename T>
AtomicSharedPtr<T>& AtomicSharedPtr<T>::operator=(SharedPtr p) {
  SpinLocker locker(m_lock);
  m_ptr = move(p);
  return *this;
}

}

#endif
