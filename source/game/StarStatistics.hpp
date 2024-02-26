#pragma once

#include "StarVersioningDatabase.hpp"
#include "StarStatisticsDatabase.hpp"
#include "StarLuaComponents.hpp"
#include "StarStatisticsService.hpp"

namespace Star {

STAR_CLASS(Statistics);

class Statistics {
public:
  Statistics(String const& storageDirectory, StatisticsServicePtr service = {});

  void writeStatistics();

  Json stat(String const& name, Json def = {}) const;
  Maybe<String> statType(String const& name) const;
  bool achievementUnlocked(String const& name) const;

  void recordEvent(String const& name, Json const& fields);
  bool reset();

  void update();

private:
  struct Stat {
    static Stat fromJson(Json const& json);
    Json toJson() const;

    String type;
    Json value;
  };

  void processEvent(String const& name, Json const& fields);

  // setStat and unlockAchievement must be kept private as some platforms'
  // services don't implement the API calls these correspond to.
  void setStat(String const& name, String const& type, Json const& value);
  void unlockAchievement(String const& name);
  bool checkAchievement(String const& achievementName);

  void readStatistics();
  void mergeServiceStatistics();

  LuaCallbacks makeStatisticsCallbacks();

  template <typename Result = LuaValue, typename... V>
  Maybe<Result> runStatScript(StringList const& scripts, Json const& config, String const& functionName, V&&... args);

  StatisticsServicePtr m_service;
  String m_storageDirectory;
  bool m_initialized;

  List<pair<String, Json>> m_pendingEvents;
  StringSet m_pendingAchievementChecks;

  StringMap<Stat> m_stats;
  StringSet m_achievements;

  LuaRootPtr m_luaRoot;
};

}
