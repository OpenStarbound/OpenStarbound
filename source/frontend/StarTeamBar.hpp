#ifndef STAR_TEAMBAR_HPP
#define STAR_TEAMBAR_HPP

#include "StarPane.hpp"
#include "StarUuid.hpp"
#include "StarMainInterfaceTypes.hpp"
#include "StarProgressWidget.hpp"
#include "StarLabelWidget.hpp"

namespace Star {

STAR_CLASS(TeamBar);
STAR_CLASS(MainInterface);
STAR_CLASS(UniverseClient);
STAR_CLASS(Player);

STAR_CLASS(TeamInvite);
STAR_CLASS(TeamInvitation);
STAR_CLASS(TeamMemberMenu);
STAR_CLASS(TeamBar);

class TeamInvite : public Pane {
public:
  TeamInvite(TeamBar* owner);

  virtual void show() override;

private:
  TeamBar* m_owner;

  void ok();
  void close();
};

class TeamInvitation : public Pane {
public:
  TeamInvitation(TeamBar* owner);

  void open(Uuid const& inviterUuid, String const& inviterName);

private:
  TeamBar* m_owner;
  Uuid m_inviterUuid;

  void ok();
  void close();
};

class TeamMemberMenu : public Pane {
public:
  TeamMemberMenu(TeamBar* owner);

  void open(Uuid memberUuid, Vec2I position);

  virtual void update(float dt) override;

private:
  void updateWidgets();

  void close();
  void beamToShip();
  void makeLeader();
  void removeFromTeam();

  TeamBar* m_owner;
  Uuid m_memberUuid;
  bool m_canBeam;
};

class TeamBar : public Pane {
public:
  TeamBar(MainInterface* mainInterface, UniverseClientPtr client);

  bool sendEvent(InputEvent const& event) override;

  void invitePlayer(String const& playerName);
  void acceptInvitation(Uuid const& inviterUuid);

protected:
  virtual void update(float dt) override;

private:
  void updatePlayerResources();

  void inviteButton();

  void buildTeamBar();

  void showMemberMenu(Uuid memberUuid, Vec2I position);

  MainInterface* m_mainInterface;
  UniverseClientPtr m_client;

  GuiContext* m_guiContext;

  int m_nameFontSize;
  Vec2F m_nameOffset;

  TeamInvitePtr m_teamInvite;
  TeamInvitationPtr m_teamInvitation;
  TeamMemberMenuPtr m_teamMemberMenu;

  ProgressWidgetPtr m_healthBar;
  ProgressWidgetPtr m_energyBar;
  ProgressWidgetPtr m_foodBar;

  LabelWidgetPtr m_nameLabel;

  Color m_energyBarColor;
  Color m_energyBarRegenMixColor;
  Color m_energyBarUnusableColor;

  friend class TeamMemberMenu;
};

}

#endif
