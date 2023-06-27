#include "StarQuestDescriptor.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarItemDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarMonster.hpp"
#include "StarMonsterDatabase.hpp"
#include "StarObject.hpp"
#include "StarObjectDatabase.hpp"
#include "StarVersioningDatabase.hpp"

namespace Star {

bool QuestItem::operator==(QuestItem const& rhs) const {
  return itemName == rhs.itemName && parameters == rhs.parameters;
}

ItemDescriptor QuestItem::descriptor() const {
  return ItemDescriptor(itemName, 1, parameters);
}

bool QuestEntity::operator==(QuestEntity const& rhs) const {
  return uniqueId == rhs.uniqueId && species == rhs.species && gender == rhs.gender;
}

bool QuestLocation::operator==(QuestLocation const& rhs) const {
  return uniqueId == rhs.uniqueId && region == rhs.region;
}

bool QuestMonsterType::operator==(QuestMonsterType const& rhs) const {
  return typeName == rhs.typeName && parameters == rhs.parameters;
}

bool QuestNpcType::operator==(QuestNpcType const& rhs) const {
  return species == rhs.species && typeName == rhs.typeName && parameters == rhs.parameters && seed == rhs.seed;
}

bool QuestCoordinate::operator==(QuestCoordinate const& rhs) const {
  return coordinate == coordinate;
}

QuestParamDetail questParamDetailFromJson(Json const& json) {
  String type = json.getString("type");
  if (type == "item") {
    ItemDescriptor itemDescriptor = ItemDescriptor(json.get("item"));
    return QuestItem{itemDescriptor.name(), itemDescriptor.parameters()};

  } else if (type == "itemTag") {
    return QuestItemTag(json.getString("tag"));

  } else if (type == "itemList") {
    return QuestItemList(json.getArray("items").transformed(construct<ItemDescriptor>()));

  } else if (type == "entity") {
    return QuestEntity{
        json.optString("uniqueId"),
        json.optString("species"),
        json.optString("gender").apply([](String const& gender) { return GenderNames.getLeft(gender); })
      };

  } else if (type == "location") {
    return QuestLocation{json.optString("uniqueId"), jsonToRectF(json.get("region"))};

  } else if (type == "monsterType") {
    return QuestMonsterType{json.getString("typeName"), json.getObject("parameters", JsonObject{})};

  } else if (type == "npcType") {
    return QuestNpcType{
        json.getString("species"),
        json.getString("typeName"),
        json.getObject("parameters", JsonObject{}),
        json.optUInt("seed")
    };

  } else if (type == "coordinate") {
    return QuestCoordinate{ CelestialCoordinate(json.get("coordinate")) };

  } else if (type == "json") {
    return json;

  } else if (type == "noDetail") {
    return {};

  } else {
    throw StarException::format("Invalid QuestParam type {}", type);
  }
}

QuestParam QuestParam::fromJson(Json const& json) {
  return QuestParam{
      questParamDetailFromJson(json), json.optString("name"), json.opt("portrait"), json.optString("indicator")};
}

QuestParamDetail questParamDetailDiskLoad(Json const& json) {
  String type = json.getString("type");
  if (type == "item") {
    ItemDescriptor itemDescriptor = ItemDescriptor::loadStore(json.get("item"));
    return QuestItem{itemDescriptor.name(), itemDescriptor.parameters()};

  } else if (type == "itemList") {
    return QuestItemList(json.getArray("items").transformed(&ItemDescriptor::loadStore));

  } else {
    return questParamDetailFromJson(json);
  }
}

QuestParam QuestParam::diskLoad(Json const& json) {
  return QuestParam{
      questParamDetailDiskLoad(json), json.optString("name"), json.opt("portrait"), json.optString("indicator")};
}

Json questParamDetailToJson(QuestParamDetail const& detail) {
  if (detail.is<QuestItem>()) {
    QuestItem item = detail.get<QuestItem>();
    return JsonObject{{"type", "item"}, {"item", item.descriptor().toJson()}};

  } else if (detail.is<QuestItemTag>()) {
    String tag = detail.get<QuestItemTag>();
    return JsonObject{{"type", "itemTag"}, {"tag", tag}};

  } else if (detail.is<QuestItemList>()) {
    List<ItemDescriptor> itemList = detail.get<QuestItemList>();
    return JsonObject{{"type", "itemList"}, {"items", itemList.transformed(mem_fn(&ItemDescriptor::toJson))}};

  } else if (detail.is<QuestEntity>()) {
    QuestEntity entity = detail.get<QuestEntity>();
    return JsonObject{
        {"type", "entity"},
        {"uniqueId", jsonFromMaybe(entity.uniqueId)},
        {"species", jsonFromMaybe(entity.species)},
        {"gender", jsonFromMaybe(entity.gender.apply([](Gender gender) { return GenderNames.getRight(gender); }))}
      };

  } else if (detail.is<QuestLocation>()) {
    QuestLocation location = detail.get<QuestLocation>();
    return JsonObject{
        {"type", "location"},
        {"uniqueId", jsonFromMaybe<String>(location.uniqueId)},
        {"region", jsonFromRectF(location.region)}
      };

  } else if (detail.is<QuestMonsterType>()) {
    QuestMonsterType monsterType = detail.get<QuestMonsterType>();
    return JsonObject{
        {"type", "monsterType"}, {"typeName", monsterType.typeName}, {"parameters", monsterType.parameters}};

  }
  else if (detail.is<QuestNpcType>()) {
    QuestNpcType npcType = detail.get<QuestNpcType>();
    return JsonObject{ {"type", "npcType"},
        {"species", npcType.species},
        {"typeName", npcType.typeName},
        {"parameters", npcType.parameters},
        {"seed", jsonFromMaybe(npcType.seed)}
    };

  } else if (detail.is<QuestCoordinate>()) {
    return JsonObject{
      {"type", "coordinate"},
      {"coordinate", detail.get<QuestCoordinate>().coordinate.toJson()}
    };

  } else if (detail.is<QuestJson>()) {
    return detail.get<QuestJson>().set("type", "json");

  } else {
    starAssert(detail.empty());
    return JsonObject{{"type", "noDetail"}};
  }
}

Json QuestParam::toJson() const {
  Json detailJson = questParamDetailToJson(detail);
  return detailJson.set("name", jsonFromMaybe(name))
      .set("portrait", jsonFromMaybe(portrait))
      .set("indicator", jsonFromMaybe(indicator));
}

Json questParamDetailDiskStore(QuestParamDetail const& detail) {
  if (detail.is<QuestItem>()) {
    QuestItem item = detail.get<QuestItem>();
    return JsonObject{{"type", "item"}, {"item", item.descriptor().diskStore()}};

  } else if (detail.is<QuestItemList>()) {
    List<ItemDescriptor> itemList = detail.get<QuestItemList>();
    return JsonObject{{"type", "itemList"}, {"items", itemList.transformed(mem_fn(&ItemDescriptor::diskStore))}};
  } else {
    return questParamDetailToJson(detail);
  }
}

Json QuestParam::diskStore() const {
  Json detailJson = questParamDetailDiskStore(detail);
  return detailJson.set("name", jsonFromMaybe(name))
      .set("portrait", jsonFromMaybe(portrait))
      .set("indicator", jsonFromMaybe(indicator));
}

bool QuestParam::operator==(QuestParam const& rhs) const {
  return detail == rhs.detail && name == rhs.name && portrait == rhs.portrait && indicator == rhs.indicator;
}

QuestDescriptor QuestDescriptor::fromJson(Json const& json) {
  if (json.isType(Json::Type::String)) {
    return {json.toString(), json.toString(), {}, Random::randu64()};
  } else {
    return {json.getString("questId"),
        json.getString("templateId"),
        questParamsFromJson(json.get("parameters")),
        json.getUInt("seed", Random::randu64())};
  }
}

QuestDescriptor QuestDescriptor::diskLoad(Json const& spec) {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  Json json = versioningDatabase->loadVersionedJson(VersionedJson::fromJson(spec), "QuestDescriptor");
  return {json.getString("questId"),
      json.getString("templateId"),
      questParamsDiskLoad(json.get("parameters")),
      json.getUInt("seed", Random::randu64())};
}

Json QuestDescriptor::toJson() const {
  return JsonObject{
      {"questId", questId}, {"templateId", templateId}, {"parameters", questParamsToJson(parameters)}, {"seed", seed}};
}

Json QuestDescriptor::diskStore() const {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto res = JsonObject{{"questId", questId},
      {"templateId", templateId},
      {"parameters", questParamsDiskStore(parameters)},
      {"seed", seed}};
  return versioningDatabase->makeCurrentVersionedJson("QuestDescriptor", res).toJson();
}

bool QuestDescriptor::operator==(QuestDescriptor const& rhs) const {
  return questId == rhs.questId && templateId == rhs.templateId && parameters == rhs.parameters && seed == rhs.seed;
}

QuestArcDescriptor QuestArcDescriptor::fromJson(Json const& json) {
  if (json.isType(Json::Type::Object) && json.contains("quests")) {
    return QuestArcDescriptor{
        json.getArray("quests").transformed(&QuestDescriptor::fromJson), json.optString("stagehandUniqueId")};
  } else {
    return QuestArcDescriptor{{QuestDescriptor::fromJson(json)}, {}};
  }
}

QuestArcDescriptor QuestArcDescriptor::diskLoad(Json const& spec) {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  Json json = versioningDatabase->loadVersionedJson(VersionedJson::fromJson(spec), "QuestArcDescriptor");
  return QuestArcDescriptor{
      json.getArray("quests").transformed(&QuestDescriptor::diskLoad), json.optString("stagehandUniqueId")};
}

Json QuestArcDescriptor::toJson() const {
  return JsonObject{{"quests", quests.transformed(mem_fn(&QuestDescriptor::toJson))},
      {"stagehandUniqueId", jsonFromMaybe(stagehandUniqueId)}};
}

Json QuestArcDescriptor::diskStore() const {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto res = JsonObject{{"quests", quests.transformed(mem_fn(&QuestDescriptor::diskStore))},
      {"stagehandUniqueId", jsonFromMaybe(stagehandUniqueId)}};

  return versioningDatabase->makeCurrentVersionedJson("QuestArcDescriptor", res).toJson();
}

bool QuestArcDescriptor::operator==(QuestArcDescriptor const& rhs) const {
  return quests == rhs.quests && stagehandUniqueId == rhs.stagehandUniqueId;
}

String questParamText(QuestParam const& parameter) {
  if (parameter.name)
    return *parameter.name;

  auto itemDatabase = Root::singleton().itemDatabase();

  if (parameter.detail.is<QuestItem>()) {
    QuestItem item = parameter.detail.get<QuestItem>();
    return itemDatabase->item(item.descriptor())->friendlyName();

  } else if (parameter.detail.is<QuestItemList>()) {
    QuestItemList itemList = parameter.detail.get<QuestItemList>();
    StringList itemStrings = itemList.transformed([&itemDatabase](ItemDescriptor const& itemDesc) -> String {
      return strf("{} {}", itemDesc.count(), itemDatabase->item(itemDesc)->friendlyName());
    });
    return itemStrings.join(", ");

  } else {
    return "";
  }
}

template <typename Fun,
    typename ArgType = typename FunctionTraits<Fun>::template Arg<0>,
    typename RetType = typename FunctionTraits<Fun>::Return>
StringMap<RetType> transformedMapValues(StringMap<ArgType> const& map, Fun fun) {
  return StringMap<RetType>::from(map.pairs().transformed(
      [fun](pair<String, ArgType> entry) { return make_pair(entry.first, fun(entry.second)); }));
}

StringMap<String> questParamTags(StringMap<QuestParam> const& parameters) {
  return transformedMapValues(parameters, questParamText);
}

StringMap<QuestParam> questParamsFromJson(Json const& json) {
  return transformedMapValues(json.toObject(), &QuestParam::fromJson);
}

StringMap<QuestParam> questParamsDiskLoad(Json const& json) {
  return transformedMapValues(json.toObject(), &QuestParam::diskLoad);
}

Json questParamsToJson(StringMap<QuestParam> const& parameters) {
  return transformedMapValues(parameters, [](QuestParam const& quest) { return quest.toJson(); });
}

Json questParamsDiskStore(StringMap<QuestParam> const& parameters) {
  return transformedMapValues(parameters, [](QuestParam const& quest) { return quest.diskStore(); });
}

DataStream& operator>>(DataStream& ds, QuestItem& item) {
  ds.read(item.itemName);
  ds.read(item.parameters);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestItem const& item) {
  ds.write(item.itemName);
  ds.write(item.parameters);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestEntity& entity) {
  ds.read(entity.uniqueId);
  ds.read(entity.species);
  ds.read(entity.gender);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestEntity const& entity) {
  ds.write(entity.uniqueId);
  ds.write(entity.species);
  ds.write(entity.gender);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestLocation& location) {
  ds.read(location.uniqueId);
  ds.read(location.region);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestLocation const& location) {
  ds.write(location.uniqueId);
  ds.write(location.region);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestMonsterType& monsterType) {
  ds.read(monsterType.typeName);
  ds.read(monsterType.parameters);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestMonsterType const& monsterType) {
  ds.write(monsterType.typeName);
  ds.write(monsterType.parameters);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestNpcType& npcType) {
  ds.read(npcType.species);
  ds.read(npcType.typeName);
  ds.read(npcType.parameters);
  ds.read(npcType.seed);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestNpcType const& npcType) {
  ds.write(npcType.species);
  ds.write(npcType.typeName);
  ds.write(npcType.parameters);
  ds.write(npcType.seed);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestCoordinate& coordinate) {
  ds.read(coordinate.coordinate);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestCoordinate const& coordinate) {
  ds.write(coordinate.coordinate);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestParam& param) {
  ds.read(param.detail);
  ds.read(param.name);
  ds.read(param.portrait);
  ds.read(param.indicator);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestParam const& param) {
  ds.write(param.detail);
  ds.write(param.name);
  ds.write(param.portrait);
  ds.write(param.indicator);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestDescriptor& quest) {
  ds.read(quest.questId);
  ds.read(quest.templateId);
  ds.read(quest.parameters);
  ds.read(quest.seed);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestDescriptor const& quest) {
  ds.write(quest.questId);
  ds.write(quest.templateId);
  ds.write(quest.parameters);
  ds.write(quest.seed);
  return ds;
}

DataStream& operator>>(DataStream& ds, QuestArcDescriptor& questArc) {
  ds.read(questArc.quests);
  ds.read(questArc.stagehandUniqueId);
  return ds;
}

DataStream& operator<<(DataStream& ds, QuestArcDescriptor const& questArc) {
  ds.write(questArc.quests);
  ds.write(questArc.stagehandUniqueId);
  return ds;
}

}
