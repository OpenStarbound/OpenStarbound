#include "StarQuests.hpp"
#include "StarJsonExtra.hpp"
#include "StarFile.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarTime.hpp"
#include "StarRandom.hpp"
#include "StarItemDatabase.hpp"
#include "StarItemDrop.hpp"
#include "StarMonster.hpp"
#include "StarNpc.hpp"
#include "StarObjectDatabase.hpp"
#include "StarObject.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerTech.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarQuestManager.hpp"
#include "StarClientContext.hpp"
#include "StarUuid.hpp"
#include "StarCelestialLuaBindings.hpp"

namespace Star {

EnumMap<QuestState> const QuestStateNames {
  {QuestState::New, "New"},
  {QuestState::Offer, "Offer"},
  {QuestState::Active, "Active"},
  {QuestState::Complete, "Complete"},
  {QuestState::Failed, "Failed"}
};

Quest::Quest(QuestArcDescriptor const& questArc, size_t arcPos, Player* player) {
  m_trackedIndicator = Root::singleton().assets()->json("/quests/quests.config:trackedCustomIndicator").toString();
  m_untrackedIndicator = Root::singleton().assets()->json("/quests/quests.config:untrackedCustomIndicator").toString();

  m_money = 0;
  m_unread = true;
  m_canTurnIn = false;
  m_lastUpdatedOn = Time::monotonicMilliseconds();

  m_arc = questArc;
  m_arcPos = arcPos;

  auto itemDatabase = Root::singleton().itemDatabase();
  auto templateDatabase = Root::singleton().questTemplateDatabase();
  auto questTemplate = templateDatabase->questTemplate(templateId());

  m_parameters = questDescriptor().parameters;
  m_displayParameters = DisplayParameters {
     questTemplate->ephemeral,
     questTemplate->showInLog,
     questTemplate->showAcceptDialog,
     questTemplate->showCompleteDialog,
     questTemplate->showFailDialog,
     questTemplate->mainQuest,
     questTemplate->hideCrossServer
  };
  setEntityParameter("player", player);

  m_money = Random::randUInt(questTemplate->moneyRange[0], questTemplate->moneyRange[1]);
  m_rewards = Random::randValueFrom(questTemplate->rewards, {}).transformed(
      [&itemDatabase](ItemDescriptor const& item) -> ItemConstPtr { return itemDatabase->item(item); });

  for (String const& rewardParamName : questTemplate->rewardParameters) {
    if (!m_parameters.contains(rewardParamName))
      continue;
    QuestParam const& rewardParam = m_parameters[rewardParamName];
    if (rewardParam.detail.is<QuestItem>()) {
      addReward(rewardParam.detail.get<QuestItem>().descriptor());
    } else {
      if (!rewardParam.detail.is<QuestItemList>())
        throw StarException::format("Quest parameter %s cannot be used as a reward parameter", rewardParamName);
      for (auto item : rewardParam.detail.get<QuestItemList>())
        addReward(item);
    }
  }

  m_title = questTemplate->title;
  m_text = questTemplate->text;
  m_completionText = questTemplate->completionText;
  m_failureText = questTemplate->failureText;

  m_state = QuestState::New;
  m_showDialog = false;

  m_player = nullptr;
  m_world = nullptr;
  m_inited = false;
}

Quest::Quest(Json const& spec) {
  m_trackedIndicator = Root::singleton().assets()->json("/quests/quests.config:trackedCustomIndicator").toString();
  m_untrackedIndicator = Root::singleton().assets()->json("/quests/quests.config:untrackedCustomIndicator").toString();

  auto versioningDatabase = Root::singleton().versioningDatabase();
  Json diskStore = versioningDatabase->loadVersionedJson(VersionedJson::fromJson(spec), "Quest");

  m_state = QuestStateNames.getLeft(diskStore.getString("state"));

  m_arc = QuestArcDescriptor::diskLoad(diskStore.get("arc"));
  m_arcPos = diskStore.getUInt("arcPos");
  m_parameters = questParamsDiskLoad(diskStore.get("parameters"));
  m_worldId = diskStore.optString("worldId").apply(parseWorldId);
  m_location = diskStore.opt("location").apply([](Json const& json) {
    return make_pair(jsonToVec3I(json.get("system")), jsonToSystemLocation(json.get("location")));
  });
  m_serverUuid = diskStore.optString("serverUuid").apply(construct<Uuid>());
  m_money = diskStore.getUInt("money");

  auto itemDatabase = Root::singleton().itemDatabase();
  m_rewards = diskStore.getArray("rewards").transformed(
      [&itemDatabase](Json const& json) -> ItemConstPtr { return itemDatabase->diskLoad(json); });

  m_lastUpdatedOn = diskStore.getInt("lastUpdatedOn");
  m_unread = diskStore.getBool("unread", true);
  m_canTurnIn = diskStore.getBool("canTurnIn", false);
  m_indicators = jsonToStringSet(diskStore.get("indicators", JsonArray{}));
  m_scriptComponent.setScriptStorage(diskStore.getObject("scriptStorage", JsonObject{}));

  auto templateDatabase = Root::singleton().questTemplateDatabase();
  auto questTemplate = templateDatabase->questTemplate(templateId());
  m_displayParameters = DisplayParameters{
    questTemplate->ephemeral,
    questTemplate->showInLog,
    questTemplate->showAcceptDialog,
    questTemplate->showCompleteDialog,
    questTemplate->showFailDialog,
    questTemplate->mainQuest,
    questTemplate->hideCrossServer
  };

  m_title = diskStore.getString("title", questTemplate->title);
  m_text = diskStore.getString("text", questTemplate->text);
  m_completionText = diskStore.getString("completionText", questTemplate->completionText);
  m_failureText = diskStore.getString("failureText", questTemplate->failureText);

  m_portraits = jsonToMapV<StringMap<List<Drawable>>>(diskStore.get("portraits", JsonObject{}), [](Json const& portrait) {
      return portrait.toArray().transformed(construct<Drawable>());
    });
  m_portraitTitles = jsonToMapV<StringMap<String>>(diskStore.get("portraitTitles", JsonObject{}), mem_fn(&Json::toString));
  m_showDialog = diskStore.getBool("showDialog", false);

  m_player = nullptr;
  m_world = nullptr;
  m_inited = false;
}

Json Quest::diskStore() const {
  auto versioningDatabase = Root::singleton().versioningDatabase();

  JsonObject result;
  result["state"] = QuestStateNames.getRight(m_state);
  result["arc"] = m_arc.diskStore();
  result["arcPos"] = m_arcPos;
  result["parameters"] = questParamsDiskStore(m_parameters);
  result["money"] = m_money;

  result["worldId"] = jsonFromMaybe(m_worldId.apply(printWorldId));
  result["location"] = jsonFromMaybe(m_location.apply([](pair<Vec3I, SystemLocation> const& location) {
      return JsonObject{{"system", jsonFromVec3I(location.first)}, {"location", jsonFromSystemLocation(location.second)}};
    }));
  result["serverUuid"] = jsonFromMaybe(m_serverUuid.apply(mem_fn(&Uuid::hex)));

  auto itemDatabase = Root::singleton().itemDatabase();
  result["rewards"] =
      m_rewards.transformed([&itemDatabase](ItemConstPtr const& item) { return itemDatabase->diskStore(item); });

  result["lastUpdatedOn"] = m_lastUpdatedOn;
  result["unread"] = m_unread;
  result["canTurnIn"] = m_canTurnIn;
  result["indicators"] = jsonFromStringSet(m_indicators);
  result["scriptStorage"] = m_scriptComponent.getScriptStorage();

  result["title"] = m_title;
  result["text"] = m_text;
  result["completionText"] = m_completionText;
  result["failureText"] = m_failureText;

  result["portraits"] = jsonFromMapV(m_portraits, [](List<Drawable> const& portrait) {
      return portrait.transformed(mem_fn(&Drawable::toJson));
    });
  result["portraitTitles"] = jsonFromMap(m_portraitTitles);
  result["showDialog"] = m_showDialog;

  return versioningDatabase->makeCurrentVersionedJson("Quest", result).toJson();
}

QuestTemplatePtr Quest::getTemplate() const {
  return Root::singleton().questTemplateDatabase()->questTemplate(templateId());
}

void Quest::init(Player* player, World* world, UniverseClient* client) {
  m_player = player;
  m_world = world;
  m_client = client;

  if (m_state == QuestState::Offer || m_state == QuestState::Active)
    initScript();
}

void Quest::uninit() {
  if (m_inited)
    uninitScript();

  m_player = nullptr;
  m_world = nullptr;
}

Maybe<Json> Quest::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  if (!m_inited)
    return {};
  return m_scriptComponent.handleMessage(message, localMessage, args);
}

