#include "StarStatisticsDatabase.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

StatisticsDatabase::StatisticsDatabase() : m_cacheMutex(), m_eventCache() {
  auto assets = Root::singleton().assets();

  auto eventFiles = assets->scanExtension("event");
  assets->queueJsons(eventFiles);
  auto achievementFiles = assets->scanExtension("achievement");
  assets->queueJsons(achievementFiles);

  for (auto file : eventFiles) {
    try {
      String name = assets->json(file).getString("eventName");
      if (m_eventPaths.contains(name))
        Logger::error("Event {} defined twice, second time from {}", name, file);
      else
        m_eventPaths[name] = file;
    } catch (std::exception const& e) {
      Logger::error("Error loading event file {}: {}", file, outputException(e, true));
    }
  }

  for (auto file : achievementFiles) {
    try {
      Json achievement = assets->json(file);
      String name = achievement.getString("name");
      if (m_achievementPaths.contains(name))
        Logger::error("Achievement {} defined twice, second time from {}", name, file);
      else
        m_achievementPaths[name] = file;

      for (Json const& stat : achievement.getArray("triggers", {})) {
        m_statAchievements[stat.toString()].append(name);
      }
    } catch (std::exception const& e) {
      Logger::error("Error loading achievement file {}: {}", file, outputException(e, true));
    }
  }
}

StatEventPtr StatisticsDatabase::event(String const& name) const {
  MutexLocker locker(m_cacheMutex);
  return m_eventCache.get(name, [this](String const& name) -> StatEventPtr {
      if (auto path = m_eventPaths.maybe(name))
        return readEvent(*path);
      return {};
    });
}

AchievementPtr StatisticsDatabase::achievement(String const& name) const {
  MutexLocker locker(m_cacheMutex);
  return m_achievementCache.get(name, [this](String const& name) -> AchievementPtr {
      if (auto path = m_achievementPaths.maybe(name))
        return readAchievement(*path);
      return {};
    });
}

StringList StatisticsDatabase::allAchievements() const {
  return m_achievementPaths.keys();
}

StringList StatisticsDatabase::achievementsForStat(String const& statName) const {
  return m_statAchievements.value(statName);
}

StatEventPtr StatisticsDatabase::readEvent(String const& path) {
  auto assets = Root::singleton().assets();
  Json config = assets->json(path);

  return make_shared<StatEvent>(StatEvent {
      config.getString("eventName"),
      jsonToStringList(config.get("scripts")),
      config
    });
}

AchievementPtr StatisticsDatabase::readAchievement(String const& path) {
  auto assets = Root::singleton().assets();
  Json config = assets->json(path);

  return make_shared<Achievement>(Achievement {
      config.getString("name"),
      jsonToStringList(config.get("triggers")),
      jsonToStringList(config.get("scripts")),
      config
    });
}

}
