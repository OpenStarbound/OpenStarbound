#include "StarThread.hpp"
#include "StarTime.hpp"
#include "StarLogging.hpp"

#include <limits.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <dirent.h>
#include <pthread.h>
#ifdef STAR_SYSTEM_FREEBSD
#include <pthread_np.h>
#endif
#include <sys/time.h>
#include <errno.h>

#ifdef MAXCOMLEN
#define MAX_THREAD_NAMELEN MAXCOMLEN
#else
#define MAX_THREAD_NAMELEN 16
#endif

#ifndef STAR_SYSTEM_MACOS
#define STAR_MUTEX_TIMED
#endif

namespace Star {

struct ThreadImpl {
  static void* runThread(void* data) {
    ThreadImpl* ptr = static_cast<ThreadImpl*>(data);
    try {
#ifdef STAR_SYSTEM_MACOS
      // ensure the name is under the max allowed
      char tname[MAX_THREAD_NAMELEN];
      snprintf(tname, sizeof(tname), "%s", ptr->name.utf8Ptr());

      pthread_setname_np(tname);
#endif
      ptr->function();
    } catch (std::exception const& e) {
      if (ptr->name.empty())
        Logger::error("Exception caught in Thread: {}", outputException(e, true));
      else
        Logger::error("Exception caught in Thread {}: {}", ptr->name, outputException(e, true));
    } catch (...) {
      if (ptr->name.empty())
        Logger::error("Unknown exception caught in Thread");
      else
        Logger::error("Unknown exception caught in Thread {}", ptr->name);
    }
    ptr->stopped = true;
    return nullptr;
  }

  ThreadImpl(std::function<void()> function, String name)
    : function(std::move(function)), name(std::move(name)), stopped(true), joined(true) {}

  bool start() {
    MutexLocker mutexLocker(mutex);
    if (!joined)
      return false;

    stopped = false;
    joined = false;
    int ret = pthread_create(&pthread, NULL, &runThread, (void*)this);
    if (ret != 0) {
      stopped = true;
      joined = true;
      throw StarException(strf("Failed to create thread, error {}", ret));
    }

    // ensure the name is under the max allowed
    char tname[MAX_THREAD_NAMELEN];
    snprintf(tname, sizeof(tname), "%s", name.utf8Ptr());

#ifdef STAR_SYSTEM_FREEBSD
    pthread_set_name_np(pthread, tname);
#elif defined(STAR_SYSTEM_NETBSD)
    pthread_setname_np(pthread, "%s", tname);
#elif not defined STAR_SYSTEM_MACOS
    pthread_setname_np(pthread, tname);
#endif
    return true;
  }

  bool join() {
    MutexLocker mutexLocker(mutex);
    if (joined)
      return false;
    int ret = pthread_join(pthread, NULL);
    if (ret != 0)
      throw StarException(strf("Failed to join thread, error {}", ret));
    joined = true;
    return true;
  }

  std::function<void()> function;
  String name;
  pthread_t pthread;
  atomic<bool> stopped;
  bool joined;
  Mutex mutex;
};

struct ThreadFunctionImpl : ThreadImpl {
  ThreadFunctionImpl(std::function<void()> function, String name)
    : ThreadImpl(wrapFunction(std::move(function)), std::move(name)) {}

  std::function<void()> wrapFunction(std::function<void()> function) {
    return [function = std::move(function), this]() {
      try {
        function();
      } catch (...) {
        exception = std::current_exception();
      }
    };
  }

  std::exception_ptr exception;
};

struct MutexImpl {
  MutexImpl() {
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);

    pthread_mutex_init(&mutex, &mutexattr);

    pthread_mutexattr_destroy(&mutexattr);
  }

  ~MutexImpl() {
    pthread_mutex_destroy(&mutex);
  }

  void lock() {
#ifdef STAR_MUTEX_TIMED
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 60;
    if (pthread_mutex_timedlock(&mutex, &ts) != 0) {
      printStack("Mutex::lock is taking too long!");
#else
    {
#endif
      pthread_mutex_lock(&mutex);
    }
  }

  void unlock() {
    pthread_mutex_unlock(&mutex);
  }

  bool tryLock() {
    if (pthread_mutex_trylock(&mutex) == 0)
      return true;
    else
      return false;
  }

  pthread_mutex_t mutex;
};

struct ConditionVariableImpl {
  ConditionVariableImpl() {
    pthread_cond_init(&condition, NULL);
  }

  ~ConditionVariableImpl() {
    pthread_cond_destroy(&condition);
  }

  void wait(Mutex& mutex) {
    pthread_cond_wait(&condition, &mutex.m_impl->mutex);
  }

  void wait(Mutex& mutex, unsigned millis) {
    int64_t time = Time::millisecondsSinceEpoch() + millis;

    timespec ts;
    ts.tv_sec = time / 1000;
    ts.tv_nsec = (time % 1000) * 1000000;

    pthread_cond_timedwait(&condition, &mutex.m_impl->mutex, &ts);
  }