void Quest::update() {
  if (!m_inited)
    return;
  m_scriptComponent.update(m_scriptComponent.updateDt());
}

void Quest::offer() {
  starAssert(m_player && m_world);

  if (!showAcceptDialog()) {
    start();
  } else {
    setState(QuestState::Offer);
    initScript();
    m_scriptComponent.invoke("questOffer");
  }
}

void Quest::declineOffer() {
  setState(QuestState::New);
  m_scriptComponent.invoke("questDecline");
  uninitScript();
}

void Quest::start() {
  setState(QuestState::Active);
  initScript();

  auto current = m_player->questManager()->trackedQuest();
  if (mainQuest() || !current)
    m_player->questManager()->setAsTracked(questId());

  m_scriptComponent.invoke("questStart");
}

void Quest::complete(Maybe<size_t> followupIndex) {
  setState(QuestState::Complete);
  m_showDialog = showCompleteDialog();
  m_scriptComponent.invoke("questComplete");
  uninitScript();

  // Grant reward items and money
  for (auto const& item : rewards())
    m_player->giveItem(item->clone());
  m_player->inventory()->addCurrency("money", money());

  // Offer follow-up quests
  bool trackNewQuest = m_player->questManager()->isTracked(questId());
  size_t nextArcPos = followupIndex.value(questArcPosition() + 1);
  if (nextArcPos < m_arc.quests.size()) {
    auto followUp = make_shared<Quest>(m_arc, nextArcPos, m_player);
    followUp->setWorldId(worldId());
    followUp->setLocation(location());
    followUp->setServerUuid(serverUuid());
    m_player->questManager()->offer(followUp);
    if (trackNewQuest)
      m_player->questManager()->setAsTracked(followUp->questId());
  } else if (trackNewQuest) {
    // no followup, track another main quest or clear quest tracker
    if (auto main = m_player->questManager()->getFirstMainQuest())
      m_player->questManager()->setAsTracked(main.value()->questId());
    else
      m_player->questManager()->setAsTracked({});
  }
}

