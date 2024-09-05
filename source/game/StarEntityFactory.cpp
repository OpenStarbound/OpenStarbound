#include "StarEntityFactory.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerFactory.hpp"
#include "StarMonster.hpp"
#include "StarObject.hpp"
#include "StarObjectDatabase.hpp"
#include "StarPlant.hpp"
#include "StarPlantDrop.hpp"
#include "StarProjectile.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarItemDrop.hpp"
#include "StarNpc.hpp"
#include "StarRoot.hpp"
#include "StarStagehand.hpp"
#include "StarVehicleDatabase.hpp"

namespace Star {

EnumMap<EntityType> const EntityFactory::EntityStorageIdentifiers{
  {EntityType::Player, "PlayerEntity"},
  {EntityType::Monster, "MonsterEntity"},
  {EntityType::Object, "ObjectEntity"},
  {EntityType::ItemDrop, "ItemDropEntity"},
  {EntityType::Projectile, "ProjectileEntity"},
  {EntityType::Plant, "PlantEntity"},
  {EntityType::PlantDrop, "PlantDropEntity"},
  {EntityType::Npc, "NpcEntity"},
  {EntityType::Stagehand, "StagehandEntity"},
  {EntityType::Vehicle, "VehicleEntity"}
};

EntityFactory::EntityFactory() {
  auto& root = Root::singleton();
  m_playerFactory = root.playerFactory();
  m_monsterDatabase = root.monsterDatabase();
  m_objectDatabase = root.objectDatabase();
  m_projectileDatabase = root.projectileDatabase();
  m_npcDatabase = root.npcDatabase();
  m_vehicleDatabase = root.vehicleDatabase();
  m_versioningDatabase = root.versioningDatabase();
}

ByteArray EntityFactory::netStoreEntity(EntityPtr const& entity, NetCompatibilityRules rules) const {
  RecursiveMutexLocker locker(m_mutex);

  if (auto player = as<Player>(entity)) {
    return player->netStore(rules);
  } else if (auto monster = as<Monster>(entity)) {
    return monster->netStore(rules);
  } else if (auto object = as<Object>(entity)) {
    return object->netStore(rules);
  } else if (auto plant = as<Plant>(entity)) {
    return plant->netStore(rules);
  } else if (auto plantDrop = as<PlantDrop>(entity)) {
    return plantDrop->netStore(rules);
  } else if (auto projectile = as<Projectile>(entity)) {
    return projectile->netStore(rules);
  } else if (auto itemDrop = as<ItemDrop>(entity)) {
    return itemDrop->netStore(rules);
  } else if (auto npc = as<Npc>(entity)) {
    return npc->netStore(rules);
  } else if (auto stagehand = as<Stagehand>(entity)) {
    return stagehand->netStore(rules);
  } else if (auto vehicle = as<Vehicle>(entity)) {
    return m_vehicleDatabase->netStore(vehicle, rules);
  } else {
    throw EntityFactoryException::format("Don't know how to make net store for entity type '{}'", EntityTypeNames.getRight(entity->entityType()));
  }
}

EntityPtr EntityFactory::netLoadEntity(EntityType type, ByteArray const& netStore, NetCompatibilityRules rules) const {
  RecursiveMutexLocker locker(m_mutex);

  if (type == EntityType::Player) {
    return m_playerFactory->netLoadPlayer(netStore, rules);
  } else if (type == EntityType::Monster) {
    return m_monsterDatabase->netLoadMonster(netStore, rules);
  } else if (type == EntityType::Object) {
    return m_objectDatabase->netLoadObject(netStore, rules);
  } else if (type == EntityType::Plant) {
    return make_shared<Plant>(netStore, rules);
  } else if (type == EntityType::PlantDrop) {
    return make_shared<PlantDrop>(netStore, rules);
  } else if (type == EntityType::Projectile) {
    return m_projectileDatabase->netLoadProjectile(netStore, rules);
  } else if (type == EntityType::ItemDrop) {
    return make_shared<ItemDrop>(netStore, rules);
  } else if (type == EntityType::Npc) {
    return m_npcDatabase->netLoadNpc(netStore, rules);
  } else if (type == EntityType::Stagehand) {
    return make_shared<Stagehand>(netStore, rules);
  } else if (type == EntityType::Vehicle) {
    return m_vehicleDatabase->netLoad(netStore, rules);
  } else {
    throw EntityFactoryException::format("Don't know how to create entity type '{}' from net store", EntityTypeNames.getRight(type));
  }
}

Json EntityFactory::diskStoreEntity(EntityPtr const& entity) const {
  RecursiveMutexLocker locker(m_mutex);

  if (auto player = as<Player>(entity)) {
    return player->diskStore();
  } else if (auto monster = as<Monster>(entity)) {
    return monster->diskStore();
  } else if (auto object = as<Object>(entity)) {
    return object->diskStore();
  } else if (auto plant = as<Plant>(entity)) {
    return plant->diskStore();
  } else if (auto itemDrop = as<ItemDrop>(entity)) {
    return itemDrop->diskStore();
  } else if (auto npc = as<Npc>(entity)) {
    return npc->diskStore();
  } else if (auto stagehand = as<Stagehand>(entity)) {
    return stagehand->diskStore();
  } else if (auto vehicle = as<Vehicle>(entity)) {
    return m_vehicleDatabase->diskStore(vehicle);
  } else {
    throw EntityFactoryException::format("Don't know how to make disk store for entity type '{}'", EntityTypeNames.getRight(entity->entityType()));
  }
}

EntityPtr EntityFactory::diskLoadEntity(EntityType type, Json const& diskStore) const {
  RecursiveMutexLocker locker(m_mutex);

  if (type == EntityType::Player) {
    return m_playerFactory->diskLoadPlayer(diskStore);
  } else if (type == EntityType::Monster) {
    return m_monsterDatabase->diskLoadMonster(diskStore);
  } else if (type == EntityType::Object) {
    return m_objectDatabase->diskLoadObject(diskStore);
  } else if (type == EntityType::Plant) {
    return make_shared<Plant>(diskStore);
  } else if (type == EntityType::ItemDrop) {
    return make_shared<ItemDrop>(diskStore);
  } else if (type == EntityType::Npc) {
    return m_npcDatabase->diskLoadNpc(diskStore);
  } else if (type == EntityType::Stagehand) {
    return make_shared<Stagehand>(diskStore);
  } else if (type == EntityType::Vehicle) {
    return m_vehicleDatabase->diskLoad(diskStore);
  } else {
    throw EntityFactoryException::format("Don't know how to create entity type '{}' from disk store", EntityTypeNames.getRight(type));
  }
}

Json EntityFactory::loadVersionedJson(VersionedJson const& versionedJson, EntityType expectedType) const {
  RecursiveMutexLocker locker(m_mutex);

  String identifier = EntityStorageIdentifiers.getRight(expectedType);
  return m_versioningDatabase->loadVersionedJson(versionedJson, identifier);
}

VersionedJson EntityFactory::storeVersionedJson(EntityType type, Json const& store) const {
  RecursiveMutexLocker locker(m_mutex);

  String identifier = EntityStorageIdentifiers.getRight(type);
  return m_versioningDatabase->makeCurrentVersionedJson(identifier, store);
}

EntityPtr EntityFactory::loadVersionedEntity(VersionedJson const& versionedJson) const {
  RecursiveMutexLocker locker(m_mutex);

  EntityType type = EntityStorageIdentifiers.getLeft(versionedJson.identifier);
  auto store = loadVersionedJson(versionedJson, type);
  return diskLoadEntity(type, store);
}

VersionedJson EntityFactory::storeVersionedEntity(EntityPtr const& entityPtr) const {
  return storeVersionedJson(entityPtr->entityType(), diskStoreEntity(entityPtr));
}

}
