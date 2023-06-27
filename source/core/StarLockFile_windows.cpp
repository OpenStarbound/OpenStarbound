#include "StarLockFile.hpp"
#include "StarTime.hpp"
#include "StarThread.hpp"

#include "StarString_windows.hpp"

#include <windows.h>

namespace Star {

int64_t const LockFile::MaximumSleepMillis;

Maybe<LockFile> LockFile::acquireLock(String const& filename, int64_t lockTimeout) {
  LockFile lock(move(filename));
  if (lock.lock(lockTimeout))
    return move(lock);
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
  auto doFLock = [](String const& filename) -> shared_ptr<HANDLE> {
    HANDLE handle = CreateFileW(
        stringToUtf16(filename).get(), GENERIC_READ, 0, nullptr, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
      if (GetLastError() == ERROR_SHARING_VIOLATION)
        return {};
      throw StarException(strf("Could not open lock file {}, error code {}\n", filename, GetLastError()));
    }

    return make_shared<HANDLE>(handle);
  };

  if (timeout == 0) {
    m_handle = doFLock(m_filename);
    return (bool)m_handle;
  } else {
    int64_t startTime = Time::monotonicMilliseconds();
    while (true) {
      m_handle = doFLock(m_filename);
      if (m_handle)
        return true;

      if (timeout > 0 && Time::monotonicMilliseconds() - startTime > timeout)
        return false;

      Thread::sleep(min(timeout / 4, MaximumSleepMillis));
    }
  }
}

void LockFile::unlock() {
  if (m_handle) {
    HANDLE handle = *(HANDLE*)m_handle.get();
    CloseHandle(handle);
    m_handle.reset();
  }
}

bool LockFile::isLocked() const {
  return (bool)m_handle;
}

}
