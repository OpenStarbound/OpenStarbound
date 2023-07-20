#ifndef STAR_QUESTS_HPP
#define STAR_QUESTS_HPP

#include "StarSet.hpp"
#include "StarJsonRpc.hpp"
#include "StarItemDescriptor.hpp"
#include "StarDrawable.hpp"
#include "StarCelestialCoordinate.hpp"
#include "StarThread.hpp"
#include "StarQuestDescriptor.hpp"
#include "StarQuestTemplateDatabase.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaActorMovementComponent.hpp"
#include "StarWarping.hpp"

namespace Star {

STAR_CLASS(Quest);
STAR_CLASS(Player);
STAR_CLASS(UniverseClient);

enum class QuestState {
  // New - being set up and quest hasn't been offered yet (or was offered and declined)
  New,
  // Offer - waiting on the player to accept or decline the quest
  Offer,
  // Active - the quest was accepted and is in progress
  Active,
  // Complete - the quest finished successfully
  Complete,
  // Failed - the quest finished unsuccessfully or the player abandoned it
  Failed
};
extern EnumMap<QuestState> const QuestStateNames;

class Quest {
public:
  Quest(QuestArcDescriptor const& questArc, size_t arcPos, Player* player);

  Quest(Json const& diskStore);
  Json diskStore() const;

  QuestTemplatePtr getTemplate() const;

  void init(Player* player, World* world, UniverseClient* client);
  void uninit();

  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args = {});
  void update(float dt);

  void offer();
  void declineOffer();
  void start();
  void complete(Maybe<size_t> followupIndex = {});
  void fail();
  void abandon();

  bool interactWithEntity(EntityId entity);

  // The generated ID for this instance of the quest with these specific
  // parameters. Multiple players iin a universe may have quests with the same
  // questId if the the source of the quest was the same.
  String questId() const;
  // The ID of the template this quest was created from
  String templateId() const;

  StringMap<QuestParam> const& parameters() const;

  QuestState state() const;

  // Whether to show the Complete / Failed dialog
  bool showDialog() const;
  void setDialogShown();

  void setEntityParameter(String const& paramName, EntityConstPtr const& entity);
  void setParameter(String const& paramName, QuestParam const& paramValue);

  Maybe<List<Drawable>> portrait(String const& portraitName) const;
  Maybe<String> portraitTitle(String const& portraitName) const;

  QuestDescriptor questDescriptor() const;
  QuestArcDescriptor questArcDescriptor() const;
  size_t questArcPosition() const;

  Maybe<WorldId> worldId() const;
  Maybe<pair<Vec3I, SystemLocation>> location() const;
  Maybe<Uuid> serverUuid() const;
  void setWorldId(Maybe<WorldId> worldId);
  void setLocation(Maybe<pair<Vec3I, SystemLocation>> location);
  void setServerUuid(Maybe<Uuid> serverUuid);

  String title() const;
  String text() const;
  String completionText() const;
  String failureText() const;

  size_t money() const;
  List<ItemConstPtr> rewards() const;

  // The time when this quest last changed state (active/completed/failed)
  int64_t lastUpdatedOn() const;
  bool unread() const;
  void markAsRead();
  bool canTurnIn() const;

  String questGiverIndicator() const;
  String questReceiverIndicator() const;

  // The String returned by this method is an image path, not a reference to a configured indicator
  Maybe<String> customIndicator(EntityPtr const& entity) const;

  Maybe<JsonArray> objectiveList() const;
  Maybe<float> progress() const;
  Maybe<float> compassDirection() const;

  void setObjectiveList(Maybe<JsonArray> const& objectiveList);
  void setProgress(Maybe<float> const& progress);
  void setCompassDirection(Maybe<float> const& compassDirection);

  Maybe<String> completionCinema() const;
  bool canBeAbandoned() const;
  bool ephemeral() const;
  bool showInLog() const;
  bool showAcceptDialog() const;
  bool showCompleteDialog() const;
  bool showFailDialog() const;
  bool mainQuest() const;
  bool hideCrossServer() const;

private:
  struct DisplayParameters {
    bool ephemeral;
    bool showInLog;
    bool showAcceptDialog;
    bool showCompleteDialog;
    bool showFailDialog;
    bool mainQuest;
    bool hideCrossServer;
  };

  void setState(QuestState state);

  void initScript();
  void uninitScript();
  LuaCallbacks makeQuestCallbacks(Player* player);

  void setEntityParameter(String const& paramName, Entity const* entity);
  void addReward(ItemDescriptor const& reward);

  String const& defaultCustomIndicator() const;

  Player* m_player;
  World* m_world;
  UniverseClient* m_client;

  QuestState m_state;
  bool m_inited;
  bool m_showDialog;

  QuestArcDescriptor m_arc;
  size_t m_arcPos;
  StringMap<QuestParam> m_parameters;
  DisplayParameters m_displayParameters;
  Maybe<WorldId> m_worldId;
  Maybe<pair<Vec3I, SystemLocation>> m_location;
  Maybe<Uuid> m_serverUuid;
  size_t m_money;
  List<ItemConstPtr> m_rewards;
  int64_t m_lastUpdatedOn;
  bool m_unread;
  bool m_canTurnIn;
  StringSet m_indicators;

  String m_trackedIndicator;
  String m_untrackedIndicator;

  String m_title;
  String m_text;
  String m_completionText;
  String m_failureText;
  StringMap<List<Drawable>> m_portraits;
  StringMap<String> m_portraitTitles;

  Maybe<JsonArray> m_objectiveList;
  Maybe<float> m_progress;
  Maybe<float> m_compassDirection;

  LuaMessageHandlingComponent<LuaActorMovementComponent<LuaUpdatableComponent<LuaStorableComponent<LuaWorldComponent<LuaBaseComponent>>>>>
      m_scriptComponent;
};

// Create an instance of Quest for a specific template with all the parameters filled
// in with examples. Doesn't necessarily make a valid quest that can be completed, since
// its purpose is for previewing dialogs only.
QuestPtr createPreviewQuest(
    String const& templateId, String const& position, String const& questGiverSpecies, Player* player);
}

#endif
