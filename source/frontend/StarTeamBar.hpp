#pragma once

#include "StarBaseScriptPane.hpp"
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
STAR_CLASS(PortraitWidget);

class TeamInvite : public BaseScriptPane {
public:
  TeamInvite(TeamBar* owner);

  virtual void show() override;

private:
  TeamBar* m_owner;

  void ok();
  void close();
};

class TeamInvitation : public BaseScriptPane {
public:
  TeamInvitation(TeamBar* owner);

  void open(Uuid const& inviterUuid, String const& inviterName);

  // A script gets "invited(inviterName, inviterUuid)" once the pane is displayed and can change anything on it.
  void displayed() override;

private:
  TeamBar* m_owner;
  Uuid m_inviterUuid;
  String m_inviterName;

  void ok();
  void close();
};

class TeamMemberMenu : public BaseScriptPane {
public:
  TeamMemberMenu(TeamBar* owner);

  void open(Uuid memberUuid, Vec2I position);

  virtual void update(float dt) override;

  // the script gets "opened(memberUuid, memberName)" once the menu is displayed and runs after update() each frame
  void displayed() override;
  void tick(float dt) override;

private:
  void updateWidgets();

  void close();
  void beamToShip();
  void makeLeader();
  void removeFromTeam();

  TeamBar* m_owner;
  Uuid m_memberUuid;
  String m_memberName;
  bool m_canBeam;
};

class TeamBar : public BaseScriptPane {
public:
  TeamBar(MainInterface* mainInterface, UniverseClientPtr client);

  bool sendEvent(InputEvent const& event) override;

  // A script runs after the native update every frame, so what it sets wins.
  void tick(float dt) override;

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

  TextStyle m_nameStyle;
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
  PortraitWidgetPtr m_portrait;

  friend class TeamMemberMenu;
  friend class TeamInvitation;
  friend class TeamInvite;
};

}
