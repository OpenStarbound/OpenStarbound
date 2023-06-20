#ifndef STAR_QUEST_INTERFACE_HPP
#define STAR_QUEST_INTERFACE_HPP

#include "StarQuests.hpp"
#include "StarPane.hpp"

namespace Star {

STAR_CLASS(QuestManager);
STAR_CLASS(Player);
STAR_CLASS(Cinematic);
STAR_CLASS(UniverseClient);
STAR_CLASS(PaneManager);
STAR_CLASS(ItemBag);

class QuestLogInterface : public Pane {
public:
  QuestLogInterface(QuestManagerPtr manager, PlayerPtr player, CinematicPtr cinematic, UniverseClientPtr client);
  virtual ~QuestLogInterface() {}

  virtual void displayed() override;
  virtual void tick() override;
  virtual PanePtr createTooltip(Vec2I const& screenPosition) override;

  void fetchData();

  void pollDialog(PaneManager* paneManager);

private:
  WidgetPtr getSelected();
  void setSelected(WidgetPtr selected);
  void toggleTracking();
  void abandon();
  void showQuests(List<QuestPtr> quests);

  QuestManagerPtr m_manager;
  PlayerPtr m_player;
  CinematicPtr m_cinematic;
  UniverseClientPtr m_client;

  String m_trackLabel;
  String m_untrackLabel;

  ItemBagPtr m_rewardItems;
  int m_refreshRate;
  int m_refreshTimer;
};

class QuestPane : public Pane {
protected:
  QuestPane(QuestPtr const& quest, PlayerPtr player);

  void commonSetup(Json config, String bodyText, String const& portraitName);
  virtual void close();
  virtual void accept();
  virtual PanePtr createTooltip(Vec2I const& screenPosition) override;

  QuestPtr m_quest;
  PlayerPtr m_player;
};

class NewQuestInterface : public QuestPane {
public:
  NewQuestInterface(QuestManagerPtr const& manager, QuestPtr const& quest, PlayerPtr player);

protected:
  void close() override;
  void accept() override;
  void dismissed() override;

private:
  QuestManagerPtr m_manager;
  bool m_declined;
};

class QuestCompleteInterface : public QuestPane {
public:
  QuestCompleteInterface(QuestPtr const& quest, PlayerPtr player, CinematicPtr cinematic);

protected:
  void close() override;

private:
  PlayerPtr m_player;
  CinematicPtr m_cinematic;
};

class QuestFailedInterface : public QuestPane {
public:
  QuestFailedInterface(QuestPtr const& quest, PlayerPtr player);
};

}

#endif
