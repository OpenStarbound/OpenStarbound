#pragma once

#include "StarException.hpp"
#include "StarString.hpp"

namespace Star {

STAR_STRUCT(ThreadImpl);
STAR_STRUCT(ThreadFunctionImpl);
STAR_STRUCT(MutexImpl);
STAR_STRUCT(ConditionVariableImpl);
STAR_STRUCT(RecursiveMutexImpl);

template <typename Return>
class ThreadFunction;

class Thread {
public:
  // Implementations of this method should sleep for at least the given amount
  // of time, but may sleep for longer due to scheduling.
  static void sleep(unsigned millis);

  // Sleep a more precise amount of time, but uses more resources to do so.
  // Should be less likely to sleep much longer than the given amount of time.
  static void sleepPrecise(unsigned millis);

  // Yield this thread, offering the opportunity to reschedule.
  static void yield();

  static unsigned numberOfProcessors();

  template <typename Function, typename... Args>
  static ThreadFunction<decltype(std::declval<Function>()(std::declval<Args>()...))> invoke(String const& name, Function&& f, Args&&... args);

  Thread(String const& name);
  Thread(Thread&&);
  // Will not automatically join!  ALL implementations of this class MUST call
  // join() in their most derived constructors, or not rely on the destructor
  // joining.
  virtual ~Thread();

  Thread& operator=(Thread&&);

  // Start a thread that is currently in the joined state.  Returns true if the
  // thread was joined and is now started, false if the thread was not joined.
  bool start();

  // Wait for a thread to finish and re-join with the thread, on completion
  // isJoined() will be false.  Returns true if the thread was joinable, and is
  // now joined, false if the thread was already joined.
  bool join();

  // Returns false when this thread been started without being joined.  This is
  // subtlely different than "!isRunning()", in that the thread could have
  // completed its work, but a thread *must* be joined before being restarted.
  bool isJoined() const;

  // Returns false before start() has been called, true immediately after
  // start() has been called, and false once the run() method returns.
  bool isRunning() const;

  String name();

protected:
  virtual void run() = 0;

private:
  unique_ptr<ThreadImpl> m_impl;
};

// Wraps a function call and calls in another thread, very nice lightweight
// one-shot alternative to deriving from Thread.  Handles exceptions in a
// different way from Thread, instead of logging the exception, the exception
// is forwarded and re-thrown during the call to finish().
template <>
class ThreadFunction<void> {
public:
  ThreadFunction();
  ThreadFunction(ThreadFunction&&);

  // Automatically starts the given function, ThreadFunction can also be
  // constructed with Thread::invoke, which is a shorthand.
  ThreadFunction(function<void()> function, String const& name);

  // Automatically calls finish, though BEWARE that often times this is quite
  // dangerous, and this is here mostly as a fallback.  The natural destructor
  // order for members of a class is often wrong, and if the function throws,
  // since this destructor calls finish it will throw.
  ~ThreadFunction();

  ThreadFunction& operator=(ThreadFunction&&);

  // Waits on function finish if function is assigned and started, otherwise
  // does nothing.  If the function threw an exception, it will be re-thrown
  // here (on the first call to finish() only).
  void finish();

  // Returns whether the ThreadFunction::finish method been called and the
  // ThreadFunction has stopped.  Also returns true when the ThreadFunction has
  // been default constructed.
  bool isFinished() const;
  // Returns false if the thread function has stopped running, whether or not
  // finish() has been called.
  bool isRunning() const;

  // Equivalent to !isFinished()
  explicit operator bool() const;

  String name();

private:
  unique_ptr<ThreadFunctionImpl> m_impl;
};

template <typename Return>
class ThreadFunction {
public:
  ThreadFunction();
  ThreadFunction(ThreadFunction&&);
  ThreadFunction(function<Return()> function, String const& name);

  ~ThreadFunction();

  ThreadFunction& operator=(ThreadFunction&&);

  // Finishes the thread, moving and returning the final value of the function.
  // If the function threw an exception, finish() will rethrow that exception.
  // May only be called once, otherwise will throw InvalidMaybeAccessException.
  Return finish();

  bool isFinished() const;
  bool isRunning() const;

  explicit operator bool() const;

  String name();

private:
  ThreadFunction<void> m_function;
  shared_ptr<Maybe<Return>> m_return;
};

// *Non* recursive mutex lock, for use with ConditionVariable
class Mutex {
public:
  Mutex();
  Mutex(Mutex&&);
  ~Mutex();

  Mutex& operator=(Mutex&&);

  void lock();

  // Attempt to acquire the mutex without blocking.
  bool tryLock();

  void unlock();

private:
  friend struct ConditionVariableImpl;
  unique_ptr<MutexImpl> m_impl;
};

class ConditionVariable {
public:
  ConditionVariable();
  ConditionVariable(ConditionVariable&&);
  ~ConditionVariable();

  ConditionVariable& operator=(ConditionVariable&&);

  // Atomically unlocks the mutex argument and waits on the condition.  On
  // acquiring the condition, atomically returns and re-locks the mutex.  Must
  // lock the mutex before calling.  If millis is given, waits for a maximum of
  // the given milliseconds only.
  void wait(Mutex& mutex, Maybe<unsigned> millis = {});

  // Wake one waiting thread.  The calling thread for is allowed to either hold
  // or not hold the mutex that the threads waiting on the condition are using,
  // both will work and result in slightly different scheduling.
  void signal();

  // Wake all threads, policy for holding the mutex is the same for signal().
  void broadcast();

private:
  unique_ptr<ConditionVariableImpl> m_impl;
};

// Recursive mutex lock.  lock() may be called many times freely by the same
// thread, but unlock() must be called an equal number of times to unlock it.
class RecursiveMutex {
public:
  RecursiveMutex();
  RecursiveMutex(RecursiveMutex&&);
  ~RecursiveMutex();

