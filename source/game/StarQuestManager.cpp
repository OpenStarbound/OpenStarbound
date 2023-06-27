#include "StarQuestManager.hpp"
#include "StarRoot.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarAssets.hpp"
#include "StarItemDatabase.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"
#include "StarTime.hpp"
#include "StarClientContext.hpp"
#include "StarUniverseClient.hpp"

namespace Star {

QuestManager::QuestManager(Player* player) {
  m_player = player;
  m_world = nullptr;
  m_trackOnWorldQuests = false;
}

QuestTemplatePtr getTemplate(String const& templateId) {
  return Root::singleton().questTemplateDatabase()->questTemplate(templateId);
}

StringMap<QuestPtr> readQuests(Json const& json) {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto questTemplateDatabase = Root::singleton().questTemplateDatabase();

  auto validateArc = [questTemplateDatabase](QuestArcDescriptor const& arc) {
      for (auto quest : arc.quests) {
        if (!questTemplateDatabase->questTemplate(quest.templateId))
          return false;
      }
      return true;
    };

  StringMap<QuestPtr> result;
  for (auto const& questPair : json.iterateObject()) {
    // don't load the quest unless all quests in the arc exist
    Json diskStore = versioningDatabase->loadVersionedJson(VersionedJson::fromJson(questPair.second), "Quest");
    auto questArc = QuestArcDescriptor::diskLoad(diskStore.get("arc"));
    if (validateArc(questArc))
      result[questPair.first] = make_shared<Quest>(questPair.second);
  }
  return result;
}

function<bool (QuestPtr const&)> questFilter(QuestState state) {
  return [state](QuestPtr const& quest) {
      return quest->state() == state;
    };
}

void QuestManager::diskLoad(Json const& quests) {
  m_quests = readQuests(quests.get("quests", JsonObject{}));
  m_trackedQuestId = quests.optString("currentQuest");
}

Json QuestManager::diskStore() {
  auto questPtrToJson = [](QuestPtr const& quest) { return quest->diskStore(); };

  auto quests = m_quests.values();

  return JsonObject{
      {"quests", jsonFromMapV<StringMap<QuestPtr>>(m_quests, questPtrToJson)},
      {"currentQuest", jsonFromMaybe(m_trackedQuestId)}
    };
}

void QuestManager::setUniverseClient(UniverseClient* client) {
  m_client = client;
}

void QuestManager::init(World* world) {
  m_world = world;
  serverQuests().exec([this, world](QuestPtr const& quest) { quest->init(m_player, world, m_client); });
  m_trackOnWorldQuests = true;

  // untrack tracked quest if it's not cross-server, and we're on a different server
  if (auto tracked = m_trackedQuestId) {
    if (auto maybeQuest = m_quests.maybe(*tracked)) {
      auto quest = *maybeQuest;
      if (quest->hideCrossServer() && quest->serverUuid() && *quest->serverUuid() != m_player->clientContext()->serverUuid())
        m_trackedQuestId = {};
    }
  }
}

void QuestManager::uninit() {
  m_quests.values().exec([](QuestPtr const& quest) { quest->uninit(); });
  m_world = nullptr;
}

bool QuestManager::canStart(QuestArcDescriptor const& questArc) const {
  if (questArc.quests.size() == 0)
    return false;

  for (auto questDesc : questArc.quests) {
    auto questTemplate = getTemplate(questDesc.templateId);
    if (!questTemplate)
      return false;
    if (auto quest = m_quests.maybe(questDesc.questId))
      if ((*quest)->state() != QuestState::Failed)
        return false;
    for (auto const& prerequisiteQuestId : questTemplate->prerequisiteQuests) {
      if (!hasCompleted(prerequisiteQuestId))
        return false;
    }
    for (auto item : questTemplate->requiredItems)
      if (!m_player->inventory()->hasItem(item))
        return false;
    if (questTemplate->requiredShipLevel) {
      if (m_player->clientContext()->shipUpgrades().shipLevel < *questTemplate->requiredShipLevel)
        return false;
    }
  }

  return true;
}

void QuestManager::offer(QuestPtr const& quest) {
  m_quests[quest->questId()] = quest;
  quest->init(m_player, m_world, m_client);
  quest->offer();
}

QuestPtr QuestManager::getQuest(String const& questId) const {
  return m_quests.get(questId);
}

bool QuestManager::hasQuest(String const& questId) const {
  return m_quests.contains(questId);
}

bool QuestManager::hasAcceptedQuest(String const& questId) const {
  return m_quests.contains(questId) && m_quests.get(questId)->state() != QuestState::New && m_quests.get(questId)->state() != QuestState::Offer;
}

bool QuestManager::isActive(String const& questId) const {
  return m_quests.contains(questId) && m_quests.get(questId)->state() == QuestState::Active;
}

bool QuestManager::isCurrent(String const& questId) const {
  return m_onWorldQuestId.orMaybe(m_trackedQuestId) == questId;
}

bool QuestManager::isTracked(String const& questId) const {
  return m_trackedQuestId == questId;
}

void QuestManager::setAsTracked(Maybe<String> const& questId) {
  if (questId && isActive(*questId)) {
    m_trackedQuestId = questId;
    if (m_onWorldQuestId) {
      // stop auto tracking quests on this world
      m_onWorldQuestId = {};
      m_trackOnWorldQuests = false;
    }
  } else {
    m_trackedQuestId = {};
    m_trackOnWorldQuests = true;
  }
}

void QuestManager::markAsRead(String const& questId) {
  if (!m_quests.contains(questId))
    return;

  getQuest(questId)->markAsRead();
}

bool QuestManager::hasCompleted(String const& questId) const {
  if (auto quest = m_quests.maybe(questId))
    if ((*quest)->state() == QuestState::Complete)
      return true;
  return false;
}

bool QuestManager::canTurnIn(String const& questId) const {
  if (auto quest = m_quests.maybe(questId))
    if ((*quest)->state() == QuestState::Active && (*quest)->canTurnIn())
      return true;
  return false;
}

Maybe<QuestPtr> QuestManager::getFirstNewQuest() {
  for (auto& q : serverQuests()) {
    if (q->state() == QuestState::Offer)
      return q;
  }
  return {};
}

Maybe<QuestPtr> QuestManager::getFirstCompletableQuest() {
  for (auto& q : serverQuests()) {
    if (q->state() == QuestState::Complete && q->showDialog())
      return q;
  }
  return {};
}

Maybe<QuestPtr> QuestManager::getFirstFailableQuest() {
  for (auto& q : serverQuests()) {
    if (q->state() == QuestState::Failed&& q->showDialog())
      return q;
  }
  return {};
}

Maybe<QuestPtr> QuestManager::getFirstMainQuest() {
  for (auto& q : serverQuests()) {
    if (q->state() == QuestState::Active && q->mainQuest())
      return q;
  }
  return {};
}

void sortQuests(List<QuestPtr>& quests) {
  std::sort(quests.begin(),
      quests.end(),
      [](QuestPtr const& left, QuestPtr const& right) -> bool {
        int64_t leftUpdated = left->lastUpdatedOn();
        int64_t rightUpdated = right->lastUpdatedOn();
        String leftQuestId = left->templateId();
        String rightQuestId = right->templateId();
        return std::tie(leftUpdated, leftQuestId) < std::tie(rightUpdated, rightQuestId);
      });
}

List<QuestPtr> QuestManager::listActiveQuests() const {
  List<QuestPtr> result = serverQuests();
  result.filter([&](QuestPtr quest) {
      return quest->state() == QuestState::Active && quest->showInLog();
    });
  sortQuests(result);
  return result;
}

List<QuestPtr> QuestManager::listCompletedQuests() const {
  List<QuestPtr> result = serverQuests();
  result.filter([](QuestPtr quest) {
      return quest->state() == QuestState::Complete && quest->showInLog();
    });
  sortQuests(result);
  return result;
}

List<QuestPtr> QuestManager::listFailedQuests() const {
  List<QuestPtr> result = serverQuests();
  result.filter([](QuestPtr quest) {
      return quest->state() == QuestState::Failed && quest->showInLog();
    });
  sortQuests(result);
  return result;
}

Maybe<QuestPtr> QuestManager::currentQuest() const {
  auto questId = m_onWorldQuestId.orMaybe(m_trackedQuestId);
  if (questId && isActive(*questId)) {
    auto current = getQuest(*questId);
    if (current->showInLog())
      return current;
  }
  return {};
}

Maybe<QuestPtr> QuestManager::trackedQuest() const {
  if (m_trackedQuestId && isActive(*m_trackedQuestId)) {
    auto current = getQuest(*m_trackedQuestId);
    if (current->showInLog())
      return current;
  }
  return {};
}

Maybe<QuestIndicator> QuestManager::getQuestIndicator(EntityPtr const& entity) const {
  Maybe<String> indicatorType;
  Vec2F indicatorPos = entity->position() + Vec2F(0, 2.75);

  if (auto questGiver = as<InteractiveEntity>(entity)) {
    indicatorPos = questGiver->questIndicatorPosition();

    if (!indicatorType) {
      for (auto questId : questGiver->turnInQuests()) {
        if (!isActive(questId))
          continue;
        auto quest = getQuest(questId);
        if (quest->canTurnIn()) {
          indicatorType = quest->questReceiverIndicator();
          break;
        }
      }
    }

    if (!indicatorType) {
      auto questTemplateDatabase = Root::singleton().questTemplateDatabase();
      for (auto questArc : questGiver->offeredQuests()) {
        if (canStart(questArc) && questArc.quests.size() > 0) {
          auto questDesc = questArc.quests[0];
          auto questTemplate = questTemplateDatabase->questTemplate(questDesc.templateId);
          indicatorType = questTemplate->questGiverIndicator;
          break;
        }
      }
    }
  }

  if (indicatorType) {
    Json indicators = Root::singleton().assets()->json("/quests/quests.config:indicators");
    String indicatorImage = indicators.get(*indicatorType).getString("image");
    return QuestIndicator{indicatorImage, indicatorPos};
  }

  for (auto& pair : m_quests) {
    if (pair.second->state() == QuestState::Active) {
      if (auto indicatorImage = pair.second->customIndicator(entity))
        return QuestIndicator{ *indicatorImage, indicatorPos };
    }
  }

  return {};
}

StringSet QuestManager::interestingObjects() {
  StringSet result;
  m_quests.values().exec([&result](QuestPtr const& quest) {
    if (auto questObjects = quest->receiveMessage("interestingObjects", true, JsonArray()))
      result.addAll(jsonToStringSet(*questObjects));
  });
  return result;
}

Maybe<Json> QuestManager::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  starAssert(m_world);
  Maybe<Json> result;
  m_quests.values().exec([&result, message, localMessage, args](
      QuestPtr const& quest) { result = result.orMaybe(quest->receiveMessage(message, localMessage, args)); });
  return result;
}

