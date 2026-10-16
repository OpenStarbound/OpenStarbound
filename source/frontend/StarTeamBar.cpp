#include "StarTeamBar.hpp"
#include "StarMainInterface.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarGuiReader.hpp"
#include "StarButtonWidget.hpp"
#include "StarTeamClient.hpp"
#include "StarImageWidget.hpp"
#include "StarProgressWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"
#include "StarWorldClient.hpp"
#include "StarPortraitWidget.hpp"
#include "StarMathCommon.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarCelestialLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

static void addGameCallbacks(LuaBaseComponent& script, UniverseClientPtr const& client) {
  script.setLuaRoot(client->luaRoot());
  script.addCallbacks("player", LuaBindings::makePlayerCallbacks(client->mainPlayer().get()));
  script.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(client->mainPlayer()->statusController()));
  script.addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(client.get()));
}

TeamBar::TeamBar(MainInterface* mainInterface, UniverseClientPtr client)
  : BaseScriptPane(BaseScriptPane::windowConfig("/interface/windowconfig/teambar.config", false), false) {
  m_mainInterface = mainInterface;
  m_client = client;
  addGameCallbacks(m_script, m_client);

  m_guiContext = GuiContext::singletonPtr();
  auto assets = Root::singleton().assets();

  m_teamInvite = make_shared<TeamInvite>(this);
  m_teamInvitation = make_shared<TeamInvitation>(this);
  m_teamMemberMenu = make_shared<TeamMemberMenu>(this);

  m_nameStyle = assets->json("/interface.config:teamBarNameStyle");
  m_nameStyle.fontSize = assets->json("/interface.config:font.nameSize").toInt();
  m_nameOffset = jsonToVec2F(assets->json("/interface.config:nameOffset"));

  m_reader->registerCallback("inviteButton", [this](Widget*) { inviteButton(); });
  m_reader->registerCallback("showSelfMenu", [this](Widget*) {
      if (!m_client->teamClient()->isMemberOfTeam())
        return;
      auto position = jsonToVec2I(Root::singleton().assets()->json("/interface/windowconfig/teambar.config:selfMenuOffset"));
      position[1] += windowHeight() / m_guiContext->interfaceScale();
      showMemberMenu(m_client->mainPlayer()->clientContext()->clientUuid(), position);
    });

  construct(m_config.get("gui"));

  m_healthBar = fetchChild<ProgressWidget>("healthBar");
  m_energyBar = fetchChild<ProgressWidget>("energyBar");
  m_foodBar = fetchChild<ProgressWidget>("foodBar");
  m_nameLabel = fetchChild<LabelWidget>("name");

  m_energyBarColor = jsonToColor(assets->json("/interface/windowconfig/teambar.config:energyBarColor"));
  m_energyBarRegenMixColor = jsonToColor(assets->json("/interface/windowconfig/teambar.config:energyBarRegenMixColor"));
  m_energyBarUnusableColor = jsonToColor(assets->json("/interface/windowconfig/teambar.config:energyBarUnusableColor"));

  m_energyBar->setColor(m_energyBarColor);

  m_portrait = fetchChild<PortraitWidget>("portrait");
  m_portrait->setEntity(as<PortraitEntity>(m_client->mainPlayer()));

  // "teamBar" script callbacks: the identity portrait can show another entity (defaults to the player) or another mode
  LuaCallbacks teamBarCallbacks;
  teamBarCallbacks.registerCallback("setPortraitEntity", [this](Maybe<EntityId> entityId) {
      PortraitEntityPtr entity;
      if (!entityId)
        entity = as<PortraitEntity>(m_client->mainPlayer());
      else if (auto world = m_client->worldClient())
        entity = as<PortraitEntity>(world->entity(*entityId));
      m_portrait->setEntity(entity);
    });
  teamBarCallbacks.registerCallback("setPortraitMode", [this](String const& mode) {
      m_portrait->setMode(PortraitModeNames.getLeft(mode));
    });
  teamBarCallbacks.registerCallback("setPortraitScale", [this](float scale) {
      m_portrait->setScale(scale);
    });
  m_script.addCallbacks("teamBar", std::move(teamBarCallbacks));

  updatePlayerResources();
  disableScissoring();
}

