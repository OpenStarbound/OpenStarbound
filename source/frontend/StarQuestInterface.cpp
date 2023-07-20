#include "StarQuestInterface.hpp"
#include "StarQuestManager.hpp"
#include "StarCinematic.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarPaneManager.hpp"
#include "StarListWidget.hpp"
#include "StarItemGridWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarAssets.hpp"
#include "StarItemDatabase.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarVerticalLayout.hpp"
#include "StarItemTooltip.hpp"

namespace Star {

QuestLogInterface::QuestLogInterface(QuestManagerPtr manager, PlayerPtr player, CinematicPtr cinematic, UniverseClientPtr client) {
  m_manager = manager;
  m_player = player;
  m_cinematic = cinematic;
  m_client = client;

  auto assets = Root::singleton().assets();
  auto config = assets->json("/interface/windowconfig/questlog.config");

  m_trackLabel = config.getString("trackLabel");
  m_untrackLabel = config.getString("untrackLabel");

  GuiReader reader;

  reader.registerCallback("close", [=](Widget*) { dismiss(); });
  reader.registerCallback("btnToggleTracking",
      [=](Widget*) {
        toggleTracking();
      });
  reader.registerCallback("btnAbandon",
      [=](Widget*) {
        abandon();
      });
  reader.registerCallback("filter",
      [=](Widget*) {
        fetchData();
      });

  reader.construct(config.get("paneLayout"), this);

  auto mainQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.mainQuestList");
  auto sideQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.sideQuestList");
  mainQuestList->setCallback([sideQuestList](Widget* widget) {
      auto listWidget = as<ListWidget>(widget);
      if (listWidget->selectedItem() != NPos)
        sideQuestList->clearSelected();
    });
  sideQuestList->setCallback([mainQuestList](Widget* widget) {
      auto listWidget = as<ListWidget>(widget);
      if (listWidget->selectedItem() != NPos)
        mainQuestList->clearSelected();
    });
  mainQuestList->disableScissoring();
  sideQuestList->disableScissoring();

  m_rewardItems = make_shared<ItemBag>(5);
  fetchChild<ItemGridWidget>("rewardItems")->setItemBag(m_rewardItems);

  m_refreshRate = 30;
  m_refreshTimer = 0;
}

void QuestLogInterface::pollDialog(PaneManager* paneManager) {
  if (paneManager->topPane({PaneLayer::ModalWindow}))
    return;

  if (auto failableQuest = m_manager->getFirstFailableQuest()) {
    auto qfi = make_shared<QuestFailedInterface>(failableQuest.value(), m_player);
    (*failableQuest)->setDialogShown();
    paneManager->displayPane(PaneLayer::ModalWindow, qfi);
  } else if (auto completableQuest = m_manager->getFirstCompletableQuest()) {
    auto qci = make_shared<QuestCompleteInterface>(completableQuest.value(), m_player, m_cinematic);
    (*completableQuest)->setDialogShown();
    paneManager->displayPane(PaneLayer::ModalWindow, qci);
  } else if (auto newQuest = m_manager->getFirstNewQuest()) {
    auto nqd = make_shared<NewQuestInterface>(m_manager, newQuest.value(), m_player);
    paneManager->displayPane(PaneLayer::ModalWindow, nqd);
  }
}

void QuestLogInterface::displayed() {
  Pane::displayed();
  tick(0);
  fetchData();
}

void QuestLogInterface::tick(float dt) {
  Pane::tick(dt);
  auto selected = getSelected();
  if (selected && m_manager->hasQuest(selected->data().toString())) {
    auto quest = m_manager->getQuest(selected->data().toString());

    m_manager->markAsRead(quest->questId());

    if (m_manager->isActive(quest->questId())) {
      fetchChild<ButtonWidget>("btnToggleTracking")->enable();
      fetchChild<ButtonWidget>("btnToggleTracking")->setText(m_manager->isCurrent(quest->questId()) ? m_untrackLabel : m_trackLabel);
      if (quest->canBeAbandoned())
        fetchChild<ButtonWidget>("btnAbandon")->enable();
      else
        fetchChild<ButtonWidget>("btnAbandon")->disable();
    } else {
      fetchChild<ButtonWidget>("btnToggleTracking")->disable();
      fetchChild<ButtonWidget>("btnToggleTracking")->setText(m_trackLabel);
      fetchChild<ButtonWidget>("btnAbandon")->disable();
    }
    fetchChild<LabelWidget>("lblQuestTitle")->setText(quest->title());
    fetchChild<LabelWidget>("lblQuestBody")->setText(quest->text());

    auto portraitName = "Objective";
    auto imagePortrait = quest->portrait(portraitName);
    if (imagePortrait) {
      auto portraitTitleLabel = fetchChild<LabelWidget>("lblPortraitTitle");
      String portraitTitle = quest->portraitTitle(portraitName).value("");
      Maybe<unsigned> charLimit = portraitTitleLabel->getTextCharLimit();
      portraitTitleLabel->setText(portraitTitle);

      Drawable::scaleAll(*imagePortrait, Vec2F(-1, 1));
      fetchChild<ImageWidget>("imgPortrait")->setDrawables(*imagePortrait);
      fetchChild<ImageWidget>("imgPolaroid")->setVisibility(true);
      fetchChild<ImageWidget>("imgPolaroidBack")->setVisibility(true);

    } else {
      fetchChild<LabelWidget>("lblPortraitTitle")->setText("");
      fetchChild<ImageWidget>("imgPortrait")->setDrawables({});
      fetchChild<ImageWidget>("imgPolaroid")->setVisibility(false);
      fetchChild<ImageWidget>("imgPolaroidBack")->setVisibility(false);
    }

    m_rewardItems->clearItems();
    if (quest->rewards().size() > 0) {
      fetchChild<LabelWidget>("lblRewards")->setVisibility(true);
      fetchChild<ItemGridWidget>("rewardItems")->setVisibility(true);
      for (auto const& reward : quest->rewards())
        m_rewardItems->addItems(reward->clone());
    } else {
      fetchChild<LabelWidget>("lblRewards")->setVisibility(false);
      fetchChild<ItemGridWidget>("rewardItems")->setVisibility(false);
    }
  } else {
    fetchChild<ButtonWidget>("btnToggleTracking")->disable();
    fetchChild<ButtonWidget>("btnToggleTracking")->setText(m_trackLabel);
    fetchChild<ButtonWidget>("btnAbandon")->disable();
    fetchChild<LabelWidget>("lblQuestTitle")->setText("");
    fetchChild<LabelWidget>("lblQuestBody")->setText("");
    fetchChild<LabelWidget>("lblPortraitTitle")->setText("");
    fetchChild<ImageWidget>("imgPortrait")->setDrawables({});
    fetchChild<LabelWidget>("lblRewards")->setVisibility(false);
    fetchChild<ItemGridWidget>("rewardItems")->setVisibility(false);
    fetchChild<ImageWidget>("imgPolaroid")->setVisibility(false);
    fetchChild<ImageWidget>("imgPolaroidBack")->setVisibility(false);
    m_rewardItems->clearItems();
  }

  m_refreshTimer--;
  if (m_refreshTimer < 0) {
    fetchData();
    m_refreshTimer = m_refreshRate;
  }
}

PanePtr QuestLogInterface::createTooltip(Vec2I const& screenPosition) {
  ItemPtr item;
  if (auto child = getChildAt(screenPosition)) {
    if (auto itemSlot = as<ItemSlotWidget>(child))
      item = itemSlot->item();
    if (auto itemGrid = as<ItemGridWidget>(child))
      item = itemGrid->itemAt(screenPosition);
  }
  if (item)
    return ItemTooltipBuilder::buildItemTooltip(item, m_player);
  return {};
}

WidgetPtr QuestLogInterface::getSelected() {
  auto mainQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.mainQuestList");
  if (auto selected = mainQuestList->selectedWidget())
    return selected;

  auto sideQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.sideQuestList");
  if (auto selected = sideQuestList->selectedWidget())
    return selected;

  return {};
}

void QuestLogInterface::setSelected(WidgetPtr selected) {
  auto mainQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.mainQuestList");
  auto mainQuestListPos = mainQuestList->itemPosition(selected);
  if (mainQuestListPos != NPos) {
    mainQuestList->setSelected(mainQuestListPos);
    return;
  }

  auto sideQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.sideQuestList");
  auto sideQuestListPos = sideQuestList->itemPosition(selected);
  if (sideQuestListPos != NPos) {
    sideQuestList->setSelected(sideQuestListPos);
    return;
  }
}

void QuestLogInterface::toggleTracking() {
  if (auto selected = getSelected()) {
    String questId = selected->data().toString();
    if (!m_manager->isCurrent(questId))
      m_manager->setAsTracked(questId);
    else
      m_manager->setAsTracked({});
  }
}

void QuestLogInterface::abandon() {
  if (auto selected = getSelected()) {
    m_manager->getQuest(selected->data().toString())->abandon();
  }
}

void QuestLogInterface::fetchData() {
  auto filter = fetchChild<ButtonGroupWidget>("filter")->checkedButton()->data().toString();
  if (filter.equalsIgnoreCase("inProgress"))
    showQuests(m_manager->listActiveQuests());
  else if (filter.equalsIgnoreCase("completed"))
    showQuests(m_manager->listCompletedQuests());
  else
    throw StarException(strf("Unknown quest filter '{}'", filter));
}

void QuestLogInterface::showQuests(List<QuestPtr> quests) {
  auto mainQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.mainQuestList");
  auto sideQuestList = fetchChild<ListWidget>("scrollArea.verticalLayout.sideQuestList");

  auto mainQuestHeader = fetchChild<Widget>("scrollArea.verticalLayout.mainQuestHeader");
  auto sideQuestHeader = fetchChild<Widget>("scrollArea.verticalLayout.sideQuestHeader");

  String selectedQuest;
  if (auto selected = getSelected())
    selectedQuest = selected->data().toString();
  else if (m_manager->currentQuest())
    selectedQuest = (*m_manager->currentQuest())->questId();

  mainQuestList->clear();
  mainQuestHeader->hide();
  sideQuestList->clear();
  sideQuestHeader->hide();
  for (auto const& quest : quests) {
    WidgetPtr entry;
    if (quest->mainQuest()) {
      entry = mainQuestList->addItem();
      mainQuestHeader->show();
    } else {
      entry = sideQuestList->addItem();
      sideQuestHeader->show();
    }

    entry->setData(quest->questId());
    entry->fetchChild<LabelWidget>("lblQuestEntry")->setText(quest->title());
    entry->fetchChild<ImageWidget>("imgNew")->setVisibility(quest->unread());
    entry->fetchChild<ImageWidget>("imgTracked")->setVisibility(m_manager->isCurrent(quest->questId()));

    bool currentWorld = false;
    if (auto questWorld = quest->worldId()) {
      currentWorld = m_client->playerWorld() == *questWorld;
    }
    entry->fetchChild<ImageWidget>("imgCurrent")->setVisibility(currentWorld);

    entry->fetchChild<ImageWidget>("imgPortrait")->setDrawables(quest->portrait("QuestStarted").value({}));
    entry->show();
    if (quest->questId() == selectedQuest)
      setSelected(entry);
  }

  auto verticalLayout = fetchChild<VerticalLayout>("scrollArea.verticalLayout");
  verticalLayout->update(0);
}

QuestPane::QuestPane(QuestPtr const& quest, PlayerPtr player) : Pane(), m_quest(quest), m_player(move(player)) {}

void QuestPane::commonSetup(Json config, String bodyText, String const& portraitName) {
  GuiReader reader;

  reader.registerCallback("close", [=](Widget*) { close(); });
  reader.registerCallback("btnDecline", [=](Widget*) { close(); });
  reader.registerCallback("btnAccept", [=](Widget*) { accept(); });
  reader.construct(config.get("paneLayout"), this);

  if (auto titleLabel = fetchChild<LabelWidget>("lblQuestTitle"))
    titleLabel->setText(m_quest->title());
  if (auto bodyLabel = fetchChild<LabelWidget>("lblQuestBody"))
    bodyLabel->setText(bodyText);

  if (auto portraitImage = fetchChild<ImageWidget>("portraitImage")) {
    Maybe<List<Drawable>> portrait = m_quest->portrait(portraitName);
    portraitImage->setDrawables(portrait.value({}));
  }

  if (auto portraitTitleLabel = fetchChild<LabelWidget>("portraitTitle")) {
    Maybe<String> portraitTitle = m_quest->portraitTitle(portraitName);
    String text = m_quest->portraitTitle(portraitName).value("");
    Maybe<unsigned> charLimit = portraitTitleLabel->getTextCharLimit();
    portraitTitleLabel->setText(text);
  }

  if (auto rewardItemsWidget = fetchChild<ItemGridWidget>("rewardItems")) {
    auto rewardItems = make_shared<ItemBag>(5);
    for (auto const& reward : m_quest->rewards())
      rewardItems->addItems(reward->clone());
    rewardItemsWidget->setItemBag(rewardItems);
  }

  auto sound = Random::randValueFrom(config.get("onShowSound").toArray(), "").toString();
  if (!sound.empty())
    context()->playAudio(sound);
}

void QuestPane::close() {
  dismiss();
}

void QuestPane::accept() {
  close();
}

PanePtr QuestPane::createTooltip(Vec2I const& screenPosition) {
  ItemPtr item;
  if (auto child = getChildAt(screenPosition)) {
    if (auto itemSlot = as<ItemSlotWidget>(child))
      item = itemSlot->item();
    if (auto itemGrid = as<ItemGridWidget>(child))
      item = itemGrid->itemAt(screenPosition);
  }
  if (item)
    return ItemTooltipBuilder::buildItemTooltip(item, m_player);
  return {};
}

NewQuestInterface::NewQuestInterface(QuestManagerPtr const& manager, QuestPtr const& quest, PlayerPtr player)
  : QuestPane(quest, move(player)), m_manager(manager), m_declined(false) {
  auto assets = Root::singleton().assets();

  List<Drawable> objectivePortrait = m_quest->portrait("Objective").value({});
  bool shortDialog = objectivePortrait.size() == 0;

  String configFile;
  if (shortDialog)
    configFile = m_quest->getTemplate()->newQuestGuiConfig.value(assets->json("/quests/quests.config:defaultGuiConfigs.newQuest").toString());
  else
    configFile = m_quest->getTemplate()->newQuestGuiConfig.value(assets->json("/quests/quests.config:defaultGuiConfigs.newQuestPortrait").toString());

  Json config = assets->json(configFile);

  commonSetup(config, m_quest->text(), "QuestStarted");

  m_declined = m_quest->canBeAbandoned();
  if (!m_declined) {
    if (auto declineButton = fetchChild<ButtonWidget>("btnDecline"))
      declineButton->disable();
  }

  if (!shortDialog) {
    if (auto objectivePortraitImage = fetchChild<ImageWidget>("objectivePortraitImage")) {
      Drawable::scaleAll(objectivePortrait, Vec2F(-1, 1));
      objectivePortraitImage->setDrawables(objectivePortrait);

      String objectivePortraitTitle = m_quest->portraitTitle("Objective").value("");
      auto portraitLabel = fetchChild<LabelWidget>("objectivePortraitTitle");
      portraitLabel->setText(objectivePortraitTitle);
      portraitLabel->setVisibility(objectivePortrait.size() > 0);

      fetchChild<ImageWidget>("imgPolaroid")->setVisibility(objectivePortrait.size() > 0);
      fetchChild<ImageWidget>("imgPolaroidBack")->setVisibility(objectivePortrait.size() > 0);
    }
  }

  if (auto rewardItemsWidget = fetchChild<ItemGridWidget>("rewardItems"))
    rewardItemsWidget->setVisibility(m_quest->rewards().size() > 0);
  if (auto rewardsLabel = fetchChild<LabelWidget>("lblRewards"))
    rewardsLabel->setVisibility(m_quest->rewards().size() > 0);
}

void NewQuestInterface::close() {
  m_declined = true;
  dismiss();
}

void NewQuestInterface::accept() {
  m_declined = false;
  dismiss();
}

void NewQuestInterface::dismissed() {
  QuestPane::dismissed();
  if (m_declined && m_quest->canBeAbandoned()) {
    m_manager->getQuest(m_quest->questId())->declineOffer();
  } else {
    m_manager->getQuest(m_quest->questId())->start();
  }
}

QuestCompleteInterface::QuestCompleteInterface(QuestPtr const& quest, PlayerPtr player, CinematicPtr cinematic)
  : QuestPane(quest, player) {
  auto assets = Root::singleton().assets();
  String configFile = m_quest->getTemplate()->questCompleteGuiConfig.value(assets->json("/quests/quests.config:defaultGuiConfigs.questComplete").toString());
  Json config = assets->json(configFile);

  m_player = player;
  m_cinematic = cinematic;

  commonSetup(config, m_quest->completionText(), "QuestComplete");

  if (auto moneyLabel = fetchChild<LabelWidget>("lblMoneyAmount"))
    moneyLabel->setText(toString(m_quest->money()));
  disableScissoring();
}

void QuestCompleteInterface::close() {
  auto assets = Root::singleton().assets();
  if (m_quest->completionCinema() && m_cinematic) {
    String cinema = m_quest->completionCinema()->replaceTags(
        StringMap<String>{{"species", m_player->species()}, {"gender", GenderNames.getRight(m_player->gender())}});
    m_cinematic->load(assets->fetchJson(cinema));
  }
  dismiss();
}

QuestFailedInterface::QuestFailedInterface(QuestPtr const& quest, PlayerPtr player) : QuestPane(quest, move(player)) {
  auto assets = Root::singleton().assets();
  String configFile = m_quest->getTemplate()->questFailedGuiConfig.value(assets->json("/quests/quests.config:defaultGuiConfigs.questFailed").toString());
  Json config = assets->json(configFile);
  commonSetup(config, m_quest->failureText(), "QuestFailed");
  disableScissoring();
}

}