  RecursiveMutex& operator=(RecursiveMutex&&);

  void lock();

  // Attempt to acquire the mutex without blocking.
  bool tryLock();

  void unlock();

private:
  unique_ptr<RecursiveMutexImpl> m_impl;
};

// RAII for mutexes.  Locking and unlocking are always safe, MLocker will never
// attempt to lock the held mutex more than once, or unlock more than once, and
// destruction will always unlock the mutex *iff* it is actually locked.
// (Locked here refers to one specific MLocker *itself* locking the mutex, not
// whether the mutex is locked *at all*, so it is sensible to use with
// RecursiveMutex)
template <typename MutexType>
class MLocker {
public:
  // Pass false to lock to start unlocked
  MLocker(MutexType& ref, bool lock = true);
  ~MLocker();

  MLocker(MLocker const&) = delete;
  MLocker& operator=(MLocker const&) = delete;

  MutexType& mutex();

  void unlock();
  void lock();
  bool tryLock();

private:
  MutexType& m_mutex;
  bool m_locked;
};
typedef MLocker<Mutex> MutexLocker;
typedef MLocker<RecursiveMutex> RecursiveMutexLocker;

class ReadersWriterMutex {
public:
  ReadersWriterMutex();

  void readLock();
  bool tryReadLock();
  void readUnlock();

  void writeLock();
  bool tryWriteLock();
  void writeUnlock();

private:
  Mutex m_mutex;
  ConditionVariable m_readCond;
  ConditionVariable m_writeCond;
  unsigned m_readers;
  unsigned m_writers;
  unsigned m_readWaiters;
  unsigned m_writeWaiters;
};

class ReadLocker {
public:
  ReadLocker(ReadersWriterMutex& rwlock, bool startLocked = true);
  ~ReadLocker();

  ReadLocker(ReadLocker const&) = delete;
  ReadLocker& operator=(ReadLocker const&) = delete;

  void unlock();
  void lock();
  bool tryLock();

private:
  ReadersWriterMutex& m_lock;
  bool m_locked;
};

class WriteLocker {
public:
  WriteLocker(ReadersWriterMutex& rwlock, bool startLocked = true);
  ~WriteLocker();

  WriteLocker(WriteLocker const&) = delete;
  WriteLocker& operator=(WriteLocker const&) = delete;

  void unlock();
  void lock();
  bool tryLock();

private:
  ReadersWriterMutex& m_lock;
  bool m_locked;
};

class SpinLock {
public:
  SpinLock();

  void lock();
  bool tryLock();
  void unlock();

private:
  atomic_flag m_lock;
};
typedef MLocker<SpinLock> SpinLocker;

template <typename MutexType>
MLocker<MutexType>::MLocker(MutexType& ref, bool l)
  : m_mutex(ref), m_locked(false) {
  if (l)
    lock();
}

template <typename MutexType>
MLocker<MutexType>::~MLocker() {
  unlock();
}

template <typename MutexType>
MutexType& MLocker<MutexType>::mutex() {
  return m_mutex;
}

template <typename MutexType>
void MLocker<MutexType>::unlock() {
  if (m_locked) {
    m_mutex.unlock();
    m_locked = false;
  }
}

template <typename MutexType>
void MLocker<MutexType>::lock() {
  if (!m_locked) {
    m_mutex.lock();
    m_locked = true;
  }
}

template <typename MutexType>
bool MLocker<MutexType>::tryLock() {
  if (!m_locked) {
    if (m_mutex.tryLock())
      m_locked = true;
  }

  return m_locked;
}

template <typename Function, typename... Args>
ThreadFunction<decltype(std::declval<Function>()(std::declval<Args>()...))> Thread::invoke(String const& name, Function&& f, Args&&... args) {
  return {bind(std::forward<Function>(f), std::forward<Args>(args)...), name};
}

template <typename Return>
ThreadFunction<Return>::ThreadFunction() {}

template <typename Return>
ThreadFunction<Return>::ThreadFunction(ThreadFunction&&) = default;

template <typename Return>
ThreadFunction<Return>::ThreadFunction(function<Return()> function, String const& name) {
  m_return = make_shared<Maybe<Return>>();
  m_function = ThreadFunction<void>([function = std::move(function), retValue = m_return]() {
      *retValue = function();
    }, name);
}

template <typename Return>
ThreadFunction<Return>::~ThreadFunction() {
  m_function.finish();
}

template <typename Return>
ThreadFunction<Return>& ThreadFunction<Return>::operator=(ThreadFunction&&) = default;

template <typename Return>
Return ThreadFunction<Return>::finish() {
  m_function.finish();
  return m_return->take();
}

template <typename Return>
bool ThreadFunction<Return>::isFinished() const {
  return m_function.isFinished();
}

template <typename Return>
bool ThreadFunction<Return>::isRunning() const {
  return m_function.isRunning();
}

template <typename Return>
ThreadFunction<Return>::operator bool() const {
  return !isFinished();
}

template <typename Return>
String ThreadFunction<Return>::name() {
  return m_function.name();
}

inline SpinLock::SpinLock() {
  m_lock.clear();
}

inline void SpinLock::lock() {
  while (m_lock.test_and_set(std::memory_order_acquire))
    ;
}

inline void SpinLock::unlock() {
  m_lock.clear(std::memory_order_release);
}

inline bool SpinLock::tryLock() {
  return !m_lock.test_and_set(std::memory_order_acquire);
}

}