void Quest::fail() {
  setState(QuestState::Failed);
  m_showDialog = showFailDialog();
  m_scriptComponent.invoke("questFail", false);
  uninitScript();
}

void Quest::abandon() {
  setState(QuestState::Failed);
  m_showDialog = false;
  m_scriptComponent.invoke("questFail", true);
  uninitScript();
}

bool Quest::interactWithEntity(EntityId entity) {
  Maybe<bool> result = m_scriptComponent.invoke<bool>("questInteract", entity);
  return result && result.value();
}

String Quest::questId() const {
  return questDescriptor().questId;
}

String Quest::templateId() const {
  return questDescriptor().templateId;
}

StringMap<QuestParam> const& Quest::parameters() const {
  return m_parameters;
}

QuestState Quest::state() const {
  return m_state;
}

bool Quest::showDialog() const {
  return m_showDialog;
}

void Quest::setDialogShown() {
  m_showDialog = false;
}

void Quest::setEntityParameter(String const& paramName, EntityConstPtr const& entity) {
  setEntityParameter(paramName, entity.get());
}

void Quest::setParameter(String const& paramName, QuestParam const& paramValue) {
  m_parameters[paramName] = paramValue;
}

Maybe<List<Drawable>> Quest::portrait(String const& portraitName) const {
  return m_portraits.maybe(portraitName);
}

Maybe<String> Quest::portraitTitle(String const& portraitName) const {
  return m_portraitTitles.maybe(portraitName);
}

QuestDescriptor Quest::questDescriptor() const {
  return m_arc.quests[m_arcPos];
}

QuestArcDescriptor Quest::questArcDescriptor() const {
  return m_arc;
}

size_t Quest::questArcPosition() const {
  return m_arcPos;
}

Maybe<WorldId> Quest::worldId() const {
  return m_worldId;
}

Maybe<pair<Vec3I, SystemLocation>> Quest::location() const {
  return m_location;
}

Maybe<Uuid> Quest::serverUuid() const {
  return m_serverUuid;
}

void Quest::setWorldId(Maybe<WorldId> worldId) {
  m_worldId = worldId;
}

void Quest::setLocation(Maybe<pair<Vec3I, SystemLocation>> location) {
  m_location = move(location);
}

void Quest::setServerUuid(Maybe<Uuid> serverUuid) {
  m_serverUuid = serverUuid;
}

String Quest::title() const {
  return m_title;
}

String Quest::text() const {
  return m_text;
}

String Quest::completionText() const {
  return m_completionText;
}

