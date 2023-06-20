#ifndef STAR_STATS_BACKEND_HPP
#define STAR_STATS_BACKEND_HPP

#include "StarJson.hpp"

namespace Star {

STAR_CLASS(StatisticsService);

class StatisticsService {
public:
  virtual ~StatisticsService() = default;

  virtual bool initialized() const = 0;
  virtual Maybe<String> error() const = 0;

  // The functions below aren't valid unless initialized() returns true and
  // error() is empty.

  // setStat should return false for stats or types that aren't known by the
  // service, without reporting an error.
  // By sending all stats to the StatisticsService, we can configure collection
  // of new stats entirely on the service, without any modifications to the game.
  virtual bool setStat(String const& name, String const& type, Json const& value) = 0;
  virtual Json getStat(String const& name, String const& type, Json def = {}) const = 0;

  // reportEvent should return false if the service doesn't handle this event.
  virtual bool reportEvent(String const& name, Json const& fields) = 0;

  virtual bool unlockAchievement(String const& name) = 0;
  virtual StringSet achievementsUnlocked() const = 0;

  virtual void refresh() = 0;
  virtual void flush() = 0;
  virtual bool reset() = 0;
};

}

#endif