void QuestManager::update() {
  startInitialQuests();

  if (m_trackedQuestId && !isActive(*m_trackedQuestId))
    m_trackedQuestId = {};

  if (m_onWorldQuestId) {
    auto active = isActive(*m_onWorldQuestId);
    if (active) {
      auto worldId = getQuest(*m_onWorldQuestId)->worldId();
      if (!worldId || m_player->clientContext()->playerWorldId() != *worldId)
        active = false;
    }
    if (!active)
      m_onWorldQuestId = {};
  } else if (m_trackOnWorldQuests) {
    auto playerWorldId = m_client->clientContext()->playerWorldId();
    auto trackedWorld = currentQuest().sequence([](QuestPtr q) { return q->worldId(); });
    if (!trackedWorld || playerWorldId != *trackedWorld) {
      // the currently tracked quest is not on this world, track another quest on this world
      for (auto quest : listActiveQuests()) {
        if (auto questWorld = quest->worldId()) {
          if (playerWorldId == *questWorld)
            m_onWorldQuestId = quest->questId();  
        }
      }
    }
  }

  for (auto& entry : m_quests) {
    auto quest = entry.second;
    QuestState state = quest->state();
    bool finished = state == QuestState::Complete || state == QuestState::Failed;
    if (state == QuestState::New || (finished && quest->ephemeral() && !quest->showDialog())) {
      quest->uninit();
      m_quests.remove(entry.first);
    }
  }

  serverQuests().exec([](QuestPtr const& quest) { quest->update(); });
}

List<QuestPtr> QuestManager::serverQuests() const {
  return m_quests.values().filtered([this](QuestPtr const& q) -> bool {
      if (q->hideCrossServer() && q->serverUuid().isValid() && *q->serverUuid() != m_player->clientContext()->serverUuid())
        return false;
      return true;
    });
}

void QuestManager::startInitialQuests() {
  auto startingQuests =
      Root::singleton().assets()->json(strf("/quests/quests.config:initialquests.{}", m_player->species())).toArray();
  for (auto const& questArcJson : startingQuests) {
    QuestArcDescriptor quest = QuestArcDescriptor::fromJson(questArcJson);
    if (canStart(quest))
      offer(make_shared<Quest>(quest, 0, m_player));
  }
}

void QuestManager::setMostRecentQuestCurrent() {
  List<QuestPtr> sortedActiveQuests = listActiveQuests();
  if (sortedActiveQuests.size() > 0)
    setAsTracked(sortedActiveQuests.last()->questId());
}

}
