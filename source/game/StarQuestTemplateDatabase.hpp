#pragma once

#include "StarItemDescriptor.hpp"
#include "StarThread.hpp"
#include "StarVector.hpp"
#include "StarStrongTypedef.hpp"

namespace Star {

STAR_CLASS(SpeciesTextVariants);
STAR_CLASS(PositionalTextVariants);
STAR_CLASS(QuestTemplate);
STAR_CLASS(QuestTemplateDatabase);

// A Quest Template
// Used to check prerequisites for quest availability and by the QuestManager to
// instantiate quests
class QuestTemplate {
public:
  QuestTemplate(Json const& config);

  Json config;
  String templateId;
  String title;
  String text;
  String completionText;
  String failureText;
  StringMap<String> parameterTypes;
  JsonObject parameterExamples;
  Vec2U moneyRange;
  List<List<ItemDescriptor>> rewards;
  List<String> rewardParameters;
  Maybe<String> completionCinema;
  bool canBeAbandoned;
  // Whether the quest is cleared from the quest log when it is
  // completed/failed.
  bool ephemeral;
  // determine whether to show the quest in the quest log
  bool showInLog;
  // whether to show the quest accept, quest complete, and/or quest fail popups
  bool showAcceptDialog;
  bool showCompleteDialog;
  bool showFailDialog;
  // main quests are listed separately in the quest log
  bool mainQuest;
  // hide from log when the quest server uuid doesn't match the current client context server uuid
  bool hideCrossServer;
  String questGiverIndicator;
  String questReceiverIndicator;

  List<String> prerequisiteQuests;
  Maybe<unsigned> requiredShipLevel;
  List<ItemDescriptor> requiredItems;

  unsigned updateDelta;
  Maybe<String> script;
  JsonObject scriptConfig;

  Maybe<String> newQuestGuiConfig;
  Maybe<String> questCompleteGuiConfig;
  Maybe<String> questFailedGuiConfig;
};

// Quest Template Database
// Stores and returns from the list of known quest templates
class QuestTemplateDatabase {
public:
  QuestTemplateDatabase();

  // Return a list of all known template id values
  List<String> allQuestTemplateIds() const;

  // Return the template for the given template id
  QuestTemplatePtr questTemplate(String const& templateId) const;

private:
  StringMap<QuestTemplatePtr> m_templates;
};

}
