#ifndef STAR_PHYSICS_OBJECT_HPP
#define STAR_PHYSICS_OBJECT_HPP

#include "StarObject.hpp"
#include "StarPhysicsEntity.hpp"

namespace Star {

class PhysicsObject : public Object, public virtual PhysicsEntity {
public:
  PhysicsObject(ObjectConfigConstPtr config, Json const& parameters = Json());

  void enableInterpolation(float extrapolationHint = 0.0f) override;
  void disableInterpolation() override;

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  void update(float dt, uint64_t currentStep) override;

  RectF metaBoundBox() const override;

  List<PhysicsForceRegion> forceRegions() const override;

  size_t movingCollisionCount() const override;
  Maybe<PhysicsMovingCollision> movingCollision(size_t positionIndex) const override;

private:
  struct PhysicsForceConfig {
    PhysicsForceRegion forceRegion;
    NetElementBool enabled;
  };

  struct PhysicsCollisionConfig {
    PhysicsMovingCollision movingCollision;
    NetElementFloat xPosition;
    NetElementFloat yPosition;
    NetElementBool enabled;
  };

  OrderedHashMap<String, PhysicsForceConfig> m_physicsForces;
  OrderedHashMap<String, PhysicsCollisionConfig> m_physicsCollisions;

  RectF m_metaBoundBox;
};

}

#endif
