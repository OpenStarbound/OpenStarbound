#pragma once

#include "StarQuests.hpp"

namespace Star {

STAR_CLASS(QuestManager);

struct QuestIndicator {
  String indicatorImage;
  Vec2F worldPosition;
};

class QuestManager {
public:
  QuestManager(Player* player);

  void diskLoad(Json const& quests);
  Json diskStore();

  void setUniverseClient(UniverseClient* client);

  void init(World* world);
  void uninit();

  bool canStart(QuestArcDescriptor const& questArc) const;

  // Show a dialog offering the player a quest, and later start it if they
  // accept it.
  void offer(QuestPtr const& quest);
  StringMap<QuestPtr> quests() const;
  // Only returns quests that are exclusive to the current server.
  StringMap<QuestPtr> serverQuests() const;
  QuestPtr getQuest(String const& questId) const;

  bool hasQuest(String const& questId) const;
  bool hasAcceptedQuest(String const& questId) const;
  bool isActive(String const& questId) const;
  bool isCurrent(String const& questId) const;
  bool isTracked(String const& questId) const;
  void setAsTracked(Maybe<String> const& questId);
  void markAsRead(String const& questId);
  bool hasCompleted(String const& questId) const;
  bool canTurnIn(String const& questId) const;

  Maybe<QuestPtr> getFirstNewQuest();
  Maybe<QuestPtr> getFirstCompletableQuest();
  Maybe<QuestPtr> getFirstFailableQuest();
  Maybe<QuestPtr> getFirstMainQuest();

  List<QuestPtr> listActiveQuests() const;
  List<QuestPtr> listCompletedQuests() const;
  List<QuestPtr> listFailedQuests() const;

  Maybe<QuestPtr> currentQuest() const;
  Maybe<QuestPtr> trackedQuest() const;
  Maybe<QuestIndicator> getQuestIndicator(EntityPtr const& entity) const;

  // Handled at this level to allow multiple active quests to specify interestingObjects
  StringSet interestingObjects();

  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args = {});
  void update(float dt);

private:
  void startInitialQuests();
  void setMostRecentQuestCurrent();
  bool questValidOnServer(QuestPtr quest) const;

  Player* m_player;
  World* m_world;
  UniverseClient* m_client;

  StringMap<QuestPtr> m_quests;

  Maybe<String> m_trackedQuestId;
  bool m_trackOnWorldQuests;
  Maybe<String> m_onWorldQuestId;
};

}