String Quest::failureText() const {
  return m_failureText;
}

size_t Quest::money() const {
  return m_money;
}

List<ItemConstPtr> Quest::rewards() const {
  return m_rewards;
}

int64_t Quest::lastUpdatedOn() const {
  return m_lastUpdatedOn;
}

bool Quest::unread() const {
  return m_unread;
}

void Quest::markAsRead() {
  m_unread = false;
}

bool Quest::canTurnIn() const {
  return m_canTurnIn;
}

String Quest::questGiverIndicator() const {
  return getTemplate()->questGiverIndicator;
}

String Quest::questReceiverIndicator() const {
  return getTemplate()->questReceiverIndicator;
}

bool hasItemIndicator(EntityPtr const& entity, List<ItemDescriptor> indicatedItems) {
  if (auto itemDrop = as<ItemDrop>(entity)) {
    for (auto const& itemDesc : indicatedItems) {
      if (itemDrop->item()->matches(itemDesc, true)) {
        return true;
      }
    }
  } else if (auto object = as<Object>(entity)) {
    ObjectConfigPtr objectConfig = Root::singleton().objectDatabase()->getConfig(object->name());
    if (!objectConfig->hasObjectItem)
      return false;
    for (auto const& itemDesc : indicatedItems) {
      if (object->name() == itemDesc.name())
        return true;
    }
  }
  return false;
}

Maybe<String> Quest::customIndicator(EntityPtr const& entity) const {
  if (!m_inited)
    return {};

  for (String const& indicator : m_indicators) {
    auto param = parameters().get(indicator);
    if (param.detail.is<QuestEntity>()) {
      QuestEntity questEntity = param.detail.get<QuestEntity>();
      if (questEntity.uniqueId && entity->uniqueId() == questEntity.uniqueId) {
        return param.indicator.value(defaultCustomIndicator());
      }

    } else if (param.detail.is<QuestItem>()) {
      QuestItem questItem = param.detail.get<QuestItem>();
      if (hasItemIndicator(entity, {questItem.descriptor()}))
        return param.indicator.value(defaultCustomIndicator());

    } else if (param.detail.is<QuestItemTag>()) {
      String questItemTag = param.detail.get<QuestItemTag>();
      if (auto itemDrop = as<ItemDrop>(entity)) {
        if (itemDrop->item()->itemTags().contains(questItemTag))
          return param.indicator.value(defaultCustomIndicator());
      }

    } else if (param.detail.is<QuestItemList>()) {
      QuestItemList questItemList = param.detail.get<QuestItemList>();
      if (hasItemIndicator(entity, questItemList))
        return param.indicator.value(defaultCustomIndicator());

    } else if (param.detail.is<QuestMonsterType>()) {
      QuestMonsterType questMonsterType = param.detail.get<QuestMonsterType>();
      if (auto monster = as<Monster>(entity)) {
        if (monster->typeName() == questMonsterType.typeName) {
          TeamType team = monster->getTeam().type;
          if (team == TeamType::Enemy || team == TeamType::Passive)
            return param.indicator.value(defaultCustomIndicator());
        }
      }

    }
  }
  return {};
}

Maybe<JsonArray> Quest::objectiveList() const {
  return m_objectiveList;
}

Maybe<float> Quest::progress() const {
  return m_progress;
}

Maybe<float> Quest::compassDirection() const {
  return m_compassDirection;
}

void Quest::setObjectiveList(Maybe<JsonArray> const& objectiveList) {
  m_objectiveList = objectiveList;
}

void Quest::setProgress(Maybe<float> const& progress) {
  m_progress = progress;
}

void Quest::setCompassDirection(Maybe<float> const& compassDirection) {
  m_compassDirection = compassDirection;
}

Maybe<String> Quest::completionCinema() const {
  return getTemplate()->completionCinema;
}

bool Quest::canBeAbandoned() const {
  return getTemplate()->canBeAbandoned;
}

bool Quest::ephemeral() const {
  return m_displayParameters.ephemeral;
}

bool Quest::showInLog() const {
  return m_displayParameters.showInLog;
}

bool Quest::showAcceptDialog() const {
  return m_displayParameters.showAcceptDialog;
}

bool Quest::showCompleteDialog() const {
  return m_displayParameters.showCompleteDialog;
}

bool Quest::showFailDialog() const {
  return m_displayParameters.showFailDialog;
}

bool Quest::mainQuest() const {
  return m_displayParameters.mainQuest;
}

bool Quest::hideCrossServer() const {
  return m_displayParameters.hideCrossServer;
}

