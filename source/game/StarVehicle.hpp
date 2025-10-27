#pragma once

#include "StarNetElementSystem.hpp"
#include "StarEntity.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarMovementController.hpp"
#include "StarLuaComponents.hpp"
#include "StarLoungingEntities.hpp"
#include "StarScriptedEntity.hpp"
#include "StarLuaAnimationComponent.hpp"

namespace Star {

STAR_EXCEPTION(VehicleException, StarException);
STAR_CLASS(Vehicle);

class Vehicle : public virtual LoungeableEntity, public virtual InteractiveEntity, public virtual PhysicsEntity, public virtual ScriptedEntity {
public:
  Vehicle(Json baseConfig, String path, Json dynamicConfig);

  String name() const override;

  Json baseConfig() const;
  Json dynamicConfig() const;

  Json diskStore() const;
  void diskLoad(Json diskStore);

  EntityType entityType() const override;
  ClientEntityMode clientEntityMode() const override;

  List<DamageSource> damageSources() const override;
  Maybe<HitType> queryHit(DamageSource const& source) const override;
  Maybe<PolyF> hitPoly() const override;

  List<DamageNotification> applyDamage(DamageRequest const& damage) override;
  List<DamageNotification> selfDamageNotifications() override;

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  Vec2F position() const override;
  RectF metaBoundBox() const override;
  RectF collisionArea() const override;
  Vec2F velocity() const;

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0, NetCompatibilityRules rules = {}) override;
  void readNetState(ByteArray data, float interpolationTime = 0.0f, NetCompatibilityRules rules = {}) override;

  void enableInterpolation(float extrapolationHint) override;
  void disableInterpolation() override;

  void update(float dt, uint64_t currentStep) override;

  void render(RenderCallback* renderer) override;

  void renderLightSources(RenderCallback* renderer) override;

  List<LightSource> lightSources() const override;

  bool shouldDestroy() const override;
  void destroy(RenderCallback* renderCallback) override;

  Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) override;

  RectF interactiveBoundBox() const override;
  bool isInteractive() const override;
  InteractAction interact(InteractRequest const& request) override;

  List<PhysicsForceRegion> forceRegions() const override;
  size_t movingCollisionCount() const override;
  Maybe<PhysicsMovingCollision> movingCollision(size_t positionIndex) const override;

  Maybe<LuaValue> callScript(String const& func, LuaVariadic<LuaValue> const& args) override;
  Maybe<LuaValue> evalScript(String const& code) override;

  void setPosition(Vec2F const& position);

  virtual EntityRenderLayer loungeRenderLayer(size_t anchorPositionIndex) const override;
  virtual NetworkedAnimator const* networkedAnimator() const override;
  virtual NetworkedAnimator * networkedAnimator()  override;

  virtual LoungeableEntity::LoungePositions* loungePositions() override;
  virtual LoungeableEntity::LoungePositions const* loungePositions() const override;

private:
  struct MovingCollisionConfig {
    PhysicsMovingCollision movingCollision;
    Maybe<String> attachToPart;
    NetElementBool enabled;
  };

  struct ForceRegionConfig {
    PhysicsForceRegion forceRegion;
    Maybe<String> attachToPart;
    NetElementBool enabled;
  };

  struct DamageSourceConfig {
    DamageSource damageSource;
    Maybe<String> attachToPart;
    NetElementBool enabled;
  };

  enum class VehicleLayer { Back, Passenger, Front };

  EntityRenderLayer renderLayer(VehicleLayer vehicleLayer) const;

  LuaCallbacks makeVehicleCallbacks();
  Json configValue(String const& name, Json def = {}) const;

  String m_typeName;
  Json m_baseConfig;
  String m_path;
  Json m_dynamicConfig;
  RectF m_boundBox;
  OrderedHashMap<String, MovingCollisionConfig> m_movingCollisions;
  OrderedHashMap<String, ForceRegionConfig> m_forceRegions;

  ClientEntityMode m_clientEntityMode;

  NetElementTopGroup m_netGroup;
  NetElementBool m_interactive;
  MovementController m_movementController;
  NetworkedAnimator m_networkedAnimator;
  NetworkedAnimator::DynamicTarget m_networkedAnimatorDynamicTarget;
  LuaMessageHandlingComponent<LuaStorableComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>> m_scriptComponent;

  LuaAnimationComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>> m_scriptedAnimator;
  NetElementHashMap<String, Json> m_scriptedAnimationParameters;

  bool m_shouldDestroy = false;
  NetElementData<EntityDamageTeam> m_damageTeam;
  OrderedHashMap<String, DamageSourceConfig> m_damageSources;

  LoungePositions m_loungePositions;

  EntityRenderLayer m_baseRenderLayer;
  Maybe<EntityRenderLayer> m_overrideRenderLayer;

  GameTimer m_slaveHeartbeatTimer;
};

}
