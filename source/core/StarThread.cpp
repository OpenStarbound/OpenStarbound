#include "StarThread.hpp"
#include "StarFormat.hpp"

namespace Star {

ReadersWriterMutex::ReadersWriterMutex()
  : m_readers(), m_writers(), m_readWaiters(), m_writeWaiters() {}

void ReadersWriterMutex::readLock() {
  MutexLocker locker(m_mutex);
  if (m_writers || m_writeWaiters) {
    m_readWaiters++;
    while (m_writers || m_writeWaiters)
      m_readCond.wait(m_mutex);
    m_readWaiters--;
  }
  m_readers++;
}

bool ReadersWriterMutex::tryReadLock() {
  MutexLocker locker(m_mutex);
  if (m_writers || m_writeWaiters)
    return false;
  m_readers++;
  return true;
}

void ReadersWriterMutex::readUnlock() {
  MutexLocker locker(m_mutex);
  m_readers--;
  if (m_writeWaiters)
    m_writeCond.signal();
}

void ReadersWriterMutex::writeLock() {
  MutexLocker locker(m_mutex);
  if (m_readers || m_writers) {
    m_writeWaiters++;
    while (m_readers || m_writers)
      m_writeCond.wait(m_mutex);
    m_writeWaiters--;
  }
  m_writers = 1;
}

bool ReadersWriterMutex::tryWriteLock() {
  MutexLocker locker(m_mutex);
  if (m_readers || m_writers)
    return false;
  m_writers = 1;
  return true;
}

void ReadersWriterMutex::writeUnlock() {
  MutexLocker locker(m_mutex);
  m_writers = 0;
  if (m_writeWaiters)
    m_writeCond.signal();
  else if (m_readWaiters)
    m_readCond.broadcast();
}

ReadLocker::ReadLocker(ReadersWriterMutex& rwlock, bool startLocked) : m_lock(rwlock), m_locked(false) {
  if (startLocked)
    lock();
}

ReadLocker::~ReadLocker() {
  unlock();
}

void ReadLocker::unlock() {
  if (m_locked)
    m_lock.readUnlock();
  m_locked = false;
}

void ReadLocker::lock() {
  if (!m_locked)
    m_lock.readLock();
  m_locked = true;
}

bool ReadLocker::tryLock() {
  if (!m_locked) {
    m_locked = m_lock.tryReadLock();
    return m_locked;
  }
  return true;
}

WriteLocker::WriteLocker(ReadersWriterMutex& rwlock, bool startLocked) : m_lock(rwlock), m_locked(false) {
  if (startLocked)
    lock();
}

WriteLocker::~WriteLocker() {
  unlock();
}

void WriteLocker::unlock() {
  if (m_locked)
    m_lock.writeUnlock();
  m_locked = false;
}

void WriteLocker::lock() {
  if (!m_locked)
    m_lock.writeLock();
  m_locked = true;
}

bool WriteLocker::tryLock() {
  if (!m_locked) {
    m_locked = m_lock.tryWriteLock();
    return m_locked;
  }
  return true;
}

}
