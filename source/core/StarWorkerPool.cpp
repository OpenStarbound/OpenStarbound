#include "StarWorkerPool.hpp"
#include "StarIterator.hpp"
#include "StarMathCommon.hpp"

namespace Star {

bool WorkerPoolHandle::done() const {
  MutexLocker locker(m_impl->mutex);
  return m_impl->done;
}

bool WorkerPoolHandle::wait(unsigned millis) const {
  MutexLocker locker(m_impl->mutex);

  if (!m_impl->done && millis != 0)
    m_impl->condition.wait(m_impl->mutex, millis);

  if (m_impl->exception)
    std::rethrow_exception(m_impl->exception);

  return m_impl->done;
}

bool WorkerPoolHandle::poll() const {
  return wait(0);
}

void WorkerPoolHandle::finish() const {
  MutexLocker locker(m_impl->mutex);

  if (!m_impl->done)
    m_impl->condition.wait(m_impl->mutex);

  if (m_impl->exception)
    std::rethrow_exception(m_impl->exception);

  return;
}

WorkerPoolHandle::Impl::Impl() : done(false) {}

WorkerPoolHandle::WorkerPoolHandle(shared_ptr<Impl> impl) : m_impl(move(impl)) {}

WorkerPool::WorkerPool(String name) : m_name(move(name)) {}

WorkerPool::WorkerPool(String name, unsigned threadCount) : WorkerPool(move(name)) {
  start(threadCount);
}

WorkerPool::~WorkerPool() {
  stop();
}

WorkerPool::WorkerPool(WorkerPool&&) = default;
WorkerPool& WorkerPool::operator=(WorkerPool&&) = default;

void WorkerPool::start(unsigned threadCount) {
  MutexLocker threadLock(m_threadMutex);

  for (auto const& workerThread : m_workerThreads)
    workerThread->shouldStop = true;

  m_workCondition.broadcast();
  m_workerThreads.clear();

  for (size_t i = m_workerThreads.size(); i < threadCount; ++i)
    m_workerThreads.append(make_unique<WorkerThread>(this));
}

void WorkerPool::stop() {
  MutexLocker threadLock(m_threadMutex);
  for (auto const& workerThread : m_workerThreads)
    workerThread->shouldStop = true;

  {
    // Must hold the work lock while broadcasting to ensure that any worker
    // threads that might wait without stopping actually get the signal.
    MutexLocker workLock(m_workMutex);
    m_workCondition.broadcast();
  }

  m_workerThreads.clear();
}

void WorkerPool::finish() {
  // This is kind of a weird way to "wait" until all the pending work is
  // finished.  In order for the currently active worker threads to
  // cooperatively complete the remaining work, the work lock must not be held
  // the entire time (then just this thread would be the one finishing the
  // work).  Instead, the calling thread joins in on the action and tries to
  // finish work while yielding to the other threads after each completed job.
  MutexLocker workMutex(m_workMutex);
  while (!m_pendingWork.empty()) {
    auto firstWork = m_pendingWork.takeFirst();
    workMutex.unlock();
    firstWork();
    Thread::yield();
    workMutex.lock();
  }
  workMutex.unlock();

  stop();
}

WorkerPoolHandle WorkerPool::addWork(function<void()> work) {
  // Construct a worker pool handle and wrap the work to signal the handle when
  // finished.  Set the result to empty string if successful and to the content
  // of the exception if an exception is thrown.
  auto workerPoolHandleImpl = make_shared<WorkerPoolHandle::Impl>();
  queueWork([workerPoolHandleImpl, work]() {
    try {
      work();
      MutexLocker handleLocker(workerPoolHandleImpl->mutex);
      workerPoolHandleImpl->done = true;
      workerPoolHandleImpl->condition.broadcast();
    } catch (...) {
      MutexLocker handleLocker(workerPoolHandleImpl->mutex);
      workerPoolHandleImpl->done = true;
      workerPoolHandleImpl->exception = std::current_exception();
      workerPoolHandleImpl->condition.broadcast();
    }
  });

  return workerPoolHandleImpl;
}

WorkerPool::WorkerThread::WorkerThread(WorkerPool* parent)
  : Thread(strf("WorkerThread for WorkerPool '%s'", parent->m_name)),
    parent(parent),
    shouldStop(false),
    waiting(false) {
  start();
}

WorkerPool::WorkerThread::~WorkerThread() {
  join();
}

void WorkerPool::WorkerThread::run() {
  MutexLocker workLock(parent->m_workMutex);
  while (true) {
    if (shouldStop)
      break;

    if (parent->m_pendingWork.empty()) {
      waiting = true;
      parent->m_workCondition.wait(parent->m_workMutex);
      waiting = false;
    }

    if (!parent->m_pendingWork.empty()) {
      auto work = parent->m_pendingWork.takeFirst();
      workLock.unlock();
      work();
      workLock.lock();
    }
  }
}

void WorkerPool::queueWork(function<void()> work) {
  MutexLocker workLock(m_workMutex);
  m_pendingWork.append(move(work));
  m_workCondition.signal();
}

}
