#include "StarLogging.hpp"
#include "StarPlatformServices_pc.hpp"
#include "StarP2PNetworkingService_pc.hpp"

#ifdef STAR_ENABLE_STEAM_INTEGRATION
#include "StarStatisticsService_pc_steam.hpp"
#include "StarUserGeneratedContentService_pc_steam.hpp"
#include "StarDesktopService_pc_steam.hpp"
#endif

namespace Star {

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
uint64_t const DiscordClientId = 467102538278109224;
#endif

PcPlatformServicesState::PcPlatformServicesState()
#ifdef STAR_ENABLE_STEAM_INTEGRATION
  : callbackGameOverlayActivated(this, &PcPlatformServicesState::onGameOverlayActivated) {
#else
  {
#endif

#ifdef STAR_ENABLE_STEAM_INTEGRATION
  if (SteamAPI_Init()) {
    steamAvailable = true;
    Logger::info("Initialized Steam platform services");
  } else {
    Logger::info("Failed to initialize Steam platform services");
  }
#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  static int64_t const DiscordEventSleep = 3;

  discord::Core* discordCorePtr = nullptr;
  discord::Result res = discord::Core::Create(DiscordClientId, DiscordCreateFlags_NoRequireDiscord, &discordCorePtr);
  if (res == discord::Result::Ok && discordCorePtr) {
    discordCore.reset(discordCorePtr);
    discordAvailable = true;

    discordCore->UserManager().OnCurrentUserUpdate.Connect([this](){
        discord::User user;
        auto res = discordCore->UserManager().GetCurrentUser(&user);
        if (res != discord::Result::Ok)
          Logger::error("Could not get current discord user. (err {})", (int)res);
        else
          discordCurrentUser = user;
      });

  } else {
    Logger::error("Failed to instantiate discord core (err {})", (int)res);
  }

  if (discordAvailable) {
    MutexLocker locker(discordMutex);
    discordCore->SetLogHook(discord::LogLevel::Info, [](discord::LogLevel level, char const* msg) {
      if (level == discord::LogLevel::Debug)
        Logger::debug("[DISCORD]: {}", msg);
      else if (level == discord::LogLevel::Error)
        Logger::debug("[DISCORD]: {}", msg);
      else if (level == discord::LogLevel::Info)
        Logger::info("[DISCORD]: {}", msg);
      else if (level == discord::LogLevel::Warn)
        Logger::warn("[DISCORD]: {}", msg);
    });
    discordEventShutdown = false;
    discordEventThread = Thread::invoke("PcPlatformServices::discordEventThread", [this]() {
        while (!discordEventShutdown) {
          {
            MutexLocker locker(discordMutex);
            discordCore->RunCallbacks();
            discordCore->LobbyManager().FlushNetwork();
          }
          Thread::sleep(DiscordEventSleep);
        }
      });

    Logger::info("Initialized Discord platform services");
  } else {
    Logger::info("Was not able to authenticate with Discord and create all components, Discord services will be unavailable");
  }
#endif
}

PcPlatformServicesState::~PcPlatformServicesState() {
#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  if (discordAvailable) {
    discordEventShutdown = true;
    discordEventThread.finish();
  }
#endif
}

#ifdef STAR_ENABLE_STEAM_INTEGRATION
void PcPlatformServicesState::onGameOverlayActivated(GameOverlayActivated_t* callback) {
  overlayActive = callback->m_bActive;
}
#endif

PcPlatformServicesUPtr PcPlatformServices::create([[maybe_unused]] String const& path, StringList platformArguments) {
  auto services = unique_ptr<PcPlatformServices>(new PcPlatformServices);

  services->m_state = make_shared<PcPlatformServicesState>();

  bool provideP2PNetworking = false;

#ifdef STAR_ENABLE_STEAM_INTEGRATION
  provideP2PNetworking |= services->m_state->steamAvailable;
#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  provideP2PNetworking |= services->m_state->discordAvailable;
#endif

  if (provideP2PNetworking) {
    auto p2pNetworkingService = make_shared<PcP2PNetworkingService>(services->m_state);
    for (auto& argument : platformArguments) {
      if (argument.beginsWith("+platform:connect:")) {
        Logger::info("PC platform services joining from command line argument '{}'", argument);
        p2pNetworkingService->addPendingJoin(std::move(argument));
      } else {
        throw ApplicationException::format("Unrecognized PC platform services command line argument '{}'", argument);
      }
    }

    services->m_p2pNetworkingService = p2pNetworkingService;
  }

#ifdef STAR_ENABLE_STEAM_INTEGRATION
  if (services->m_state->steamAvailable) {
    services->m_statisticsService = make_shared<SteamStatisticsService>(services->m_state);
    services->m_userGeneratedContentService = make_shared<SteamUserGeneratedContentService>(services->m_state);
    services->m_desktopService = make_shared<SteamDesktopService>(services->m_state);
  }
#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  MutexLocker discordLocker(services->m_state->discordMutex);
  if (services->m_state->discordAvailable) {
    Logger::debug("Registering starbound to discord at path: {}", path);
    services->m_state->discordCore->ActivityManager().RegisterCommand(path.utf8Ptr());
  }
#endif

  return services;
}

StatisticsServicePtr PcPlatformServices::statisticsService() const {
  return m_statisticsService;
}

P2PNetworkingServicePtr PcPlatformServices::p2pNetworkingService() const {
  return m_p2pNetworkingService;
}

UserGeneratedContentServicePtr PcPlatformServices::userGeneratedContentService() const {
  return m_userGeneratedContentService;
}

DesktopServicePtr PcPlatformServices::desktopService() const {
  return m_desktopService;
}

bool PcPlatformServices::overlayActive() const {
  return m_state->overlayActive;
}

void PcPlatformServices::update() {
#ifdef STAR_ENABLE_STEAM_INTEGRATION
  SteamAPI_RunCallbacks();
#endif
}

}
