#ifndef STAR_CLIENT_COMMAND_PROCESSOR_HPP
#define STAR_CLIENT_COMMAND_PROCESSOR_HPP

#include "StarShellParser.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarQuestManager.hpp"
#include "StarCinematic.hpp"
#include "StarMainInterfaceTypes.hpp"

namespace Star {

class ClientCommandProcessor {
public:
  ClientCommandProcessor(UniverseClientPtr universeClient, CinematicPtr cinematicOverlay,
      MainInterfacePaneManager* paneManager, StringMap<StringList> macroCommands);

  StringList handleCommand(String const& commandLine);

  bool debugDisplayEnabled() const;
  bool debugHudEnabled() const;
  bool fixedCameraEnabled() const;

private:
  bool adminCommandAllowed() const;
  String previewQuestPane(StringList const& arguments, function<PanePtr(QuestPtr)> createPane);

  String reload();
  String whoami();
  String gravity();
  String debug(StringList const& arguments);
  String boxes();
  String fullbright();
  String asyncLighting();
  String setGravity(StringList const& arguments);
  String resetGravity();
  String fixedCamera();
  String monochromeLighting();
  String radioMessage(StringList const& arguments);
  String clearRadioMessages();
  String clearCinematics();
  String startQuest(StringList const& arguments);
  String completeQuest(StringList const& arguments);
  String failQuest(StringList const& arguments);
  String previewNewQuest(StringList const& arguments);
  String previewQuestComplete(StringList const& arguments);
  String previewQuestFailed(StringList const& arguments);
  String clearScannedObjects();
  String playTime();
  String deathCount();
  String cinema(StringList const& arguments);
  String suicide();
  String naked();
  String resetAchievements();
  String statistic(StringList const& arguments);
  String giveEssentialItem(StringList const& arguments);
  String makeTechAvailable(StringList const& arguments);
  String enableTech(StringList const& arguments);
  String upgradeShip(StringList const& arguments);

  UniverseClientPtr m_universeClient;
  CinematicPtr m_cinematicOverlay;
  MainInterfacePaneManager* m_paneManager;
  CaseInsensitiveStringMap<function<String(StringList const&)>> m_builtinCommands;
  StringMap<StringList> m_macroCommands;
  ShellParser m_parser;
  LuaBaseComponent m_scriptComponent;
  bool m_debugDisplayEnabled = false;
  bool m_debugHudEnabled = true;
  bool m_fixedCameraEnabled = false;
};

}

#endif
