#include "StarCollectionDatabase.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarMonsterDatabase.hpp"
#include "StarItemDatabase.hpp"

namespace Star {

EnumMap<CollectionType> const CollectionTypeNames {
  {CollectionType::Generic, "generic"},
  {CollectionType::Item, "item"},
  {CollectionType::Monster, "monster"}
};

Collection::Collection() : name(), title(), type() {}

Collection::Collection(String const& name, CollectionType type, String const& title) : name(name), title(title), type(type) {}

Collectable::Collectable() : name(), order(), title(), description(), icon() {}

Collectable::Collectable(String const& name, int order, String const& title, String const& description, String const& icon)
  : name(name), order(order), title(title), description(description), icon(icon) {};

CollectionDatabase::CollectionDatabase() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("collection");
  assets->queueJsons(files);
  for (auto file : files) {
    auto config = assets->json(file);

    Collection collection;

    collection.name = config.getString("name");
    collection.title = config.getString("title", collection.name);
    collection.type = CollectionTypeNames.getLeft(config.getString("type", "generic"));

    m_collectables[collection.name] = {};
    for (auto pair : config.get("collectables").iterateObject()) {
      Collectable collectable;
      if (collection.type == CollectionType::Monster)
        collectable = parseMonsterCollectable(pair.first, pair.second);
      else if (collection.type == CollectionType::Item)
        collectable = parseItemCollectable(pair.first, pair.second);
      else
        collectable = parseGenericCollectable(pair.first, pair.second);

      m_collectables[collection.name][collectable.name] = collectable;
    }

    m_collections[collection.name] = collection;
  }
}

List<Collection> CollectionDatabase::collections() const {
  return m_collections.values();
}

Collection CollectionDatabase::collection(String const& collectionName) const {
  try {
    return m_collections.get(collectionName); 
  } catch (MapException const& e) {
    throw CollectionDatabaseException(strf("Collection '%s' not found", collectionName), e);
  }
}

List<Collectable> CollectionDatabase::collectables(String const& collectionName) const {
  try {
    return m_collectables.get(collectionName).values();
  } catch (MapException const& e) {
    throw CollectionDatabaseException(strf("Collection '%s' not found", collectionName), e);
  }
}

Collectable CollectionDatabase::collectable(String const& collectionName, String const& collectableName) const {
  try {
    return m_collectables.get(collectionName).get(collectableName);
  } catch (MapException const& e) {
    throw CollectionDatabaseException(strf("Collectable '%s' not found in collection '%s'", collectableName, collectionName), e);
  }
}

bool CollectionDatabase::hasCollectable(String const& collectionName, String const& collectableName) const {
  return (m_collections.contains(collectionName) && m_collectables.get(collectionName).contains(collectableName));
}

Collectable CollectionDatabase::parseGenericCollectable(String const& name, Json const& config) const {
  Collectable collectable;
  collectable.name = name;
  collectable.order = config.getInt("order", 0);

  collectable.title = config.getString("title", "");
  collectable.description = config.getString("description", "");
  collectable.icon = config.getString("icon", "");

  return collectable;
}

Collectable CollectionDatabase::parseMonsterCollectable(String const& name, Json const& config) const {
  Collectable collectable = parseGenericCollectable(name, config);
  auto seed = 0; // use a static seed to utilize caching
  auto variant = Root::singleton().monsterDatabase()->monsterVariant(config.getString("monsterType"), seed);

  collectable.title = variant.shortDescription.value("");
  collectable.description = variant.description.value("");

  return collectable;
}

Collectable CollectionDatabase::parseItemCollectable(String const& name, Json const& config) const {
  Collectable collectable = parseGenericCollectable(name, config);
  auto itemDatabase = Root::singleton().itemDatabase();
  auto item = itemDatabase->item(ItemDescriptor(config.getString("item")));

  collectable.title = item->friendlyName();
  collectable.description = item->description();

  if (config.contains("icon")) {
    collectable.icon = config.getString("icon");
  } else {
    auto inventoryIcon = item->instanceValue("inventoryIcon", "");
    if (inventoryIcon.isType(Json::Type::String))
      collectable.icon = AssetPath::relativeTo(itemDatabase->itemConfig(item->name(), JsonObject()).directory, inventoryIcon.toString());
  }

  return collectable;
}

}
