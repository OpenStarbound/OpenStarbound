#include "StarLockFile.hpp"
#include "StarTime.hpp"
#include "StarThread.hpp"

#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

namespace Star {

int64_t const LockFile::MaximumSleepMillis;

Maybe<LockFile> LockFile::acquireLock(String const& filename, int64_t lockTimeout) {
  LockFile lock(move(filename));
  if (lock.lock(lockTimeout))
    return lock;
  return {};
}

LockFile::LockFile(String const& filename) : m_filename(move(filename)) {}

LockFile::LockFile(LockFile&& lockFile) {
  operator=(move(lockFile));
}

LockFile::~LockFile() {
  unlock();
}

LockFile& LockFile::operator=(LockFile&& lockFile) {
  m_filename = move(lockFile.m_filename);
  m_handle = move(lockFile.m_handle);

  return *this;
}

bool LockFile::lock(int64_t timeout) {
  auto doFLock = [](String const& filename, bool block) -> shared_ptr<int> {
    int fd = open(filename.utf8Ptr(), O_RDONLY | O_CREAT, 0644);
    if (fd < 0)
      throw StarException(strf("Could not open lock file %s, %s\n", filename, strerror(errno)));

    int ret;
    if (block)
      ret = flock(fd, LOCK_EX);
    else
      ret = flock(fd, LOCK_EX | LOCK_NB);

    if (ret != 0) {
      close(fd);
      if (errno != EWOULDBLOCK)
        throw StarException(strf("Could not lock file %s, %s\n", filename, strerror(errno)));
      return {};
    }

    return make_shared<int>(fd);
  };

  if (timeout < 0) {
    m_handle = doFLock(m_filename, true);
    return true;
  } else if (timeout == 0) {
    m_handle = doFLock(m_filename, false);
    return (bool)m_handle;
  } else {
    int64_t startTime = Time::monotonicMilliseconds();
    while (true) {
      m_handle = doFLock(m_filename, false);
      if (m_handle)
        return true;

      if (Time::monotonicMilliseconds() - startTime > timeout)
        return false;

      Thread::sleep(min(timeout / 4, MaximumSleepMillis));
    }
  }
}

void LockFile::unlock() {
  if (m_handle) {
    int fd = *(int*)m_handle.get();
    unlink(m_filename.utf8Ptr());
    close(fd);
    m_handle.reset();
  }
}

bool LockFile::isLocked() const {
  return (bool)m_handle;
}

}
