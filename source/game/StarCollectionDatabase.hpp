#ifndef STAR_COLLECTION_DATABASE_HPP
#define STAR_COLLECTION_DATABASE_HPP

#include "StarGameTypes.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_EXCEPTION(CollectionDatabaseException, StarException);

STAR_CLASS(CollectionDatabase);

enum class CollectionType : uint16_t {
  Generic,
  Item,
  Monster
};
extern EnumMap<CollectionType> const CollectionTypeNames;

struct Collectable {
  Collectable();
  Collectable(String const& name, int order, String const& title, String const& description, String const& icon);

  String name;
  int order;
  String title;
  String description;
  String icon;
};

struct Collection {
  Collection();
  Collection(String const& name, CollectionType type, String const& icon);

  String name;
  String title;
  CollectionType type;
};

class CollectionDatabase {
public:
  CollectionDatabase();

  List<Collection> collections() const;
  Collection collection(String const& collectionName) const;
  List<Collectable> collectables(String const& collectionName) const;
  Collectable collectable(String const& collectionName, String const& collectableName) const;

  bool hasCollectable(String const& collectionName, String const& collectableName) const;

private:
  Collectable parseGenericCollectable(String const& name, Json const& config) const;
  Collectable parseMonsterCollectable(String const& name, Json const& config) const;
  Collectable parseItemCollectable(String const& name, Json const& config) const;

  StringMap<Collection> m_collections;
  StringMap<StringMap<Collectable>> m_collectables;
};


}
#endif