#include "StarStatisticsService_pc_steam.hpp"
#include "StarLogging.hpp"

namespace Star {

SteamStatisticsService::SteamStatisticsService(PcPlatformServicesStatePtr)
: m_callbackUserStatsReceived(this, &SteamStatisticsService::onUserStatsReceived),
  m_callbackUserStatsStored(this, &SteamStatisticsService::onUserStatsStored),
  m_callbackAchievementStored(this, &SteamStatisticsService::onAchievementStored) {
  m_appId = SteamUtils()->GetAppID();
  refresh();
}

bool SteamStatisticsService::initialized() const {
  return m_initialized;
}

Maybe<String> SteamStatisticsService::error() const {
  return m_error;
}

bool SteamStatisticsService::setStat(String const& name, String const& type, Json const& value) {
  if (type == "int")
    return SteamUserStats()->SetStat(name.utf8Ptr(), (int32_t)value.toInt());

  if (type == "float")
    return SteamUserStats()->SetStat(name.utf8Ptr(), value.toFloat());

  return false;
}

Json SteamStatisticsService::getStat(String const& name, String const& type, Json def) const {
  if (type == "int") {
    int32_t intValue = 0;
    if (SteamUserStats()->GetStat(name.utf8Ptr(), &intValue))
      return Json(intValue);
  }

  if (type == "float") {
    float floatValue = 0.0f;
    if (SteamUserStats()->GetStat(name.utf8Ptr(), &floatValue))
      return Json(floatValue);
  }

  return def;
}

bool SteamStatisticsService::reportEvent(String const&, Json const&) {
  // Steam doesn't support events
  return false;
}

bool SteamStatisticsService::unlockAchievement(String const& name) {
    std::vector<std::string> ValidSteamAchievements = {
    // list of all 51 steam achivements
        "completequest", "protectorate", "harvestcrop", "preparefood", "findoutpost",
        "findlore", "lunarbasemission", "findinstrument", "killmotherpoptop", "craftarmor",
        "findaugment", "floranmission", "gaincrew", "killdreadwing", "killinnocent",
        "hylotlmission", "findbike", "capturemonster", "avianmission", "findpgi",
        "killshockhopper", "gaintenant", "apexmission", "killbirds", "floranarena",
        "collectallfruit", "glitchmission", "destroyruin", "penguincrew", "killrobotchicken",
        "findbug", "maxcrew", "mazebound", "cookallfood", "largecolony",
        "10tenantquests", "crampedcolony", "museum", "restorefossil", "killplayer",
        "25tenantquests", "uniquetenants", "everyspeciescrew", "collectcodex", "findalpaca",
        "50tenantquests", "craftallarmors", "catchallbugs", "planetblocks", "collectionaf",
        "findallfossils"
    };

    for (const auto& achievement : ValidSteamAchievements) {
        if (achievement == name) {
            if (!SteamUserStats()->SetAchievement(name.utf8Ptr())) {
                Logger::error("Cannot set Steam achievement {}", name);
                return false;
            }
            return true;
        }
    }
    return false;
}

StringSet SteamStatisticsService::achievementsUnlocked() const {
  StringSet achievements;
  for (uint32_t i = 0; i < SteamUserStats()->GetNumAchievements(); ++i) {
    String achievement = SteamUserStats()->GetAchievementName(i);

    bool unlocked = false;
    if (SteamUserStats()->GetAchievement(achievement.utf8Ptr(), &unlocked) && unlocked) {
      achievements.add(achievement);
    }
  }
  return {};
}

void SteamStatisticsService::refresh() {
  if (!SteamUser()->BLoggedOn()) {
    m_error = {"Not logged in"};
    return;
  }

  SteamUserStats()->RequestCurrentStats();
}

void SteamStatisticsService::flush() {
  SteamUserStats()->StoreStats();
}

bool SteamStatisticsService::reset() {
  SteamUserStats()->ResetAllStats(true);
  return true;
}

void SteamStatisticsService::onUserStatsReceived(UserStatsReceived_t* callback) {
  if (callback->m_nGameID != m_appId)
    return;

  if (callback->m_eResult != k_EResultOK) {
    m_error = {strf("Steam RequestCurrentStats failed with code {}", (int)callback->m_eResult)};
    return;
  }

  Logger::debug("Steam RequestCurrentStats successful");
  m_initialized = true;
}

void SteamStatisticsService::onUserStatsStored(UserStatsStored_t* callback) {
  if (callback->m_nGameID != m_appId)
    return;

  if (callback->m_eResult == k_EResultOK) {
    Logger::debug("Steam StoreStats successful");
    return;
  }

  if (callback->m_eResult == k_EResultInvalidParam) {
    // A stat we set broke a constraint and was reverted on the service.
    Logger::info("Steam StoreStats: Some stats failed validation");
    return;
  }

  m_error = {strf("Steam StoreStats failed with code {}", (int)callback->m_eResult)};
}

void SteamStatisticsService::onAchievementStored(UserAchievementStored_t* callback) {
  if (callback->m_nGameID != m_appId)
    return;

  Logger::debug("Steam achievement {} stored successfully", callback->m_rgchAchievementName);
}

}
