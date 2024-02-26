#pragma once

#include "StarThread.hpp"

namespace Star {

STAR_EXCEPTION(WorkerPoolException, StarException);

STAR_CLASS(WorkerPool);

// Shareable handle for a WorkerPool computation that does not produce any
// value.
class WorkerPoolHandle {
public:
  // Returns true if the work is completed (either due to error or actual
  // completion, will not re-throw)
  bool done() const;

  // Waits up to given millis for the computation to finish.  Returns true if
  // the computation finished within the allotted time, false otherwise.  If
  // the computation is finished but it threw an exception, it will be
  // re-thrown here.
  bool wait(unsigned millis) const;

  // synonym for wait(0)
  bool poll() const;

  // Wait until the computation finishes completely.  If the computation threw
  // an exception it will be re-thrown by this method.
  void finish() const;

private:
  friend WorkerPool;

  struct Impl {
    Impl();

    Mutex mutex;
    ConditionVariable condition;
    atomic<bool> done;
    std::exception_ptr exception;
  };

  WorkerPoolHandle(shared_ptr<Impl> impl);

  shared_ptr<Impl> m_impl;
};

// Shareable handle for a WorkerPool computation that produces a value.
template <typename ResultType>
class WorkerPoolPromise {
public:
  // Returns true if the work is completed (either due to error or actual
  // completion, will not re-throw)
  bool done() const;

  // Waits for the given amount of time for the work to be completed.  If the
  // work is completed, returns true.  If the producer function throws for any
  // reason, this method will re-throw the exception.  If millis is zero, does
  // not wait at all simply polls to see if the computation is finished.
  bool wait(unsigned millis) const;

  // synonym for wait(0)
  bool poll() const;

  // Blocks until the work is done, and returns the result.  May be called
  // multiple times to access the result.  If the computation threw
  // an exception it will be re-thrown by this method.
  ResultType& get();
  ResultType const& get() const;

private:
  friend WorkerPool;

  struct Impl {
    Mutex mutex;
    ConditionVariable condition;
    Maybe<ResultType> result;
    std::exception_ptr exception;
  };

  WorkerPoolPromise(shared_ptr<Impl> impl);

  shared_ptr<Impl> m_impl;
};

class WorkerPool {
public:
  // Creates a stopped pool
  WorkerPool(String name);
  // Creates a started pool
  WorkerPool(String name, unsigned threadCount);
  ~WorkerPool();

  WorkerPool(WorkerPool&&);
  WorkerPool& operator=(WorkerPool&&);

  // Start the thread pool with the given thread count range, or if it is
  // already started, reconfigure the thread counts.
  void start(unsigned threadCount);

  // Stop the thread pool, not necessarily finishing any pending jobs (may
  // leave pending jobs on the queue).
  void stop();

  // Try to finish any remaining jobs, then stop the thread pool.  This method
  // must not be called if the worker pool will continuously receive new work,
  // as it may not ever complete if that is the case.  The work queue must
  // eventually become empty for this to properly return.
  void finish();

  // Add the given work to the pool and return a handle for the work.  It not
  // required that the caller of this method hold on to the worker handle, the
  // work will be managed and completed regardless of the WorkerPoolHandle
  // lifetime.
  WorkerPoolHandle addWork(function<void()> work);

  // Like addWork, but the worker is expected to produce some result.  The
  // returned promise can be used to get this return value once the producer is
  // complete.
  template <typename ResultType>
  WorkerPoolPromise<ResultType> addProducer(function<ResultType()> producer);

private:
  class WorkerThread : public Thread {
  public:
    // Starts automatically
    WorkerThread(WorkerPool* parent);
    ~WorkerThread();

    void run() override;

    WorkerPool* parent;
    atomic<bool> shouldStop;
    atomic<bool> waiting;
  };

  void queueWork(function<void()> work);

  String m_name;
  Mutex m_threadMutex;
  List<unique_ptr<WorkerThread>> m_workerThreads;

  Mutex m_workMutex;
  ConditionVariable m_workCondition;
  Deque<function<void()>> m_pendingWork;
};

template <typename ResultType>
bool WorkerPoolPromise<ResultType>::done() const {
  MutexLocker locker(m_impl->mutex);
  return m_impl->result || m_impl->exception;
}

template <typename ResultType>
bool WorkerPoolPromise<ResultType>::wait(unsigned millis) const {
  MutexLocker locker(m_impl->mutex);

  if (!m_impl->result && !m_impl->exception && millis != 0)
    m_impl->condition.wait(m_impl->mutex, millis);

  if (m_impl->exception)
    std::rethrow_exception(m_impl->exception);

  if (m_impl->result)
    return true;

  return false;
}

template <typename ResultType>
bool WorkerPoolPromise<ResultType>::poll() const {
  return wait(0);
}

template <typename ResultType>
ResultType& WorkerPoolPromise<ResultType>::get() {
  MutexLocker locker(m_impl->mutex);

  if (!m_impl->result && !m_impl->exception)
    m_impl->condition.wait(m_impl->mutex);

  if (m_impl->exception)
    std::rethrow_exception(m_impl->exception);

  return *m_impl->result;
}

template <typename ResultType>
ResultType const& WorkerPoolPromise<ResultType>::get() const {
  return const_cast<WorkerPoolPromise*>(this)->get();
}

template <typename ResultType>
WorkerPoolPromise<ResultType>::WorkerPoolPromise(shared_ptr<Impl> impl)
  : m_impl(std::move(impl)) {}

template <typename ResultType>
WorkerPoolPromise<ResultType> WorkerPool::addProducer(function<ResultType()> producer) {
  // Construct a worker pool promise and wrap the producer to signal the
  // promise when finished.
  auto workerPoolPromiseImpl = make_shared<typename WorkerPoolPromise<ResultType>::Impl>();
  queueWork([workerPoolPromiseImpl, producer]() {
    try {
      auto result = producer();
      MutexLocker promiseLocker(workerPoolPromiseImpl->mutex);
      workerPoolPromiseImpl->result = std::move(result);
      workerPoolPromiseImpl->condition.broadcast();
    } catch (...) {
      MutexLocker promiseLocker(workerPoolPromiseImpl->mutex);
      workerPoolPromiseImpl->exception = std::current_exception();
      workerPoolPromiseImpl->condition.broadcast();
    }
  });

  return workerPoolPromiseImpl;
}

}