  void signal() {
    pthread_cond_signal(&condition);
  }

  void broadcast() {
    pthread_cond_broadcast(&condition);
  }

  pthread_cond_t condition;
};

struct RecursiveMutexImpl {
  RecursiveMutexImpl() {
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);

    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&mutex, &mutexattr);

    pthread_mutexattr_destroy(&mutexattr);
  }

  ~RecursiveMutexImpl() {
    pthread_mutex_destroy(&mutex);
  }

  void lock() {
#ifdef STAR_MUTEX_TIMED
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 60;
    if (pthread_mutex_timedlock(&mutex, &ts) != 0) {
      printStack("RecursiveMutex::lock is taking too long!");
#else
    {
#endif
      pthread_mutex_lock(&mutex);
    }
  }

  void unlock() {
    pthread_mutex_unlock(&mutex);
  }

  bool tryLock() {
    if (pthread_mutex_trylock(&mutex) == 0)
      return true;
    else
      return false;
  }

  pthread_mutex_t mutex;
};

void Thread::sleepPrecise(unsigned msecs) {
  int64_t now = Time::monotonicMilliseconds();
  int64_t deadline = now + msecs;

  while (deadline - now > 10) {
    usleep((deadline - now - 10) * 1000);
    now = Time::monotonicMilliseconds();
  }

  while (deadline > now) {
    usleep((deadline - now) * 500);
    now = Time::monotonicMilliseconds();
  }
}

void Thread::sleep(unsigned msecs) {
  usleep(msecs * 1000);
}

void Thread::yield() {
  sched_yield();
}

unsigned Thread::numberOfProcessors() {
  long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
  if (nprocs < 1)
    throw StarException(strf("Could not determine number of CPUs online: {}\n", strerror(errno)));
  return nprocs;
}

Thread::Thread(String const& name) {
  m_impl.reset(new ThreadImpl([this]() {
      run();
    }, name));
}

Thread::Thread(Thread&&) = default;

Thread::~Thread() {}

Thread& Thread::operator=(Thread&&) = default;

bool Thread::start() {
  return m_impl->start();
}

bool Thread::join() {
  return m_impl->join();
}

String Thread::name() {
  return m_impl->name;
}

bool Thread::isJoined() const {
  return m_impl->joined;
}

bool Thread::isRunning() const {
  return !m_impl->stopped;
}

ThreadFunction<void>::ThreadFunction() {}

ThreadFunction<void>::ThreadFunction(ThreadFunction&&) = default;

ThreadFunction<void>::ThreadFunction(function<void()> function, String const& name) {
  m_impl.reset(new ThreadFunctionImpl(std::move(function), name));
  m_impl->start();
}

ThreadFunction<void>::~ThreadFunction() {
  finish();
}

ThreadFunction<void>& ThreadFunction<void>::operator=(ThreadFunction&&) = default;

void ThreadFunction<void>::finish() {
  if (m_impl) {
    m_impl->join();

    if (m_impl->exception)
      std::rethrow_exception(take(m_impl->exception));
  }
}

bool ThreadFunction<void>::isFinished() const {
  return !m_impl || m_impl->joined;
}

bool ThreadFunction<void>::isRunning() const {
  return m_impl && !m_impl->stopped;
}

ThreadFunction<void>::operator bool() const {
  return !isFinished();
}

String ThreadFunction<void>::name() {
  if (m_impl)
    return m_impl->name;
  else
    return "";
}

Mutex::Mutex()
  : m_impl(new MutexImpl()) {}

Mutex::Mutex(Mutex&&) = default;

Mutex::~Mutex() {}

Mutex& Mutex::operator=(Mutex&&) = default;

void Mutex::lock() {
  m_impl->lock();
}

bool Mutex::tryLock() {
  return m_impl->tryLock();
}

void Mutex::unlock() {
  m_impl->unlock();
}

ConditionVariable::ConditionVariable()
  : m_impl(new ConditionVariableImpl()) {}

ConditionVariable::ConditionVariable(ConditionVariable&&) = default;

ConditionVariable::~ConditionVariable() {}

ConditionVariable& ConditionVariable::operator=(ConditionVariable&&) = default;

void ConditionVariable::wait(Mutex& mutex, Maybe<unsigned> millis) {
  if (millis)
    m_impl->wait(mutex, *millis);
  else
    m_impl->wait(mutex);
}

void ConditionVariable::signal() {
  m_impl->signal();
}

void ConditionVariable::broadcast() {
  m_impl->broadcast();
}

RecursiveMutex::RecursiveMutex()
  : m_impl(new RecursiveMutexImpl()) {}

RecursiveMutex::RecursiveMutex(RecursiveMutex&&) = default;

RecursiveMutex::~RecursiveMutex() {}

RecursiveMutex& RecursiveMutex::operator=(RecursiveMutex&&) = default;

void RecursiveMutex::lock() {
  m_impl->lock();
}

bool RecursiveMutex::tryLock() {
  return m_impl->tryLock();
}

void RecursiveMutex::unlock() {
  m_impl->unlock();
}

}
