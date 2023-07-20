#ifndef STAR_ENTITY_HPP
#define STAR_ENTITY_HPP

#include "StarCasting.hpp"
#include "StarDamage.hpp"
#include "StarLightSource.hpp"

namespace Star {

STAR_CLASS(RenderCallback);
STAR_CLASS(World);
STAR_STRUCT(DamageNotification);
STAR_CLASS(Entity);

STAR_EXCEPTION(EntityException, StarException);

// Specifies how the client should treat an entity created on the client,
// whether it should always be sent to the server and be a slave on the client,
// whether it is allowed to be master on the client, and whether client master
// entities should contribute to client presence.
enum class ClientEntityMode {
  // Always a slave on the client
  ClientSlaveOnly,
  // Can be a master on the client
  ClientMasterAllowed,
  // Can be a master on the client, and when it is contributes to client
  // presence.
  ClientPresenceMaster
};
extern EnumMap<ClientEntityMode> const ClientEntityModeNames;

// The top-level entity type.  The enum order is intended to be in the order in
// which entities should be updated every tick
enum class EntityType : uint8_t {
  Plant,
  Object,
  Vehicle,
  ItemDrop,
  PlantDrop,
  Projectile,
  Stagehand,
  Monster,
  Npc,
  Player
};
extern EnumMap<EntityType> const EntityTypeNames;

class Entity {
public:
  virtual ~Entity();

  virtual EntityType entityType() const = 0;

  // Called when an entity is first inserted into a World.  Calling base class
  // init sets the world pointer, entityId, and entityMode.
  virtual void init(World* world, EntityId entityId, EntityMode mode);

  // Should do whatever steps necessary to take an entity out of a world,
  // default implementation clears the world pointer, entityMode, and entityId.
  virtual void uninit();

  // Write state data that changes over time, and is used to keep slaves in
  // sync.  Can return empty and this is the default.  May be called
  // uninitalized.  Should return the delta to be written to the slave, along
  // with the version to pass into writeDeltaState on the next call.  The first
  // delta written to a slave entity will always be the delta starting with 0.
  virtual pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0);
  // Will be called with deltas written by writeDeltaState, including if the
  // delta is empty.  interpolationTime will be provided if interpolation is
  // enabled.
  virtual void readNetState(ByteArray data, float interpolationTime = 0.0);

  virtual void enableInterpolation(float extrapolationHint);
  virtual void disableInterpolation();

  // Base position of this entity, bound boxes, drawables, and other entity
  // positions are relative to this.
  virtual Vec2F position() const = 0;

  // Largest bounding-box of this entity.  Any damage boxes / drawables / light
  // or sound *sources* must be contained within this bounding box.  Used for
  // all top-level spatial queries.
  virtual RectF metaBoundBox() const = 0;

  // By default returns a null rect, if non-null, it defines the area around
  // this entity where it is likely for the entity to physically collide with
  // collision geometry.
  virtual RectF collisionArea() const;

  // Should this entity allow object / block placement over it, and can the
  // entity immediately be despawned without terribly bad effects?
  virtual bool ephemeral() const;

  // How should this entity be treated if created on the client?  Defaults to
  // ClientSlave.
  virtual ClientEntityMode clientEntityMode() const;
  // Should this entity only exist on the master side?
  virtual bool masterOnly() const;

  virtual String description() const;

  // Gameplay affecting light sources (separate from light sources added during
  // rendering)
  virtual List<LightSource> lightSources() const;

  // All damage sources for this frame.
  virtual List<DamageSource> damageSources() const;

  // Return the damage that would result from being hit by the given damage
  // source.  Will be called on master and slave entities.  Culling based on
  // team damage and self damage will be done outside of this query.
  virtual Maybe<HitType> queryHit(DamageSource const& source) const;

  // Return the polygonal area in which the entity can be hit. Not used for
  // actual hit computation, only for determining more precisely where a
  // hit intersection occurred (e.g. by projectiles)
  virtual Maybe<PolyF> hitPoly() const;

  // Apply a request to damage this entity. Will only be called on Master
  // entities. DamageRequest might be adjusted based on protection and other
  // effects
  virtual List<DamageNotification> applyDamage(DamageRequest const& damage);

  // Pull any pending damage notifications applied internally, only called on
  // Master entities.
  virtual List<DamageNotification> selfDamageNotifications();

  // Called on master entities when a DamageRequest has been generated due to a
  // DamageSource from this entity being applied to another entity.  Will be
  // called on the *causing* entity of the damage.
  virtual void hitOther(EntityId targetEntityId, DamageRequest const& damageRequest);

  // Called on master entities when this entity has damaged another entity.
  // Only called on the *source entity* of the damage, which may be different
  // than the causing entity.
  virtual void damagedOther(DamageNotification const& damage);

  // Returning true here indicates that this entity should be removed from the
  // world, default returns false.
  virtual bool shouldDestroy() const;
  // Will be called once before removing the entity from the World on both
  // master and slave entities.
  virtual void destroy(RenderCallback* renderCallback);

  // Entities can send other entities potentially remote messages and get
  // responses back from them, and should implement this to receive and respond
  // to messages.  If the message is NOT handled, should return Nothing,
  // otherwise should return some Json value.
  // This will only ever be called on master entities.
  virtual Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args);

  virtual void update(float dt, uint64_t currentStep);

  virtual void render(RenderCallback* renderer);

  virtual void renderLightSources(RenderCallback* renderer);

  EntityId entityId() const;

  EntityDamageTeam getTeam() const;

  // Returns true if an entity is initialized in a world, and thus has a valid
  // world pointer, entity id, and entity mode.
  bool inWorld() const;

  // Throws an exception if not currently in a world.
  World* world() const;
  // Returns nullptr if not currently in a world.
  World* worldPtr() const;

  // Specifies if the entity is to be saved to disk alongside the sector or
  // despawned.
  bool persistent() const;

  // Entity should keep any sector it is in alive.  Default implementation
  // returns false.
  bool keepAlive() const;

  // If set, then the entity will be discoverable by its unique id and will be
  // indexed in the stored world.  Unique ids must be different across all
  // entities in a single world.
  Maybe<String> uniqueId() const;

  // EntityMode will only be set if the entity is initialized, if the entity is
  // uninitialized then isMaster and isSlave will both return false.
  Maybe<EntityMode> entityMode() const;
  bool isMaster() const;
  bool isSlave() const;

protected:
  Entity();

  void setPersistent(bool persistent);
  void setKeepAlive(bool keepAlive);
  void setUniqueId(Maybe<String> uniqueId);
  void setTeam(EntityDamageTeam newTeam);

private:
  EntityId m_entityId;
  Maybe<EntityMode> m_entityMode;
  bool m_persistent;
  bool m_keepAlive;
  Maybe<String> m_uniqueId;
  World* m_world;
  EntityDamageTeam m_team;
};

template <typename EntityT>
using EntityCallbackOf = function<void(shared_ptr<EntityT> const&)>;

template <typename EntityT>
using EntityFilterOf = function<bool(shared_ptr<EntityT> const&)>;

typedef EntityCallbackOf<Entity> EntityCallback;
typedef EntityFilterOf<Entity> EntityFilter;

// Filters based first on dynamic casting to the given type, then optionally on
// the given derived type filter.
template <typename EntityT>
EntityFilter entityTypeFilter(function<bool(shared_ptr<EntityT> const&)> filter = {}) {
  return [filter](EntityPtr const& e) -> bool {
    if (auto entity = as<EntityT>(e)) {
      return !filter || filter(entity);
    } else {
      return false;
    }
  };
}
}

#endif