void Quest::setState(QuestState state) {
  m_state = state;
  m_lastUpdatedOn = Time::monotonicMilliseconds();
}

void Quest::initScript() {
  if (!m_player || !m_world || m_inited)
    return;

  auto questTemplate = getTemplate();
  if (questTemplate->script.isValid())
    m_scriptComponent.setScript(*questTemplate->script);
  else
    m_scriptComponent.setScripts(StringList{});
  m_scriptComponent.setUpdateDelta(questTemplate->updateDelta);

  m_scriptComponent.addCallbacks("quest", makeQuestCallbacks(m_player));
  m_scriptComponent.addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(m_client));
  m_scriptComponent.addCallbacks("player", LuaBindings::makePlayerCallbacks(m_player));
  m_scriptComponent.addCallbacks("config", LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def) {
      return Json(getTemplate()->scriptConfig).query(name, def);
    }));
  m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(m_player));
  m_scriptComponent.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_player->statusController()));
  m_scriptComponent.addActorMovementCallbacks(m_player->movementController());
  m_inited = true;

  m_scriptComponent.init(m_world);
}

void Quest::uninitScript() {
  m_scriptComponent.uninit();
  m_scriptComponent.removeCallbacks("quest");
  m_scriptComponent.removeCallbacks("celestial");
  m_scriptComponent.removeCallbacks("player");
  m_scriptComponent.removeCallbacks("config");
  m_scriptComponent.removeCallbacks("entity");
  m_scriptComponent.removeCallbacks("status");
  m_scriptComponent.removeActorMovementCallbacks();
  m_inited = false;
}

LuaCallbacks Quest::makeQuestCallbacks(Player* player) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("state", [this]() { return QuestStateNames.getRight(state()); });

  callbacks.registerCallback("complete", [this](Maybe<size_t> followup) { complete(followup); });

  callbacks.registerCallback("fail", [this]() { fail(); });

  callbacks.registerCallback("setCanTurnIn", [this](bool value) { this->m_canTurnIn = value; });

  callbacks.registerCallback("questId", [this]() { return questId(); });

  callbacks.registerCallback("templateId", [this]() { return templateId(); });

  callbacks.registerCallback("seed", [this]() { return questDescriptor().seed; });

  callbacks.registerCallback("questDescriptor", [this]() { return questDescriptor().toJson(); });

  callbacks.registerCallback("questArcDescriptor", [this]() { return questArcDescriptor().toJson(); });

  callbacks.registerCallback("questArcPosition", [this]() { return m_arcPos; });

  callbacks.registerCallback("worldId", [this]() { return worldId().apply(printWorldId); });

  callbacks.registerCallback("setWorldId", [this](Maybe<String> const& worldId) { setWorldId(worldId.apply(parseWorldId)); });

  callbacks.registerCallback("serverUuid", [this]() { return serverUuid().apply(mem_fn(&Uuid::hex)); });

  callbacks.registerCallback("setServerUuid", [this](String const& serverUuid) { setServerUuid(Uuid(serverUuid)); });

  callbacks.registerCallback("isCurrent", [this, player]() -> bool { return player->questManager()->isCurrent(questId()); });

  callbacks.registerCallback("location", [this]() -> Json {
      if (auto loc = location()) {
        return JsonObject{
          {"system", jsonFromVec3I(loc.get().first)},
          {"location", jsonFromSystemLocation(loc.get().second)}
        };
      }
      return {};
    });

  callbacks.registerCallback("setLocation", [this](Json const& json) {
      if (json.isNull()) {
        setLocation({});
      } else {
        Vec3I system = jsonToVec3I(json.get("system"));
        SystemLocation location = jsonToSystemLocation(json.opt("location").value({}));
        setLocation(make_pair(system, location));
      }
    });

  callbacks.registerCallback("parameters", [this]() { return questParamsToJson(parameters()); });

  callbacks.registerCallback("setParameter",
      [this](String const& name, Json paramJson) { m_parameters[name] = QuestParam::fromJson(paramJson); });

  callbacks.registerCallback(
      "setIndicators", [this](StringList indicators) { m_indicators = StringSet::from(indicators); });

  callbacks.registerCallbackWithSignature<void, Maybe<JsonArray>>("setObjectiveList", bind(&Quest::setObjectiveList, this, _1));
  callbacks.registerCallbackWithSignature<void, Maybe<float>>("setProgress", bind(&Quest::setProgress, this, _1));
  callbacks.registerCallbackWithSignature<void, Maybe<float>>("setCompassDirection", bind(&Quest::setCompassDirection, this, _1));

  callbacks.registerCallbackWithSignature<void, String>("setTitle", [this](String const& title) {
      m_title = title;
    });
  callbacks.registerCallbackWithSignature<void, String>("setText", [this](String const& text) {
      m_text = text;
    });
  callbacks.registerCallbackWithSignature<void, String>("setCompletionText", [this](String const& completionText) {
      m_completionText = completionText;
    });
  callbacks.registerCallbackWithSignature<void, String>("setFailureText", [this](String const& failureText) {
      m_failureText = failureText;
    });

  callbacks.registerCallbackWithSignature<void, String, Maybe<JsonArray>>("setPortrait", [this](String const& portraitName, Maybe<JsonArray> const& portrait) {
      if (portrait) {
        m_portraits[portraitName] = portrait->transformed(construct<Drawable>());
      } else {
        m_portraits.remove(portraitName);
      }
    });
  callbacks.registerCallbackWithSignature<void, String, Maybe<String>>("setPortraitTitle", [this](String const& portraitName, Maybe<String> const& portrait) {
      if (portrait) {
        m_portraitTitles[portraitName] = *portrait;
      } else {
        m_portraitTitles.remove(portraitName);
      }
    });
  callbacks.registerCallbackWithSignature<void, Json>("addReward", [this](Json const& reward) {
      addReward(ItemDescriptor(reward));
    });

  return callbacks;
}