bool TeamBar::sendEvent(InputEvent const& event) {
  if (event.is<MouseButtonDownEvent>()
      && (event.get<MouseButtonDownEvent>().mouseButton == MouseButton::Left || event.get<MouseButtonDownEvent>().mouseButton == MouseButton::Right)) {
    if (m_teamMemberMenu->isDisplayed() && !m_teamMemberMenu->inMember(*context()->mousePosition(event)))
      m_teamMemberMenu->dismiss();
  }
  return Pane::sendEvent(event);
}

void TeamBar::invitePlayer(String const& playerName) {
  m_client->teamClient()->invitePlayer(playerName);
}

void TeamBar::acceptInvitation(Uuid const& inviterUuid) {
  m_client->teamClient()->acceptInvitation(inviterUuid);
}

void TeamBar::update(float dt) {
  Pane::update(dt);

  updatePlayerResources();

  auto teamClient = m_client->teamClient();

  if (!m_teamInvitation->active()) {
    if (teamClient->hasInvitationPending()) {
      auto invitation = teamClient->pullInvitation();
      m_teamInvitation->open(invitation.first, invitation.second);
      if (!m_teamInvitation->isDisplayed())
        m_mainInterface->paneManager()->displayPane(PaneLayer::Window, m_teamInvitation);
    }
  }

  if (teamClient->currentTeam() && !teamClient->isTeamLeader())
    m_teamInvite->dismiss();

  fetchChild<ImageWidget>("leader")->setVisibility(teamClient->isTeamLeader());

  buildTeamBar();

  BaseScriptPane::tick(dt);
}

void TeamBar::tick(float dt) {
  // the script is updated from update(), after the native update, so that what it sets wins
  Pane::tick(dt);
}

void TeamBar::updatePlayerResources() {
  auto player = m_client->mainPlayer();

  m_healthBar->setCurrentProgressLevel(player->healthPercentage());
  m_energyBar->setCurrentProgressLevel(player->energyPercentage());

  if (player->modeConfig().hunger) {
    m_foodBar->setCurrentProgressLevel(player->foodPercentage());
    auto assets = Root::singleton().assets();
    if (player->foodPercentage() <= assets->json("/player.config:foodLowThreshold").toFloat()) {
      float flashTime = assets->json("/interface/windowconfig/teambar.config:foodBarFlashTime").toFloat();
      if (fmod(Time::monotonicTime(), flashTime * 2) < flashTime)
        m_foodBar->setOverlay(assets->json("/interface/windowconfig/teambar.config:foodBarFlashOverlay").toString());
      else
        m_foodBar->setOverlay("");
    } else {
      m_foodBar->setOverlay("");
    }
    m_foodBar->show();
  } else {
    m_foodBar->hide();
  }

  if (player->energyLocked()) {
    m_energyBar->setColor(m_energyBarUnusableColor);
  } else {
    m_energyBar->setColor(m_energyBarColor.mix(m_energyBarRegenMixColor, player->energyRegenBlockPercent()));
  }

  m_nameLabel->setText(player->name());
}

void TeamBar::inviteButton() {
  if (!m_teamInvite->isDisplayed())
    m_mainInterface->paneManager()->displayPane(PaneLayer::Window, m_teamInvite);
}

