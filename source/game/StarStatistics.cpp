#include "StarStatistics.hpp"
#include "StarStatisticsDatabase.hpp"
#include "StarStatisticsService.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"

namespace Star {

Statistics::Statistics(String const& storageDirectory, StatisticsServicePtr service) {
  m_service = move(service);
  m_initialized = !m_service;
  m_storageDirectory = storageDirectory;
  readStatistics();

  auto assets = Root::singleton().assets();

  m_luaRoot = make_shared<LuaRoot>();
}

void Statistics::writeStatistics() {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  String filename = File::relativeTo(m_storageDirectory, "statistics");

  Json stats = JsonObject::from(m_stats.pairs().transformed([](auto const& entry) {
      return make_pair(entry.first, entry.second.toJson());
    }));
  JsonObject storage = {
      { "stats", stats },
      { "achievements", jsonFromStringSet(m_achievements) }
    };

  auto versionedStorage = versioningDatabase->makeCurrentVersionedJson("Statistics", storage);
  VersionedJson::writeFile(versionedStorage, filename);
}

Json Statistics::stat(String const& name, Json def) const {
  if (m_stats.contains(name))
    return m_stats.get(name).value;
  return def;
}

Maybe<String> Statistics::statType(String const& name) const {
  if (m_stats.contains(name))
    return m_stats.get(name).type;
  return {};
}

bool Statistics::achievementUnlocked(String const& name) const {
  return m_achievements.contains(name);
}

void Statistics::recordEvent(String const& name, Json const& fields) {
  m_pendingEvents.append(make_pair(name, fields));
}

bool Statistics::reset() {
  if (m_initialized) {
    if (!m_service || m_service->reset()) {
      m_stats = {};
      m_achievements = {};
      return true;
    }
  }
  return false;
}

void Statistics::update() {
  if (m_service) {
    if (auto error = m_service->error()) {
      Logger::error("Statistics platform service error: {}", *error);
      // Service failed. Continue with local stats and achievements only.
      m_service = {};
      m_initialized = true;
      return;
    }
  }

  if (!m_initialized) {
    if (m_service->initialized()) {
      mergeServiceStatistics();
      m_initialized = true;
    }
  }

  for (auto event : m_pendingEvents) {
    processEvent(event.first, event.second);
  }

  for (String const& achievement : m_pendingAchievementChecks) {
    if (checkAchievement(achievement))
      unlockAchievement(achievement);
  }

  if (m_service && (m_pendingEvents.size() > 0 || m_pendingAchievementChecks.size() > 0))
    m_service->flush();
  m_pendingEvents = {};
  m_pendingAchievementChecks = {};
}

Statistics::Stat Statistics::Stat::fromJson(Json const& json) {
  return Stat {
    json.getString("type"),
    json.get("value")
  };
}

Json Statistics::Stat::toJson() const {
  return JsonObject {
    {"type", type},
    {"value", value}
  };
}

void Statistics::processEvent(String const& name, Json const& fields) {
  if (m_service)
    m_service->reportEvent(name, fields);
  Logger::debug("Event {} {}", name, fields);

  auto statisticsDatabase = Root::singleton().statisticsDatabase();
  if (auto const& event = statisticsDatabase->event(name)) {
    runStatScript(event->scripts, event->config, "event", name, fields);
  }
}

void Statistics::setStat(String const& name, String const& type, Json const& value) {
  Logger::debug("Stat {} ({}) : {}", name, type, value);
  m_stats[name] = Stat { type, value };
  if (m_service)
    m_service->setStat(name, type, value);

  auto statisticsDatabase = Root::singleton().statisticsDatabase();
  m_pendingAchievementChecks.addAll(statisticsDatabase->achievementsForStat(name));
}

void Statistics::unlockAchievement(String const& name) {
  if (achievementUnlocked(name))
    return;
  m_achievements.add(name);
  if (m_service)
    m_service->unlockAchievement(name);
  Logger::debug("Achievement get {}", name);
}

bool Statistics::checkAchievement(String const& achievementName) {
  auto statisticsDatabase = Root::singleton().statisticsDatabase();
  auto achievement = statisticsDatabase->achievement(achievementName);
  if (achievementUnlocked(achievement->name))
    return true;

  Maybe<bool> result = runStatScript<bool>(achievement->scripts, achievement->config, "check", achievementName);
  return result && *result;
}

void Statistics::readStatistics() {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  try {
    String filename = File::relativeTo(m_storageDirectory, "statistics");
    if (File::exists(filename)) {
      Json storage = versioningDatabase->loadVersionedJson(VersionedJson::readFile(filename), "Statistics");

      m_stats = StringMap<Stat>::from(storage.getObject("stats", {}).pairs().transformed([](auto const& entry) {
          return make_pair(entry.first, Stat::fromJson(entry.second));
        }));
      m_achievements = jsonToStringSet(storage.get("achievements", JsonArray{}));

    }
  } catch (std::exception const& e) {
    Logger::warn("Error loading local player statistics file, resetting: {}", outputException(e, false));
  }
}

void Statistics::mergeServiceStatistics() {
  if (!m_service || !m_service->initialized() || m_service->error()) return;

  // Publish achievements we unlocked when the platform service was unavailable.
  StringSet serviceAchievements = m_service->achievementsUnlocked();
  for (String const& achievement : m_achievements.difference(serviceAchievements)) {
    m_service->unlockAchievement(achievement);
  }
  // Locally store all the achievements we unlocked in a different install:
  m_achievements.addAll(serviceAchievements);

  // Publish our local statistics, in case we made progress while the service
  // was unavailable.
  for (auto const& stat : m_stats) {
    m_service->setStat(stat.first, stat.second.type, stat.second.value);
  }

  // However, don't _pull_ stats from the service - not all stats are recorded
  // so inconsistencies will creep in if we try. For example, if the service
  // is recording the number of poptop kills but not the total number of kills,
  // we could end up with a situation like "2 total kills, 8 poptops killed."

  // The best we can do is let the client be authoritative over its stats and
  // have the service validate changes it receives to make sure they only
  // ever increase.

  m_service->flush();
}

LuaCallbacks Statistics::makeStatisticsCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<void, String, String, Json>("setStat", bind(&Statistics::setStat, this, _1, _2, _3));

  callbacks.registerCallbackWithSignature<Json, String, Json>("stat", bind(&Statistics::stat, this, _1, _2));

  callbacks.registerCallbackWithSignature<Maybe<String>, String>("statType", bind(&Statistics::statType, this, _1));

  callbacks.registerCallbackWithSignature<bool, String>("achievementUnlocked", bind(&Statistics::achievementUnlocked, this, _1));

  callbacks.registerCallbackWithSignature<bool, String>("checkAchievement", bind(&Statistics::checkAchievement, this, _1));

  callbacks.registerCallbackWithSignature<void, String>("unlockAchievement", bind(&Statistics::unlockAchievement, this, _1));

  return callbacks;
}

template <typename Result, typename... V>
Maybe<Result> Statistics::runStatScript(StringList const& scripts, Json const& config, String const& functionName, V&&... args) {
  LuaBaseComponent script;
  script.setLuaRoot(m_luaRoot);
  script.setScripts(scripts);
  script.addCallbacks("config", LuaBindings::makeConfigCallbacks([config] (String const& name, Json const& def) {
      return config.query(name, def);
    }));
  script.addCallbacks("statistics", makeStatisticsCallbacks());
  script.init();
  Maybe<Result> result = script.invoke<Result>(functionName, args...);
  script.uninit();
  return result;
}

}
