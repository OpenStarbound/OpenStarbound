#ifndef STAR_QUESTDESCRIPTOR_HPP
#define STAR_QUESTDESCRIPTOR_HPP

#include "StarJson.hpp"
#include "StarPoly.hpp"
#include "StarGameTypes.hpp"
#include "StarStrongTypedef.hpp"
#include "StarItemDescriptor.hpp"
#include "StarCelestialCoordinate.hpp"

namespace Star {

// Item name - always one single item. QuestItem and QuestItemList are
// distinct due to how the surrounding text interacts with the parameter
// in the quest text. For a single item we might want to say "the <bandage>" or
// "any <bandage>", whereas the text for QuestItemList is always a list, e.g.
// "<1 bandage, 3 apple>."
struct QuestItem {
  bool operator==(QuestItem const& rhs) const;
  ItemDescriptor descriptor() const;

  String itemName;
  Json parameters;
};

// An item itemTag, indicating a set of possible items
strong_typedef(String, QuestItemTag);

// A collection of items
strong_typedef(List<ItemDescriptor>, QuestItemList);

// The uniqueId of a specific entity
struct QuestEntity {
  bool operator==(QuestEntity const& rhs) const;

  Maybe<String> uniqueId;
  Maybe<String> species;
  Maybe<Gender> gender;
};

// A location within the world, which could represent a spawn point or a dungeon
struct QuestLocation {
  bool operator==(QuestLocation const& rhs) const;

  Maybe<String> uniqueId;
  RectF region;
};

struct QuestMonsterType {
  bool operator==(QuestMonsterType const& rhs) const;

  String typeName;
  JsonObject parameters;
};

struct QuestNpcType {
  bool operator==(QuestNpcType const& rhs) const;

  String species;
  String typeName;
  JsonObject parameters;
  Maybe<uint64_t> seed;
};

struct QuestCoordinate {
  bool operator==(QuestCoordinate const& rhs) const;

  CelestialCoordinate coordinate;
};

typedef Json QuestJson;

typedef MVariant<QuestItem, QuestItemTag, QuestItemList, QuestEntity, QuestLocation, QuestMonsterType, QuestNpcType, QuestCoordinate, QuestJson> QuestParamDetail;

struct QuestParam {
  static QuestParam fromJson(Json const& json);
  static QuestParam diskLoad(Json const& json);

  Json toJson() const;
  Json diskStore() const;

  bool operator==(QuestParam const& rhs) const;

  QuestParamDetail detail;
  Maybe<String> name;
  Maybe<Json> portrait;
  Maybe<String> indicator;
};

struct QuestDescriptor {
  static QuestDescriptor fromJson(Json const& json);
  static QuestDescriptor diskLoad(Json const& json);

  Json toJson() const;
  Json diskStore() const;

  bool operator==(QuestDescriptor const& rhs) const;

  String questId;
  String templateId;
  StringMap<QuestParam> parameters;
  uint64_t seed;
};

struct QuestArcDescriptor {
  static QuestArcDescriptor fromJson(Json const& json);
  static QuestArcDescriptor diskLoad(Json const& json);

  Json toJson() const;
  Json diskStore() const;

  bool operator==(QuestArcDescriptor const& rhs) const;

  List<QuestDescriptor> quests;
  Maybe<String> stagehandUniqueId;
};

String questParamText(QuestParam const& param);
StringMap<String> questParamTags(StringMap<QuestParam> const& parameters);

StringMap<QuestParam> questParamsFromJson(Json const& json);
StringMap<QuestParam> questParamsDiskLoad(Json const& json);
Json questParamsToJson(StringMap<QuestParam> const& parameters);
Json questParamsDiskStore(StringMap<QuestParam> const& parameters);

DataStream& operator>>(DataStream& ds, QuestItem& param);
DataStream& operator<<(DataStream& ds, QuestItem const& param);
DataStream& operator>>(DataStream& ds, QuestEntity& param);
DataStream& operator<<(DataStream& ds, QuestEntity const& param);
DataStream& operator>>(DataStream& ds, QuestMonsterType& param);
DataStream& operator<<(DataStream& ds, QuestMonsterType const& param);
DataStream& operator>>(DataStream& ds, QuestNpcType& param);
DataStream& operator<<(DataStream& ds, QuestNpcType const& param);
DataStream& operator>>(DataStream& ds, QuestCoordinate& param);
DataStream& operator<<(DataStream& ds, QuestCoordinate const& param);
DataStream& operator>>(DataStream& ds, QuestParam& param);
DataStream& operator<<(DataStream& ds, QuestParam const& param);
DataStream& operator>>(DataStream& ds, QuestDescriptor& quest);
DataStream& operator<<(DataStream& ds, QuestDescriptor const& quest);
DataStream& operator>>(DataStream& ds, QuestArcDescriptor& questArc);
DataStream& operator<<(DataStream& ds, QuestArcDescriptor const& questArc);
}

#endif