void TeamBar::buildTeamBar() {
  auto teamClient = m_client->teamClient();
  auto player = m_client->mainPlayer();

  auto list = fetchChild("list");
  auto assets = Root::singleton().assets();

  Vec2I offset;
  size_t controlIndex = 0;

  float portraitScale = assets->json("/interface/windowconfig/teambar.config:memberPortraitScale").toFloat();
  int memberSize = assets->json("/interface/windowconfig/teambar.config:memberSize").toInt();
  int memberSpacing = assets->json("/interface/windowconfig/teambar.config:memberSpacing").toInt();

  Uuid myUuid = player->clientContext()->clientUuid();
  for (auto member : teamClient->members()) {
    if (member.uuid == myUuid) {
      continue;
    }

    String cellName = toString(controlIndex);
    WidgetPtr cell = list->fetchChild(cellName);

    if (!cell) {
      GuiReader reader;
      cell = make_shared<Widget>();
      cell->disableScissoring();
      cell->markAsContainer();

      reader.registerCallback("showMemberMenu", [this](Widget* widget) {
          auto position = widget->screenPosition() + jsonToVec2I(Root::singleton().assets()->json("/interface/windowconfig/teambar.config:memberMenuOffset"));
          showMemberMenu(Uuid(widget->parent()->data().toString()), position);
        });

      reader.construct(assets->json("/interface/windowconfig/teambar.config:entry"), cell.get());

      list->addChild(cellName, cell);
    }

    offset[1] -= memberSize;
    cell->setPosition(offset);

    cell->setData(member.uuid.hex());

    cell->show();

    if (!teamClient->isTeamLeader(member.uuid))
      cell->fetchChild<ImageWidget>("leader")->hide();
    else
      cell->fetchChild<ImageWidget>("leader")->show();

    List<Drawable> drawables = member.portrait;
    Drawable::scaleAll(drawables, portraitScale);
    cell->fetchChild<ImageWidget>("portrait")->setDrawables(std::move(drawables));

    if (member.world == m_client->playerWorld() && m_client->worldClient()) {
      auto mpos = member.position;
      if (auto entity = m_client->worldClient()->entity(member.entity))
        mpos = entity->position();
      auto direction = m_client->worldClient()->geometry().diff(mpos, player->position());

      auto compassImage = cell->fetchChild<ImageWidget>("compass");
      compassImage->setRotation(direction.angle() - Constants::pi / 2.0f);
      compassImage->show();
      cell->fetchChild<ImageWidget>("compassoffworld")->hide();
    } else {
      cell->fetchChild<ImageWidget>("compass")->hide();
      cell->fetchChild<ImageWidget>("compassoffworld")->show();
    }

    cell->fetchChild<ProgressWidget>("healthBar")->setCurrentProgressLevel(member.healthPercentage);
    cell->fetchChild<ProgressWidget>("energyBar")->setCurrentProgressLevel(member.energyPercentage);

    offset[1] -= memberSpacing;
    controlIndex++;
  }

  auto inviteButton = fetchChild<ButtonWidget>("inviteButton");
  auto noInviteImage = fetchChild<ImageWidget>("noInviteImage");

  Vec2I inviteOffset = list->position() + offset;
  inviteButton->setPosition(inviteOffset - Vec2I{0, inviteButton->size()[1]});
  noInviteImage->setPosition(inviteOffset - Vec2I{0, noInviteImage->size()[1]});

  bool couldInvite = (!teamClient->currentTeam() || teamClient->isTeamLeader())
      && m_client->teamClient()->members().size() < Root::singleton().configuration()->get("maxTeamSize").toUInt();
  inviteButton->setVisibility(couldInvite);
  inviteButton->setEnabled(!m_teamInvitation->active());
  noInviteImage->setVisibility(!couldInvite);

  while (true) {
    String cellName = toString(controlIndex);
    WidgetPtr cell = list->fetchChild(cellName);
    if (!cell)
      break;
    cell->hide();
    controlIndex++;
  }
}

void TeamBar::showMemberMenu(Uuid memberUuid, Vec2I position) {
  m_teamMemberMenu->open(memberUuid, position);
  if (!m_teamMemberMenu->isDisplayed())
    m_mainInterface->paneManager()->displayPane(PaneLayer::Window, m_teamMemberMenu);
}

TeamInvite::TeamInvite(TeamBar* owner)
  : BaseScriptPane(BaseScriptPane::windowConfig("/interface/windowconfig/teaminvite.config"), false) {
  m_owner = owner;
  addGameCallbacks(m_script, m_owner->m_client);

  m_reader->registerCallback("ok", [this](Widget*) { ok(); });
  m_reader->registerCallback("close", [this](Widget*) { close(); });
  m_reader->registerCallback("name", [](Widget*) {});
  construct(m_config.get("gui"));
  dismiss();
}

void TeamInvite::show() {
  Pane::show();
  fetchChild<TextBoxWidget>("name")->setText("", false);
  fetchChild<TextBoxWidget>("name")->focus();
}

void TeamInvite::ok() {
  String name = fetchChild<TextBoxWidget>("name")->getText();

  // a script may change the name that is invited or return false to stop the invitation
  if (auto result = m_script.invoke<Json>("invite", name)) {
    if (result->type() == Json::Type::Bool && !result->toBool())
      return;
    if (result->type() == Json::Type::String)
      name = result->toString();
  }

  m_owner->invitePlayer(name);
  dismiss();
}

void TeamInvite::close() {
  dismiss();
}

TeamInvitation::TeamInvitation(TeamBar* owner)
  : BaseScriptPane(BaseScriptPane::windowConfig("/interface/windowconfig/teaminvitation.config"), false) {
  m_owner = owner;
  addGameCallbacks(m_script, m_owner->m_client);

  m_reader->registerCallback("ok", [this](Widget*) { ok(); });
  m_reader->registerCallback("close", [this](Widget*) { close(); });

  construct(m_config.get("gui"));
  dismiss();
}

