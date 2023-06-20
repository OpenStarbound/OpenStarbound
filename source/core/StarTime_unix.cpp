#include "StarTime.hpp"
#include "StarMathCommon.hpp"

#include <sys/time.h>

#ifdef STAR_SYSTEM_MACOS
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

namespace Star {

String Time::printDateAndTime(int64_t epochTicks, String format) {
  // playing fast and loose with the standard here...
  time_t requestedTime = epochTicks / epochTickFrequency();
  struct tm ptm;
  localtime_r(&requestedTime, &ptm);

  return format.replaceTags(StringMap<String>{
      {"year", strf("%04d", ptm.tm_year + 1900)},
      {"month", strf("%02d", ptm.tm_mon + 1)},
      {"day", strf("%02d", ptm.tm_mday)},
      {"hours", strf("%02d", ptm.tm_hour)},
      {"minutes", strf("%02d", ptm.tm_min)},
      {"seconds", strf("%02d", ptm.tm_sec)},
      {"millis", strf("%03d", (epochTicks % epochTickFrequency()) / (epochTickFrequency() / 1000))}
    });
}

int64_t Time::epochTicks() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1'000'000 + tv.tv_usec;
}

int64_t Time::epochTickFrequency() {
  return 1'000'000;
}

#ifdef STAR_SYSTEM_MACOS

struct MonotonicClock {
  MonotonicClock() {
    mach_timebase_info(&timebaseInfo);
  };

  int64_t ticks() const {
    int64_t t = mach_absolute_time();
    return (t / 100) * timebaseInfo.numer / timebaseInfo.denom;
  }

  int64_t frequency() const {
    // hard coded to 100ns increments
    return 10'000'000;
  }

  mach_timebase_info_data_t timebaseInfo;
};

#else

struct MonotonicClock {
  MonotonicClock() {
    timespec ts;
    clock_getres(CLOCK_MONOTONIC, &ts);
    starAssert(ts.tv_sec == 0);
    storedFrequency = 1'000'000'000 / ts.tv_nsec;
  };

  int64_t ticks() const {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * storedFrequency + ts.tv_nsec * storedFrequency / 1'000'000'000;
  }

  int64_t frequency() const {
    return storedFrequency;
  }

  int64_t storedFrequency;
};

#endif

static MonotonicClock g_monotonicClock;

int64_t Time::monotonicTicks() {
  return g_monotonicClock.ticks();
}

int64_t Time::monotonicTickFrequency() {
  return g_monotonicClock.frequency();
}

}