void Quest::setEntityParameter(String const& paramName, Entity const* entity) {
  Maybe<Json> portrait = {};
  Maybe<String> name = {};
  Maybe<String> species = {};
  Maybe<Gender> gender = {};

  if (auto portraitEntity = as<PortraitEntity>(entity)) {
    portrait = Json{portraitEntity->portrait(PortraitMode::Full).transformed(mem_fn(&Drawable::toJson))};
    name = portraitEntity->name();
  }

  if (auto npc = as<Npc>(entity)) {
    species = npc->species();
    gender = npc->gender();
  } else if (auto player = as<Player>(entity)) {
    species = player->species();
    gender = player->gender();
  }

  m_parameters[paramName] = QuestParam{QuestEntity{entity->uniqueId(), species, gender}, name, portrait, {}};
}

void Quest::addReward(ItemDescriptor const& reward) {
  if (reward.name() == "money") {
    m_money += reward.count();
    return;
  }
  auto itemDatabase = Root::singleton().itemDatabase();
  m_rewards.append(itemDatabase->item(reward));
}

String const& Quest::defaultCustomIndicator() const {
  return m_player->questManager()->isCurrent(questId()) ? m_trackedIndicator : m_untrackedIndicator;
}

QuestPtr createPreviewQuest(
    String const& templateId, String const& position, String const& questGiverSpecies, Player* player) {
  auto questTemplates = Root::singleton().questTemplateDatabase();
  auto questTemplate = questTemplates->questTemplate(templateId);
  if (!questTemplate)
    return {};

  Json portrait = player->portrait(PortraitMode::Full).transformed(mem_fn(&Drawable::toJson));

  JsonObject paramJson = questTemplate->parameterExamples;
  for (auto const& paramName : paramJson.keys()) {
    paramJson[paramName] = paramJson[paramName].set("type", questTemplate->parameterTypes[paramName]);
    paramJson[paramName] = paramJson[paramName].set("portrait", portrait);
  }
  StringMap<QuestParam> parameters = questParamsFromJson(paramJson);
  QuestDescriptor questDesc = QuestDescriptor{"preview", templateId, parameters, Random::randu64()};

  List<QuestDescriptor> quests;
  if (!position.equalsIgnoreCase("next") && !position.equalsIgnoreCase("last") && !position.equalsIgnoreCase("first")) {
    quests = {questDesc};
  } else {
    quests = {questDesc, questDesc, questDesc};
  }
  QuestArcDescriptor arc = QuestArcDescriptor{quests, {}};

  size_t arcPos = 0;
  if (position.equalsIgnoreCase("next")) {
    arcPos = 1;
  } else if (position.equalsIgnoreCase("last")) {
    arcPos = 2;
  }

  auto quest = make_shared<Quest>(arc, arcPos, player);
  quest->setParameter("questGiver", QuestParam{QuestEntity{{}, questGiverSpecies, {}}, {"Quest Giver"}, portrait, {}});
  return quest;
}

}
