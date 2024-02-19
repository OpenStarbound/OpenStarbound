#include "StarAiInterface.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarJsonRpc.hpp"
#include "StarAssets.hpp"
#include "StarContainerEntity.hpp"
#include "StarItemBag.hpp"
#include "StarItemDatabase.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerCompanions.hpp"
#include "StarPlayerInventory.hpp"
#include "StarQuests.hpp"
#include "StarQuestManager.hpp"
#include "StarRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarPlayerStorage.hpp"
#include "StarClientContext.hpp"
#include "StarCanvasWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarImageStretchWidget.hpp"
#include "StarGuiReader.hpp"
#include "StarListWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarOrderedSet.hpp"
#include "StarAiDatabase.hpp"
#include "StarTabSet.hpp"
#include "StarPlayerTech.hpp"
#include "StarPlayerBlueprints.hpp"
#include "StarItemSlotWidget.hpp"
#include "StarStackWidget.hpp"
#include "StarCinematic.hpp"
#include "StarWorldClient.hpp"

namespace Star {

AiInterface::AiInterface(UniverseClientPtr client, CinematicPtr cinematic, MainInterfacePaneManager* paneManager) {
  m_client = client;
  m_cinematic = cinematic;
  m_paneManager = paneManager;

  m_textLength = 0.0;
  m_textMaxLength = 0.0;

  m_aiDatabase = Root::singleton().aiDatabase();
  auto assets = Root::singleton().assets();

  GuiReader reader;
  reader.registerCallback("close", bind(mem_fn(&AiInterface::dismiss), this));
  reader.registerCallback("missionItemList", bind(mem_fn(&AiInterface::selectMission), this));
  reader.registerCallback("startMission", bind(mem_fn(&AiInterface::startMission), this));
  reader.registerCallback("crewItemList", bind(mem_fn(&AiInterface::selectRecruit), this));
  reader.registerCallback("dismissRecruit", bind(mem_fn(&AiInterface::dismissRecruit), this));
  reader.registerCallback("showMissions", bind(mem_fn(&AiInterface::showMissions), this));
  reader.registerCallback("showCrew", bind(mem_fn(&AiInterface::showCrew), this));
  reader.registerCallback("goBack", bind(mem_fn(&AiInterface::goBack), this));

  reader.construct(assets->json("/interface/ai/ai.config:guiConfig"), this);

  m_mainStack = fetchChild<StackWidget>("mainStack");
  m_missionStack = findChild<StackWidget>("missionStack");
  m_crewStack = findChild<StackWidget>("crewStack");

  m_breadcrumbLeftPadding = assets->json("/interface/ai/ai.config:breadcrumbLeftPadding").toInt();
  m_breadcrumbRightPadding = assets->json("/interface/ai/ai.config:breadcrumbRightPadding").toInt();
  m_homeBreadcrumbBackground = fetchChild<ImageStretchWidget>("homeBreadcrumbBg");
  m_pageBreadcrumbBackground = fetchChild<ImageStretchWidget>("pageBreadcrumbBg");
  m_itemBreadcrumbBackground = fetchChild<ImageStretchWidget>("itemBreadcrumbBg");

  m_homeBreadcrumbWidget = fetchChild<LabelWidget>("homeBreadcrumb");
  m_pageBreadcrumbWidget = fetchChild<LabelWidget>("pageBreadcrumb");
  m_itemBreadcrumbWidget = fetchChild<LabelWidget>("itemBreadcrumb");

  m_aiFaceCanvasWidget = findChild<CanvasWidget>("aiFaceCanvas");
  m_missionListWidget = findChild<ListWidget>("missionItemList");
  m_crewListWidget = findChild<ListWidget>("crewItemList");

  m_showMissionsButton = findChild<ButtonWidget>("showMissions");
  m_showCrewButton = findChild<ButtonWidget>("showCrew");
  m_backButton = findChild<ButtonWidget>("backButton");

  m_missionNameLabel = findChild<LabelWidget>("missionName");
  m_missionIcon = findChild<ImageWidget>("missionIcon");
  m_startMissionButton = findChild<ButtonWidget>("startMission");

  m_recruitNameLabel = findChild<LabelWidget>("recruitName");
  m_recruitIcon = findChild<ImageWidget>("recruitIcon");
  m_dismissRecruitButton = findChild<ButtonWidget>("dismissRecruit");

  m_species = m_client->mainPlayer()->species();
  m_staticAnimation = m_aiDatabase->staticAnimation(m_species);
  m_scanlineAnimation = m_aiDatabase->scanlineAnimation();

  m_missionBreadcrumbText = assets->json("/interface/ai/ai.config:missionBreadcrumbText").toString();
  m_missionDeployText = assets->json("/interface/ai/ai.config:missionDeployText").toString();
  m_crewBreadcrumbText = assets->json("/interface/ai/ai.config:crewBreadcrumbText").toString();
  m_defaultRecruitName = assets->json("/interface/ai/ai.config:defaultRecruitName").toString();
  m_defaultRecruitDescription = assets->json("/interface/ai/ai.config:defaultRecruitDescription").toString();
}

void AiInterface::update(float dt) {
  if (!m_client->playerOnOwnShip())
    dismiss();

  Pane::update(dt);

  m_showCrewButton->setVisibility(m_currentPage == AiPages::StatusPage);
  m_showMissionsButton->setVisibility(m_currentPage == AiPages::StatusPage);
  m_backButton->setVisibility(m_currentPage != AiPages::StatusPage);

  m_staticAnimation.update(dt);
  m_scanlineAnimation.update(dt);

  if (m_currentSpeech) {
    m_textLength += m_currentSpeech->speedModifier * m_aiDatabase->charactersPerSecond() * dt;
    m_currentTextWidget->setText(m_currentSpeech->text);
    m_currentTextWidget->setTextCharLimit(min(m_textMaxLength, floor(m_textLength)));

    if (m_textLength < m_textMaxLength) {
      setFaceAnimation(m_currentSpeech->animation);
      if (!m_chatterSound || m_chatterSound->finished()) {
        auto assets = Root::singleton().assets();
        m_chatterSound = make_shared<AudioInstance>(*assets->audio(assets->json("/interface/ai/ai.config:chatterSound").toString()));
        m_chatterSound->setLoops(-1);
        GuiContext::singleton().playAudio(m_chatterSound);
      }
    } else {
      setFaceAnimation("idle");
      if (m_chatterSound)
        m_chatterSound->stop();
    }
    m_faceAnimation.second.update(dt * m_currentSpeech->speedModifier);
  } else {
    setFaceAnimation("idle");
    m_faceAnimation.second.update(dt);
    if (m_chatterSound)
      m_chatterSound->stop();
  }

  // If the enabled missions list changes, update the mission list
  auto aiState = m_client->mainPlayer()->aiState();
  if (aiState.availableMissions.values() != m_availableMissions
      || aiState.completedMissions.values() != m_completedMissions) {
    m_availableMissions = aiState.availableMissions.values();
    m_completedMissions = aiState.completedMissions.values();
    populateMissions();
  }

  auto crew = m_client->mainPlayer()->companions()->getCompanions("crew");
  if (crew != m_crew) {
    m_crew = crew;
    populateCrew();
  }

  m_aiFaceCanvasWidget->clear();
  m_aiFaceCanvasWidget->drawDrawable(m_faceAnimation.second.drawable(1.0f), Vec2F(0, 0));
  m_aiFaceCanvasWidget->drawDrawable(m_staticAnimation.drawable(1.0f), Vec2F(0, 0));
  m_aiFaceCanvasWidget->drawDrawable(m_scanlineAnimation.drawable(1.0f), Vec2F(0, 0));
}

void AiInterface::updateBreadcrumbs() {
  // Home breadcrumb
  auto width = m_homeBreadcrumbWidget->size()[0];
  width += m_breadcrumbLeftPadding + m_breadcrumbRightPadding;
  m_homeBreadcrumbBackground->setSize(Vec2I(width, m_homeBreadcrumbBackground->size()[1]));

  // Middle breadcrumb
  if (m_currentPage != AiPages::StatusPage) {
    Vec2I pagePosition = m_homeBreadcrumbBackground->position() + Vec2I(width, 0);
    m_pageBreadcrumbWidget->setPosition(Vec2I(pagePosition[0] + m_breadcrumbLeftPadding, m_homeBreadcrumbWidget->position()[1]));
    m_pageBreadcrumbBackground->setPosition(pagePosition - Vec2I(m_breadcrumbRightPadding, 0));
    width = m_pageBreadcrumbWidget->size()[0] + (2 * m_breadcrumbRightPadding) + m_breadcrumbLeftPadding; // Add right padding twice because of the overlap to the left
    m_pageBreadcrumbBackground->setSize(Vec2I(width, m_pageBreadcrumbBackground->size()[1]));

    // End breadcrumb
    if (m_currentPage == AiPages::MissionPage || m_currentPage == AiPages::CrewPage) {
      Vec2I itemPosition = m_pageBreadcrumbBackground->position() + Vec2I(width, 0);
      m_itemBreadcrumbWidget->setPosition(Vec2I(itemPosition[0] + m_breadcrumbLeftPadding, m_homeBreadcrumbWidget->position()[1]));
      m_itemBreadcrumbBackground->setPosition(itemPosition - Vec2I(m_breadcrumbRightPadding, 0));
      width = m_itemBreadcrumbWidget->size()[0] + (2 * m_breadcrumbRightPadding) + m_breadcrumbLeftPadding;
      m_itemBreadcrumbBackground->setSize(Vec2I(width, m_itemBreadcrumbBackground->size()[1]));
    }
  }

  // Visibility
  m_pageBreadcrumbBackground->setVisibility(m_currentPage != AiPages::StatusPage);
  m_pageBreadcrumbWidget->setVisibility(m_currentPage != AiPages::StatusPage);
  m_itemBreadcrumbBackground->setVisibility(m_currentPage == AiPages::MissionPage || m_currentPage == AiPages::CrewPage);
  m_itemBreadcrumbWidget->setVisibility(m_currentPage == AiPages::MissionPage || m_currentPage == AiPages::CrewPage);
}

void AiInterface::displayed() {
  if (!m_client->playerOnOwnShip())
    return;

  Pane::displayed();

  showStatus();
}

void AiInterface::dismissed() {
  if (m_chatterSound)
    m_chatterSound->stop();

  m_selectedMission = {};
  m_selectedRecruit = {};

  Pane::dismissed();
}

void AiInterface::setSourceEntityId(EntityId sourceEntityId) {
  m_sourceEntityId = sourceEntityId;
}

void AiInterface::showStatus() {
  m_currentPage = AiPages::StatusPage;
  setCurrentSpeech("shipStatusText", m_aiDatabase->shipStatus(m_client->clientContext()->shipUpgrades().shipLevel));
  m_mainStack->showPage(0);

  updateBreadcrumbs();
}

void AiInterface::showMissions() {
  m_currentPage = AiPages::MissionList;
  m_currentSpeech = {};
  m_pageBreadcrumbWidget->setText(m_missionBreadcrumbText);

  populateMissions();

  m_missionListWidget->clearSelected();
  m_mainStack->showPage(1);
  m_missionStack->showPage(0);
  if (m_availableMissions.empty() && m_completedMissions.empty()) {
    m_missionStack->showPage(0);
    setCurrentSpeech("noMissionsText", m_aiDatabase->noMissionsSpeech());
  } else {
    m_missionStack->showPage(1);
  }

  updateBreadcrumbs();
}

void AiInterface::selectMission() {
  size_t selectedItem = m_missionListWidget->selectedItem();

  Maybe<String> mission = {};
  if (selectedItem < m_availableMissions.size())
    mission = m_availableMissions.at(selectedItem);
  else if (selectedItem < m_availableMissions.size() + m_completedMissions.size())
    mission = m_completedMissions.at(selectedItem - m_availableMissions.size());

  m_selectedMission = mission;

  if (m_selectedMission) {
    m_currentPage = AiPages::MissionPage;

    auto mission = m_aiDatabase->mission(*m_selectedMission);
    auto missionText = mission.speciesText.value(m_species, mission.speciesText.value("default"));
    setCurrentSpeech("missionText", missionText.selectSpeech);
    m_missionNameLabel->setText(missionText.buttonText);
    m_missionIcon->setImage(mission.icon);

    m_itemBreadcrumbWidget->setText(m_missionDeployText);

    m_missionStack->showPage(2);

    updateBreadcrumbs();
  }
}

void AiInterface::showCrew() {
  m_currentPage = AiPages::CrewList;
  m_currentSpeech = {};
  m_pageBreadcrumbWidget->setText(m_crewBreadcrumbText);

  populateMissions();

  m_crewListWidget->clearSelected();
  m_mainStack->showPage(2);
  if (m_crew.empty()) {
    m_crewStack->showPage(0);
    setCurrentSpeech("noCrewText", m_aiDatabase->noCrewSpeech());
  } else {
    m_crewStack->showPage(1);
  }

  updateBreadcrumbs();
}

void AiInterface::selectRecruit() {
  CompanionPtr recruit = {};
  size_t selectedItem = m_crewListWidget->selectedItem();
  if (selectedItem < m_crew.size())
    recruit = m_crew.at(selectedItem);

  m_selectedRecruit = recruit;

  if (m_selectedRecruit) {
    m_currentPage = AiPages::CrewPage;

    auto speech = m_selectedRecruit->description().value(m_defaultRecruitDescription);
    setCurrentSpeech("recruitText", {m_aiDatabase->defaultAnimation(), speech, 1.0f});
    m_recruitNameLabel->setText(m_selectedRecruit->name().value(m_defaultRecruitName));
    m_recruitIcon->setDrawables(m_selectedRecruit->portrait());

    m_itemBreadcrumbWidget->setText(recruit->name().value(m_defaultRecruitName));

    m_crewStack->showPage(2);

    updateBreadcrumbs();
  }
}

void AiInterface::populateMissions() {
  m_missionListWidget->clear();
  for (auto const& missionName : m_availableMissions) {
    auto widget = m_missionListWidget->addItem();
    auto label = widget->fetchChild<LabelWidget>("itemName");
    auto icon = widget->fetchChild<ImageWidget>("itemIcon");

    auto const& mission = m_aiDatabase->mission(missionName);
    icon->setImage(mission.icon);
    auto const& missionText = mission.speciesText.value(m_species, mission.speciesText.value("default"));
    label->setText(missionText.buttonText);
  }
  for (auto const& missionName : m_completedMissions) {
    auto widget = m_missionListWidget->addItem();
    auto label = widget->fetchChild<LabelWidget>("itemName");
    auto icon = widget->fetchChild<ImageWidget>("itemIcon");

    auto const& mission = m_aiDatabase->mission(missionName);
    icon->setImage(mission.icon);
    auto const& missionText = mission.speciesText.value(m_species, mission.speciesText.value("default"));
    label->setText(missionText.repeatButtonText);
  }
  m_missionListWidget->setSelected(NPos);
}

void AiInterface::startMission() {
  if (m_selectedMission) {
    auto const& mission = m_aiDatabase->mission(*m_selectedMission);
    m_client->warpPlayer(
      WarpToWorld{InstanceWorldId(mission.missionUniqueWorld, m_client->teamUuid()), {}},
      true,
      mission.warpAnimation.value("default"),
      mission.warpDeploy.value(false));
    dismiss();
  }
}

void AiInterface::populateCrew() {
  m_crewListWidget->clear();
  for (auto const& recruit : m_crew) {
    auto widget = m_crewListWidget->addItem();
    auto label = widget->fetchChild<LabelWidget>("itemName");
    auto icon = widget->fetchChild<ImageWidget>("itemIcon");

    icon->setDrawables(recruit->portrait());
    label->setText(recruit->name().value(m_defaultRecruitName));
  }
  m_crewListWidget->setSelected(NPos);
}

void AiInterface::dismissRecruit() {
  if (!m_selectedRecruit)
    return;

  Uuid podUuid = m_selectedRecruit->podUuid();
  m_client->mainPlayer()->companions()->dismissCompanion("crew", podUuid);

  m_crewStack->showPage(1);
}

void AiInterface::goBack() {
  if (m_currentPage == AiPages::MissionPage)
    showMissions();
  else if (m_currentPage == AiPages::CrewPage)
    showCrew();
  else if (m_currentPage == AiPages::MissionList || m_currentPage == AiPages::CrewList)
    showStatus();

  updateBreadcrumbs();
}

void AiInterface::setFaceAnimation(String const& name) {
  if (m_faceAnimation.first != name)
    m_faceAnimation = {name, m_aiDatabase->animation(m_species, name)};
}

void AiInterface::setCurrentSpeech(String const& textWidget, AiSpeech speech) {;
  m_currentSpeech = std::move(speech);
  m_textLength = 0.0;
  m_textMaxLength = Text::stripEscapeCodes(m_currentSpeech->text).size();
  m_currentTextWidget = findChild<LabelWidget>(textWidget);
  m_currentTextWidget->setText("");
}

void AiInterface::giveBlueprint(String const& blueprintName) {
  m_client->mainPlayer()->addBlueprint(ItemDescriptor(blueprintName));
}

}
