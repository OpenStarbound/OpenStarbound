#include "StarEntity.hpp"
#include "StarDamageManager.hpp"

namespace Star {

EnumMap<ClientEntityMode> const ClientEntityModeNames{
  {ClientEntityMode::ClientSlaveOnly, "ClientSlaveOnly"},
  {ClientEntityMode::ClientMasterAllowed, "ClientMasterAllowed"},
  {ClientEntityMode::ClientPresenceMaster, "ClientPresenceMaster"}
};

EnumMap<EntityType> const EntityTypeNames{
  {EntityType::Plant, "plant"},
  {EntityType::Object, "object"},
  {EntityType::Vehicle, "vehicle"},
  {EntityType::ItemDrop, "itemDrop"},
  {EntityType::PlantDrop, "plantDrop"},
  {EntityType::Projectile, "projectile"},
  {EntityType::Stagehand, "stagehand"},
  {EntityType::Monster, "monster"},
  {EntityType::Npc, "npc"},
  {EntityType::Player, "player"}
};

Entity::~Entity() {}

void Entity::init(World* world, EntityId entityId, EntityMode mode) {
  if (!world)
    throw EntityException("Entity::init called with null world pointer");
  if (entityId == NullEntityId)
    throw EntityException("Entity::init called with null entity id");
  if (m_world)
    throw EntityException("Entity::init called when already initialized");

  m_world = world;
  m_entityMode = mode;
  m_entityId = entityId;
}

void Entity::uninit() {
  m_world = nullptr;
  m_entityMode = {};
  m_entityId = NullEntityId;
}

pair<ByteArray, uint64_t> Entity::writeNetState(uint64_t) {
  return {ByteArray(), 0};
}

void Entity::readNetState(ByteArray, float) {}

void Entity::enableInterpolation(float) {}

void Entity::disableInterpolation() {}

RectF Entity::collisionArea() const {
  return RectF::null();
}

bool Entity::ephemeral() const {
  return false;
}

ClientEntityMode Entity::clientEntityMode() const {
  return ClientEntityMode::ClientSlaveOnly;
}

bool Entity::masterOnly() const {
  return false;
}

String Entity::description() const {
  return "";
}

List<LightSource> Entity::lightSources() const {
  return {};
}

List<DamageSource> Entity::damageSources() const {
  return {};
}

void Entity::hitOther(EntityId, DamageRequest const&) {}

void Entity::damagedOther(DamageNotification const&) {}

Maybe<HitType> Entity::queryHit(DamageSource const&) const {
  return {};
}

Maybe<PolyF> Entity::hitPoly() const {
  return {};
}

List<DamageNotification> Entity::applyDamage(DamageRequest const&) {
  return {};
}

List<DamageNotification> Entity::selfDamageNotifications() {
  return {};
}

bool Entity::shouldDestroy() const {
  return false;
}

void Entity::destroy(RenderCallback*) {}

Maybe<Json> Entity::receiveMessage(ConnectionId, String const&, JsonArray const&) {
  return {};
}

void Entity::update(uint64_t) {}

void Entity::render(RenderCallback*) {}

void Entity::renderLightSources(RenderCallback*) {}

EntityId Entity::entityId() const {
  return m_entityId;
}

EntityDamageTeam Entity::getTeam() const {
  return m_team;
}

bool Entity::inWorld() const {
  if (m_world) {
    starAssert(m_world && m_entityId != NullEntityId && m_entityMode);
    return true;
  } else {
    starAssert(!m_world && m_entityId == NullEntityId && !m_entityMode);
    return false;
  }
}

World* Entity::world() const {
  if (!m_world)
    throw EntityException("world() called while uninitialized");

  return m_world;
}

World* Entity::worldPtr() const {
  return m_world;
}

bool Entity::persistent() const {
  return m_persistent;
}

bool Entity::keepAlive() const {
  return m_keepAlive;
}

Maybe<String> Entity::uniqueId() const {
  return m_uniqueId;
}

Maybe<EntityMode> Entity::entityMode() const {
  return m_entityMode;
}

bool Entity::isMaster() const {
  return m_entityMode == EntityMode::Master;
}

bool Entity::isSlave() const {
  return m_entityMode == EntityMode::Slave;
}

Entity::Entity() {
  m_world = nullptr;
  m_entityId = NullEntityId;
  m_persistent = false;
  m_keepAlive = false;
}

void Entity::setPersistent(bool persistent) {
  m_persistent = persistent;
}

void Entity::setKeepAlive(bool keepAlive) {
  m_keepAlive = keepAlive;
}

void Entity::setUniqueId(Maybe<String> uniqueId) {
  m_uniqueId = uniqueId;
}

void Entity::setTeam(EntityDamageTeam newTeam) {
  m_team = newTeam;
}

}
