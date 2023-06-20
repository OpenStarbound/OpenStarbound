#include "StarTime.hpp"
#include "StarLexicalCast.hpp"
#include "StarMathCommon.hpp"

#include <ctime>
#include <windows.h>

namespace Star {

String Time::printDateAndTime(int64_t epochTicks, String format) {
  // playing fast and loose with the standard here...
  time_t requestedTime = epochTicks / epochTickFrequency();
  struct tm* ptm;
  ptm = localtime(&requestedTime);

  return format.replaceTags(StringMap<String>{
      {"year", strf("%04d", ptm->tm_year + 1900)},
      {"month", strf("%02d", ptm->tm_mon + 1)},
      {"day", strf("%02d", ptm->tm_mday)},
      {"hours", strf("%02d", ptm->tm_hour)},
      {"minutes", strf("%02d", ptm->tm_min)},
      {"seconds", strf("%02d", ptm->tm_sec)},
      {"millis", strf("%03d", (epochTicks % epochTickFrequency()) / (epochTickFrequency() / 1000))},
    });
}

int64_t Time::epochTicks() {
  FILETIME ft_now;
  GetSystemTimeAsFileTime(&ft_now);
  LONGLONG now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
  now -= 116444736000000000LL;
  return now;
}

int64_t Time::epochTickFrequency() {
  return 10000000LL;
}

struct MonotonicClock {
  MonotonicClock() {
    QueryPerformanceFrequency(&freq);
  };

  int64_t ticks() const {
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return ticks.QuadPart;
  }

  int64_t frequency() const {
    return freq.QuadPart;
  }

  LARGE_INTEGER freq;
};

static MonotonicClock g_monotonicClock;

int64_t Time::monotonicTicks() {
  return g_monotonicClock.ticks();
}

int64_t Time::monotonicTickFrequency() {
  return g_monotonicClock.frequency();
}


}
