#include "StarPhysicsObject.hpp"
#include "StarJsonExtra.hpp"
#include "StarInterpolation.hpp"
#include "StarRoot.hpp"
#include "StarObjectDatabase.hpp"
#include "StarLuaConverters.hpp"

namespace Star {

PhysicsObject::PhysicsObject(ObjectConfigConstPtr config, Json const& parameters) : Object(move(config), parameters) {
  for (auto const& p : configValue("physicsForces", JsonObject()).iterateObject()) {
    auto& forceConfig = m_physicsForces[p.first];

    forceConfig.forceRegion = jsonToPhysicsForceRegion(p.second);
    forceConfig.enabled.set(p.second.getBool("enabled", true));
  }

  for (auto const& p : configValue("physicsCollisions", JsonObject()).iterateObject()) {
    auto& collisionConfig = m_physicsCollisions[p.first];
    collisionConfig.movingCollision = PhysicsMovingCollision::fromJson(p.second);
    collisionConfig.xPosition.set(take(collisionConfig.movingCollision.position[0]));
    collisionConfig.yPosition.set(take(collisionConfig.movingCollision.position[1]));
    collisionConfig.enabled.set(p.second.getBool("enabled", true));
  }

  m_physicsForces.sortByKey();
  for (auto& p : m_physicsForces)
    m_netGroup.addNetElement(&p.second.enabled);

  m_physicsCollisions.sortByKey();
  for (auto& p : m_physicsCollisions) {
    m_netGroup.addNetElement(&p.second.xPosition);
    m_netGroup.addNetElement(&p.second.yPosition);
    p.second.xPosition.setInterpolator(lerp<float, float>);
    p.second.yPosition.setInterpolator(lerp<float, float>);
    m_netGroup.addNetElement(&p.second.enabled);
  }
}

void PhysicsObject::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void PhysicsObject::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

void PhysicsObject::init(World* world, EntityId entityId, EntityMode mode) {
  if (mode == EntityMode::Master) {
    LuaCallbacks physicsCallbacks;
    physicsCallbacks.registerCallback("setForceEnabled", [this](String const& force, bool enabled) {
        m_physicsForces.get(force).enabled.set(enabled);
      });
    physicsCallbacks.registerCallback("setCollisionPosition", [this](String const& collision, Vec2F const& pos) {
        auto& collisionConfig = m_physicsCollisions.get(collision);
        collisionConfig.xPosition.set(pos[0]);
        collisionConfig.yPosition.set(pos[1]);
      });
    physicsCallbacks.registerCallback("setCollisionEnabled", [this](String const& collision, bool const& enabled) {
        auto& collisionConfig = m_physicsCollisions.get(collision);
        collisionConfig.enabled.set(enabled);
      });
    m_scriptComponent.addCallbacks("physics", move(physicsCallbacks));
  }
  Object::init(world, entityId, mode);
  m_metaBoundBox = Object::metaBoundBox();
  for (auto const& p : m_physicsForces) {
    PhysicsForceRegion forceRegion = p.second.forceRegion;
    forceRegion.call([pos = position()](auto& fr) { fr.translate(pos); });
    m_metaBoundBox.combine(forceRegion.call([](auto& fr) { return fr.boundBox(); }));
  }
}

void PhysicsObject::uninit() {
  m_scriptComponent.removeCallbacks("physics");
  Object::uninit();
}

void PhysicsObject::update(float dt, uint64_t currentStep) {
  Object::update(dt, currentStep);
  if (isSlave())
    m_netGroup.tickNetInterpolation(WorldTimestep);
}

RectF PhysicsObject::metaBoundBox() const {
  return m_metaBoundBox;
}

List<PhysicsForceRegion> PhysicsObject::forceRegions() const {
  List<PhysicsForceRegion> forces;
  for (auto const& p : m_physicsForces) {
    if (p.second.enabled.get()) {
      PhysicsForceRegion forceRegion = p.second.forceRegion;
      forceRegion.call([pos = position()](auto& fr) { fr.translate(pos); });
      forces.append(move(forceRegion));
    }
  }
  return forces;
}

size_t PhysicsObject::movingCollisionCount() const {
  return m_physicsCollisions.size();
}

Maybe<PhysicsMovingCollision> PhysicsObject::movingCollision(size_t positionIndex) const {
  auto const& collisionConfig = m_physicsCollisions.valueAt(positionIndex);
  if (!collisionConfig.enabled.get())
    return {};
  PhysicsMovingCollision collision = collisionConfig.movingCollision;
  collision.translate(position() + Vec2F(collisionConfig.xPosition.get(), collisionConfig.yPosition.get()));
  return collision;
}

}