void TeamInvitation::open(Uuid const& inviterUuid, String const& inviterName) {
  if (active())
    return;
  m_inviterUuid = inviterUuid;
  m_inviterName = inviterName;
  fetchChild<LabelWidget>("inviterName")->setText(inviterName);
  Pane::show();
}

void TeamInvitation::displayed() {
  BaseScriptPane::displayed();
  m_script.invoke("invited", m_inviterName, m_inviterUuid.hex());
}

void TeamInvitation::ok() {
  m_owner->acceptInvitation(m_inviterUuid);
  dismiss();
}

void TeamInvitation::close() {
  dismiss();
}

TeamMemberMenu::TeamMemberMenu(TeamBar* owner)
  : BaseScriptPane(BaseScriptPane::windowConfig("/interface/windowconfig/teammembermenu.config"), false) {
  m_owner = owner;
  addGameCallbacks(m_script, m_owner->m_client);

  m_reader->registerCallback("beamToShip", [this](Widget*) { beamToShip(); });
  m_reader->registerCallback("close", [this](Widget*) { close(); });
  m_reader->registerCallback("makeLeader", [this](Widget*) { makeLeader(); });
  m_reader->registerCallback("removeFromTeam", [this](Widget*) { removeFromTeam(); });
  construct(m_config.get("gui"));
}

void TeamMemberMenu::open(Uuid memberUuid, Vec2I position) {
  if (active())
    return;

  auto assets = Root::singleton().assets();

  setPosition(position);

  m_memberUuid = memberUuid;
  auto members = m_owner->m_client->teamClient()->members();
  for (auto member : members) {
    if (member.uuid == m_memberUuid) {
      m_memberName = member.name;
      fetchChild<LabelWidget>("name")->setText(member.name);
      break;
    }
  }

  updateWidgets();

  Pane::show();
}

void TeamMemberMenu::update(float dt) {
  auto stillValid = false;
  auto members = m_owner->m_client->teamClient()->members();
  for (auto member : members) {
    if (member.uuid == m_memberUuid) {
      stillValid = true;
      m_canBeam = member.warpMode != WarpMode::None && m_owner->m_client->canBeamToTeamShip();
    }
  }

  if (!stillValid) {
    close();
    return;
  }

  updateWidgets();

  Pane::update(dt);

  BaseScriptPane::tick(dt);
}

void TeamMemberMenu::tick(float dt) {
  // the script is updated from update(), after the native update
  Pane::tick(dt);
}

void TeamMemberMenu::displayed() {
  BaseScriptPane::displayed();
  m_script.invoke("opened", m_memberUuid.hex(), m_memberName);
}

void TeamMemberMenu::updateWidgets() {
  bool isLeader = m_owner->m_client->teamClient()->isTeamLeader();
  bool isSelf = m_owner->m_client->mainPlayer()->clientContext()->clientUuid() == m_memberUuid;

  fetchChild<ButtonWidget>("beamToShip")->setEnabled(m_canBeam);
  fetchChild<ButtonWidget>("makeLeader")->setEnabled(isLeader && !isSelf);
  fetchChild<ButtonWidget>("removeFromTeam")->setEnabled(isLeader || isSelf);

  auto assets = Root::singleton().assets();
  if (isSelf)
    fetchChild<ButtonWidget>("removeFromTeam")->setText(assets->json("/interface/windowconfig/teammembermenu.config:removeSelfText").toString());
  else
    fetchChild<ButtonWidget>("removeFromTeam")->setText(assets->json("/interface/windowconfig/teammembermenu.config:removeOtherText").toString());
}

void TeamMemberMenu::beamToShip() {
  if (m_canBeam)
    m_owner->m_mainInterface->warpTo(WarpToWorld{ClientShipWorldId(m_memberUuid), {}});
  dismiss();
}

void TeamMemberMenu::close() {
  dismiss();
}

void TeamMemberMenu::makeLeader() {
  m_owner->m_client->teamClient()->makeLeader(m_memberUuid);
  dismiss();
}

void TeamMemberMenu::removeFromTeam() {
  m_owner->m_client->teamClient()->removeFromTeam(m_memberUuid);
  dismiss();
}

}
