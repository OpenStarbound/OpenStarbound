#include "StarMovementController.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"
#include "StarWorld.hpp"
#include "StarAssets.hpp"
#include "StarRandom.hpp"

namespace Star {

MovementParameters MovementParameters::sensibleDefaults() {
  return MovementParameters(Root::singleton().assets()->json("/default_movement.config").toObject());
}

MovementParameters::MovementParameters(Json const& config) {
  if (config.isNull())
    return;

  mass = config.optFloat("mass");
  gravityMultiplier = config.optFloat("gravityMultiplier");
  liquidBuoyancy = config.optFloat("liquidBuoyancy");
  airBuoyancy = config.optFloat("airBuoyancy");
  bounceFactor = config.optFloat("bounceFactor");
  stopOnFirstBounce = config.optBool("stopOnFirstBounce");
  enableSurfaceSlopeCorrection = config.optBool("enableSurfaceSlopeCorrection");
  slopeSlidingFactor = config.optFloat("slopeSlidingFactor");
  maxMovementPerStep = config.optFloat("maxMovementPerStep");
  maximumCorrection = config.optFloat("maximumCorrection");
  speedLimit = config.optFloat("speedLimit");
  discontinuityThreshold = config.optFloat("discontinuityThreshold");
  collisionPoly = config.opt("collisionPoly").apply(jsonToPolyF);
  stickyCollision = config.optBool("stickyCollision");
  stickyForce = config.optFloat("stickyForce");
  airFriction = config.optFloat("airFriction");
  liquidFriction = config.optFloat("liquidFriction");
  groundFriction = config.optFloat("groundFriction");
  collisionEnabled = config.optBool("collisionEnabled");
  frictionEnabled = config.optBool("frictionEnabled");
  gravityEnabled = config.optBool("gravityEnabled");
  ignorePlatformCollision = config.optBool("ignorePlatformCollision");
  maximumPlatformCorrection = config.optFloat("maximumPlatformCorrection");
  maximumPlatformCorrectionVelocityFactor = config.optFloat("maximumPlatformCorrectionVelocityFactor");
  physicsEffectCategories = config.opt("physicsEffectCategories").apply(jsonToStringSet);
  restDuration = config.optInt("restDuration");
}

MovementParameters MovementParameters::merge(MovementParameters const& rhs) const {
  MovementParameters merged;

  merged.mass = rhs.mass.orMaybe(mass);
  merged.gravityMultiplier = rhs.gravityMultiplier.orMaybe(gravityMultiplier);
  merged.liquidBuoyancy = rhs.liquidBuoyancy.orMaybe(liquidBuoyancy);
  merged.airBuoyancy = rhs.airBuoyancy.orMaybe(airBuoyancy);
  merged.bounceFactor = rhs.bounceFactor.orMaybe(bounceFactor);
  merged.stopOnFirstBounce = rhs.stopOnFirstBounce.orMaybe(stopOnFirstBounce);
  merged.enableSurfaceSlopeCorrection = rhs.enableSurfaceSlopeCorrection.orMaybe(enableSurfaceSlopeCorrection);
  merged.slopeSlidingFactor = rhs.slopeSlidingFactor.orMaybe(slopeSlidingFactor);
  merged.maxMovementPerStep = rhs.maxMovementPerStep.orMaybe(maxMovementPerStep);
  merged.maximumCorrection = rhs.maximumCorrection.orMaybe(maximumCorrection);
  merged.speedLimit = rhs.speedLimit.orMaybe(speedLimit);
  merged.discontinuityThreshold = rhs.discontinuityThreshold.orMaybe(discontinuityThreshold);
  merged.collisionPoly = rhs.collisionPoly.orMaybe(collisionPoly);
  merged.stickyCollision = rhs.stickyCollision.orMaybe(stickyCollision);
  merged.stickyForce = rhs.stickyForce.orMaybe(stickyForce);
  merged.airFriction = rhs.airFriction.orMaybe(airFriction);
  merged.liquidFriction = rhs.liquidFriction.orMaybe(liquidFriction);
  merged.groundFriction = rhs.groundFriction.orMaybe(groundFriction);
  merged.collisionEnabled = rhs.collisionEnabled.orMaybe(collisionEnabled);
  merged.frictionEnabled = rhs.frictionEnabled.orMaybe(frictionEnabled);
  merged.gravityEnabled = rhs.gravityEnabled.orMaybe(gravityEnabled);
  merged.ignorePlatformCollision = rhs.ignorePlatformCollision.orMaybe(ignorePlatformCollision);
  merged.maximumPlatformCorrection = rhs.maximumPlatformCorrection.orMaybe(maximumPlatformCorrection);
  merged.maximumPlatformCorrectionVelocityFactor = rhs.maximumPlatformCorrectionVelocityFactor.orMaybe(maximumPlatformCorrectionVelocityFactor);
  merged.physicsEffectCategories = rhs.physicsEffectCategories.orMaybe(physicsEffectCategories);
  merged.restDuration = rhs.restDuration.orMaybe(restDuration);

  return merged;
}

Json MovementParameters::toJson() const {
  return JsonObject{
    {"mass", jsonFromMaybe(mass)},
    {"gravityMultiplier", jsonFromMaybe(gravityMultiplier)},
    {"liquidBuoyancy", jsonFromMaybe(liquidBuoyancy)},
    {"airBuoyancy", jsonFromMaybe(airBuoyancy)},
    {"stopOnFirstBounce", jsonFromMaybe(stopOnFirstBounce)},
    {"enableSurfaceSlopeCorrection", jsonFromMaybe(enableSurfaceSlopeCorrection)},
    {"slopeSlidingFactor", jsonFromMaybe(slopeSlidingFactor)},
    {"maxMovementPerStep", jsonFromMaybe(maxMovementPerStep)},
    {"maximumCorrection", jsonFromMaybe(maximumCorrection)},
    {"speedLimit", jsonFromMaybe(speedLimit)},
    {"discontinuityThreshold", jsonFromMaybe(discontinuityThreshold)},
    {"collisionPoly", jsonFromMaybe(collisionPoly, jsonFromPolyF)},
    {"stickyCollision", jsonFromMaybe(stickyCollision)},
    {"stickyForce", jsonFromMaybe(stickyForce)},
    {"airFriction", jsonFromMaybe(airFriction)},
    {"liquidFriction", jsonFromMaybe(liquidFriction)},
    {"groundFriction", jsonFromMaybe(groundFriction)},
    {"collisionEnabled", jsonFromMaybe(collisionEnabled)},
    {"frictionEnabled", jsonFromMaybe(frictionEnabled)},
    {"gravityEnabled", jsonFromMaybe(gravityEnabled)},
    {"ignorePlatformCollision", jsonFromMaybe(ignorePlatformCollision)},
    {"maximumPlatformCorrection", jsonFromMaybe(maximumPlatformCorrection)},
    {"maximumPlatformCorrectionVelocityFactor", jsonFromMaybe(maximumPlatformCorrectionVelocityFactor)},
    {"physicsEffectCategories", jsonFromMaybe(physicsEffectCategories, jsonFromStringSet)},
    {"restDuration", jsonFromMaybe(restDuration)}
  };
}

DataStream& operator>>(DataStream& ds, MovementParameters& movementParameters) {
  ds.read(movementParameters.mass);
  ds.read(movementParameters.gravityMultiplier);
  ds.read(movementParameters.liquidBuoyancy);
  ds.read(movementParameters.airBuoyancy);
  ds.read(movementParameters.stopOnFirstBounce);
  ds.read(movementParameters.enableSurfaceSlopeCorrection);
  ds.read(movementParameters.slopeSlidingFactor);
  ds.read(movementParameters.maxMovementPerStep);
  ds.read(movementParameters.maximumCorrection);
  ds.read(movementParameters.speedLimit);
  ds.read(movementParameters.discontinuityThreshold);
  ds.read(movementParameters.collisionPoly);
  ds.read(movementParameters.stickyCollision);
  ds.read(movementParameters.stickyForce);
  ds.read(movementParameters.airFriction);
  ds.read(movementParameters.liquidFriction);
  ds.read(movementParameters.groundFriction);
  ds.read(movementParameters.collisionEnabled);
  ds.read(movementParameters.frictionEnabled);
  ds.read(movementParameters.gravityEnabled);
  ds.read(movementParameters.ignorePlatformCollision);
  ds.read(movementParameters.maximumPlatformCorrection);
  ds.read(movementParameters.maximumPlatformCorrectionVelocityFactor);
  ds.read(movementParameters.physicsEffectCategories);
  ds.read(movementParameters.restDuration);

  return ds;
}

DataStream& operator<<(DataStream& ds, MovementParameters const& movementParameters) {
  ds.write(movementParameters.mass);
  ds.write(movementParameters.gravityMultiplier);
  ds.write(movementParameters.liquidBuoyancy);
  ds.write(movementParameters.airBuoyancy);
  ds.write(movementParameters.stopOnFirstBounce);
  ds.write(movementParameters.enableSurfaceSlopeCorrection);
  ds.write(movementParameters.slopeSlidingFactor);
  ds.write(movementParameters.maxMovementPerStep);
  ds.write(movementParameters.maximumCorrection);
  ds.write(movementParameters.speedLimit);
  ds.write(movementParameters.discontinuityThreshold);
  ds.write(movementParameters.collisionPoly);
  ds.write(movementParameters.stickyCollision);
  ds.write(movementParameters.stickyForce);
  ds.write(movementParameters.airFriction);
  ds.write(movementParameters.liquidFriction);
  ds.write(movementParameters.groundFriction);
  ds.write(movementParameters.collisionEnabled);
  ds.write(movementParameters.frictionEnabled);
  ds.write(movementParameters.gravityEnabled);
  ds.write(movementParameters.ignorePlatformCollision);
  ds.write(movementParameters.maximumPlatformCorrection);
  ds.write(movementParameters.maximumPlatformCorrectionVelocityFactor);
  ds.write(movementParameters.physicsEffectCategories);
  ds.write(movementParameters.restDuration);

  return ds;
}

MovementController::MovementController(MovementParameters const& parameters) {
  m_resting = false;

  m_timeStep = GlobalTimestep;

  m_liquidPercentage = 0.0f;
  m_liquidId = EmptyLiquidId;

  m_xPosition.setFixedPointBase(0.0125f);
  m_yPosition.setFixedPointBase(0.0125f);
  m_xVelocity.setFixedPointBase(0.00625f);
  m_yVelocity.setFixedPointBase(0.00625f);
  m_rotation.setFixedPointBase(0.01f);
  m_xRelativeSurfaceMovingCollisionPosition.setFixedPointBase(0.0125f);
  m_yRelativeSurfaceMovingCollisionPosition.setFixedPointBase(0.0125f);

  m_xVelocity.setInterpolator(lerp<float, float>);
  m_yVelocity.setInterpolator(lerp<float, float>);
  m_rotation.setInterpolator(angleLerp<float, float>);
  m_xRelativeSurfaceMovingCollisionPosition.setInterpolator(lerp<float, float>);
  m_yRelativeSurfaceMovingCollisionPosition.setInterpolator(lerp<float, float>);

  addNetElement(&m_collisionPoly);
  addNetElement(&m_mass);
  addNetElement(&m_xPosition);
  addNetElement(&m_yPosition);
  addNetElement(&m_xVelocity);
  addNetElement(&m_yVelocity);
  addNetElement(&m_rotation);
  addNetElement(&m_colliding);
  addNetElement(&m_collisionStuck);
  addNetElement(&m_nullColliding);
  addNetElement(&m_stickingDirection);
  addNetElement(&m_onGround);
  addNetElement(&m_zeroG);

  addNetElement(&m_surfaceMovingCollision);
  addNetElement(&m_xRelativeSurfaceMovingCollisionPosition);
  addNetElement(&m_yRelativeSurfaceMovingCollisionPosition);

  m_world = nullptr;

  resetParameters(parameters);
}

MovementParameters const& MovementController::parameters() const {
  return m_parameters;
}

void MovementController::applyParameters(MovementParameters const& parameters) {
  updateParameters(m_parameters.merge(parameters));
}

void MovementController::resetParameters(MovementParameters const& parameters) {
  updateParameters(MovementParameters::sensibleDefaults().merge(parameters));
}

Json MovementController::storeState() const {
  return JsonObject{
    {"position", jsonFromVec2F(position())},
    {"velocity", jsonFromVec2F(velocity())},
    {"rotation", rotation()}
  };
}

void MovementController::loadState(Json const& state) {
  setPosition(jsonToVec2F(state.get("position")));
  setVelocity(jsonToVec2F(state.get("velocity")));
  setRotation(state.getFloat("rotation"));
}

float MovementController::mass() const {
  return m_mass.get();
}

PolyF const& MovementController::collisionPoly() const {
  return m_collisionPoly.get();
}

void MovementController::setCollisionPoly(PolyF const& poly) {
  m_collisionPoly.set(poly);
}

Vec2F MovementController::position() const {
  return {m_xPosition.get(), m_yPosition.get()};
}

float MovementController::xPosition() const {
  return m_xPosition.get();
}

float MovementController::yPosition() const {
  return m_yPosition.get();
}

Vec2F MovementController::velocity() const {
  return {m_xVelocity.get(), m_yVelocity.get()};
}

float MovementController::xVelocity() const {
  return m_xVelocity.get();
}

float MovementController::yVelocity() const {
  return m_yVelocity.get();
}

float MovementController::rotation() const {
  return m_rotation.get();
}

PolyF MovementController::collisionBody() const {
  auto poly = collisionPoly();
  poly.rotate(rotation());
  poly.translate(position());
  return poly;
}

RectF MovementController::localBoundBox() const {
  auto poly = collisionPoly();
  poly.rotate(rotation());
  return poly.boundBox();
}

RectF MovementController::collisionBoundBox() const {
  return collisionBody().boundBox();
}

bool MovementController::isColliding() const {
  return m_colliding.get();
}

bool MovementController::isNullColliding() const {
  return m_nullColliding.get();
}

bool MovementController::isCollisionStuck() const {
  return m_collisionStuck.get();
}

Maybe<float> MovementController::stickingDirection() const {
  return m_stickingDirection.get();
}

float MovementController::liquidPercentage() const {
  return m_liquidPercentage;
}

LiquidId MovementController::liquidId() const {
  return m_liquidId;
}

bool MovementController::onGround() const {
  return m_onGround.get();
}

bool MovementController::zeroG() const {
  return m_zeroG.get();
}

bool MovementController::atWorldLimit(bool bottomOnly) const {
  if (m_world) {
    if (!collisionPoly().isNull()) {
      auto bounds = collisionBoundBox();
      return bounds.yMin() <= 0 || (!bottomOnly && bounds.yMax() >= m_world->geometry().height());
    } else {
      return yPosition() <= 0 || (!bottomOnly && yPosition() >= m_world->geometry().height());
    }
  }
  return false;
}

void MovementController::setPosition(Vec2F position) {
  if (m_world)
    position = m_world->geometry().limit(position);

  if (position[0] != m_xPosition.get() || position[1] != m_yPosition.get())
    m_resting = false;

  m_xPosition.set(position[0]);
  m_yPosition.set(position[1]);
}

void MovementController::setXPosition(float xPosition) {
  setPosition({xPosition, yPosition()});
}

void MovementController::setYPosition(float yPosition) {
  setPosition({xPosition(), yPosition});
}

void MovementController::translate(Vec2F const& direction) {
  setPosition(position() + direction);
}

void MovementController::setVelocity(Vec2F vel) {
  if (m_parameters.speedLimit && vel.magnitude() > *m_parameters.speedLimit) {
    vel.normalize();
    vel *= *m_parameters.speedLimit;
  }

  if ((velocity() - vel).magnitude() > 0.0001)
    m_resting = false;

  m_xVelocity.set(vel[0]);
  m_yVelocity.set(vel[1]);
}

void MovementController::setXVelocity(float xVelocity) {
  setVelocity({xVelocity, yVelocity()});
}

void MovementController::setYVelocity(float yVelocity) {
  setVelocity({xVelocity(), yVelocity});
}

void MovementController::addMomentum(Vec2F const& momentum) {
  setVelocity(velocity() + momentum / mass());
}

void MovementController::setRotation(float rotation) {
  m_resting = false;

  m_rotation.set(rotation);
}

void MovementController::rotate(float rotationRate) {
  if (rotationRate == 0.0)
    return;

  m_resting = false;
  m_rotation.set(fmod(rotation() + rotationRate * m_timeStep, 2 * Constants::pi));
}

void MovementController::accelerate(Vec2F const& acceleration) {
  setVelocity(velocity() + acceleration * m_timeStep);
}

void MovementController::force(Vec2F const& force) {
  setVelocity(velocity() + force / mass() * m_timeStep);
}

void MovementController::approachVelocity(Vec2F const& targetVelocity, float maxControlForce) {
  // Instead of applying the force directly, work backwards and figure out the
  // maximum acceleration that could be achieved by the current control force,
  // and maximize the change in velocity based on that.

  Vec2F diff = targetVelocity - velocity();
  float diffMagnitude = vmag(diff);

  if (diffMagnitude == 0.0f)
    return;

  float maximumAcceleration = maxControlForce / mass() * m_timeStep;
  float clampedMagnitude = clamp(diffMagnitude, 0.0f, maximumAcceleration);

  setVelocity(velocity() + diff * (clampedMagnitude / diffMagnitude));
}

void MovementController::approachVelocityAlongAngle(float angle, float targetVelocity, float maxControlForce, bool positiveOnly) {
  // Same strategy as approachVelocity, work backwards to figure out the
  // maximum acceleration and apply that.

  // Project the current velocity along the axis normal, the velocity
  // difference is the difference between the targetVelocity and this
  // projection.

  Vec2F axis = Vec2F::withAngle(angle, 1.0f);

  float velocityAlongAxis = velocity() * axis;
  float diff = targetVelocity - velocityAlongAxis;
  if (diff == 0.0f)
    return;
  if (positiveOnly && diff < 0)
    return;

  float maximumAcceleration = maxControlForce / mass() * m_timeStep;

  float diffMagnitude = std::fabs(diff);
  float clampedMagnitude = clamp(diffMagnitude, 0.0f, maximumAcceleration);

  setVelocity(velocity() + axis * diff * (clampedMagnitude / diffMagnitude));
}

void MovementController::approachXVelocity(float targetXVelocity, float maxControlForce) {
  approachVelocityAlongAngle(0.0f, targetXVelocity, maxControlForce);
}

void MovementController::approachYVelocity(float targetYVelocity, float maxControlForce) {
  approachVelocityAlongAngle(Constants::pi / 2, targetYVelocity, maxControlForce);
}

void MovementController::init(World* world) {
  m_world = world;
  setPosition(position());
  updatePositionInterpolators();
}

void MovementController::uninit() {
  m_world = nullptr;
  updatePositionInterpolators();
}

void MovementController::setTimestep(float dt) {
  m_timeStep = dt;
}

void MovementController::tickMaster(float dt) {
  setTimestep(dt);
  auto geometry = world()->geometry();

  m_zeroG.set(!*m_parameters.gravityEnabled || *m_parameters.gravityMultiplier == 0 || world()->gravity(position()) == 0);

  Maybe<PhysicsMovingCollision> surfaceCollision;
  if (auto movingCollisionId = m_surfaceMovingCollision.get()) {
    if (auto physicsEntity = world()->get<PhysicsEntity>(movingCollisionId->physicsEntityId))
      surfaceCollision = physicsEntity->movingCollision(movingCollisionId->collisionIndex);
  }

  if (surfaceCollision) {
    Vec2F surfacePositionDelta = geometry.diff(surfaceCollision->position, m_surfaceMovingCollisionPosition);
    setPosition(position() + surfacePositionDelta);
    Vec2F newSurfaceVelocity = surfacePositionDelta / dt;
    setVelocity(velocity() - m_surfaceVelocity + newSurfaceVelocity);
    m_surfaceVelocity = newSurfaceVelocity;
  } else {
    m_surfaceMovingCollision.set({});
    m_surfaceMovingCollisionPosition = {};
    m_surfaceVelocity = {};
  }

  if (m_resting) {
    m_restTicks -= 1;
    if (m_restTicks < 0) {
      m_resting = false;
    }
  }

  // don't integrate velocity when resting
  Vec2F relativeVelocity = m_resting ? Vec2F() : velocity();
  Vec2F originalMovement = relativeVelocity * dt;
  if (surfaceCollision)
    relativeVelocity -= m_surfaceVelocity;

  m_collisionCorrection = {};
  m_surfaceSlope = Vec2F(1, 0);
  m_surfaceMovingCollision.set({});

  unsigned steps;
  float maxMovementPerStep = *m_parameters.maxMovementPerStep;
  if (maxMovementPerStep > 0.0f)
    steps = std::floor(vmag(relativeVelocity) * dt / maxMovementPerStep) + 1;
  else
    steps = 1;

  // skip collision checks when resting (there's no movement anyway)
  if (m_resting)
    steps = 0;

  for (unsigned i = 0; i < steps; ++i) {
    float dtSteps = dt / steps;
    Vec2F movement = relativeVelocity * dtSteps;

    if (!*m_parameters.collisionEnabled || collisionPoly().isNull()) {
      setPosition(position() + movement);
      m_surfaceSlope = Vec2F(1, 0);
      m_surfaceVelocity = Vec2F(0, 0);

      m_colliding.set(false);
      m_collisionStuck.set(false);
      m_nullColliding.set(false);
      m_stickingDirection.set({});
      m_onGround.set(false);

    } else {
      auto body = collisionBody();

      float velocityMagnitude = vmag(relativeVelocity);
      Vec2F velocityDirection = relativeVelocity / velocityMagnitude;

      bool ignorePlatforms = *m_parameters.ignorePlatformCollision || relativeVelocity[1] > 0;
      float maximumCorrection = *m_parameters.maximumCorrection;
      float maximumPlatformCorrection = *m_parameters.maximumPlatformCorrection
          + *m_parameters.maximumPlatformCorrectionVelocityFactor * velocityMagnitude;
      Vec2F bodyCenter = body.center();

      RectF queryBounds = body.boundBox().padded(maximumCorrection);
      queryBounds.combine(queryBounds.translated(movement));
      queryCollisions(queryBounds);
      auto result = collisionMove(m_workingCollisions, body, movement, ignorePlatforms, *m_parameters.enableSurfaceSlopeCorrection && !zeroG(),
          maximumCorrection, maximumPlatformCorrection, bodyCenter, dtSteps);

      setPosition(position() + result.movement);

      if (result.collisionKind == CollisionKind::Null) {
        m_nullColliding.set(true);
        break;
      } else {
        m_nullColliding.set(false);
      }

      Vec2F correction = result.correction;
      Vec2F normCorrection = vnorm(correction);

      m_surfaceSlope = result.groundSlope;
      m_surfaceMovingCollision.set(result.surfaceMovingCollisionId);
      m_collisionCorrection += correction;
      m_colliding.set(correction != Vec2F() || result.isStuck);
      m_onGround.set(!zeroG() && result.onGround);
      m_collisionStuck.set(result.isStuck);

      // If we have collided, apply either sticky or normal (bouncing) collision physics
      if (correction != Vec2F()) {
        if (*m_parameters.stickyCollision && result.collisionKind != CollisionKind::Slippery) {
          // When sticking, cancel all velocity and apply stickyForce in the
          // opposite of the direction of collision correction.
          relativeVelocity = -normCorrection * *m_parameters.stickyForce / mass() * dtSteps;
          m_stickingDirection.set(-normCorrection.angle());
          break;
        } else {
          m_stickingDirection.set({});

          float correctionMagnitude = vmag(correction);
          Vec2F correctionDirection = correction / correctionMagnitude;

          if (*m_parameters.bounceFactor != 0.0f) {
            Vec2F adjustment = correctionDirection * (velocityMagnitude * (correctionDirection * -velocityDirection));
            relativeVelocity += adjustment + *m_parameters.bounceFactor * adjustment;
            if (*m_parameters.stopOnFirstBounce) {
              // When bouncing, stop integrating at the moment of bounce.  This
              // prevents the frame of contact from being missed due to multiple
              // iterations per frame.
              break;
            }
          } else {
            // Only adjust the velocity to the extent that the collision was
            // caused by the velocity in each axis, to eliminate collision
            // induced velocity in a platformery way (each axis considered
            // independently).

            if (relativeVelocity[0] < 0 && correction[0] > 0)
              relativeVelocity[0] = min(0.0f, relativeVelocity[0] + correction[0] / dt);
            else if (relativeVelocity[0] > 0 && correction[0] < 0)
              relativeVelocity[0] = max(0.0f, relativeVelocity[0] + correction[0] / dt);

            if (relativeVelocity[1] < 0 && correction[1] > 0)
              relativeVelocity[1] = min(0.0f, relativeVelocity[1] + correction[1] / dt);
            else if (relativeVelocity[1] > 0 && correction[1] < 0)
              relativeVelocity[1] = max(0.0f, relativeVelocity[1] + correction[1] / dt);
          }
        }
      }
    }
  }
  
  Vec2F newVelocity = relativeVelocity + m_surfaceVelocity;

  PolyF body = collisionBody();

  updateLiquidPercentage();

  if (auto movingCollisionId = m_surfaceMovingCollision.get()) {
    if (auto physicsEntity = world()->get<PhysicsEntity>(movingCollisionId->physicsEntityId))
      surfaceCollision = physicsEntity->movingCollision(movingCollisionId->collisionIndex);
  }

  if (surfaceCollision) {
    m_surfaceMovingCollisionPosition = surfaceCollision->position;
    m_xRelativeSurfaceMovingCollisionPosition.set(geometry.diff(xPosition(), m_surfaceMovingCollisionPosition[0]));
    m_yRelativeSurfaceMovingCollisionPosition.set(yPosition() - m_surfaceMovingCollisionPosition[1]);
  } else {
    m_surfaceMovingCollisionPosition = {};
    m_surfaceVelocity = {};
  }
  
  // In order to make control work accurately, passive forces need to be
  // applied to velocity *after* integrating.  This prevents control from
  // having to account for one timestep of passive forces in order to result
  // in the correct controlled movement.
  if (!zeroG() && !stickingDirection()) {
    float buoyancy = *m_parameters.liquidBuoyancy * m_liquidPercentage + *m_parameters.airBuoyancy * (1.0f - liquidPercentage());
    float gravity = world()->gravity(position()) * *m_parameters.gravityMultiplier * (1.0f - buoyancy);
    Vec2F environmentVelocity;
    environmentVelocity[1] -= gravity * dt;

    if (onGround() && *m_parameters.slopeSlidingFactor != 0 && m_surfaceSlope[1] != 0)
      environmentVelocity += -m_surfaceSlope * (m_surfaceSlope[0] * m_surfaceSlope[1]) * *m_parameters.slopeSlidingFactor;

    newVelocity += environmentVelocity;
  }

  // If original movement was entirely (almost) in the direction of gravity
  // and was entirely (almost) cancelled by collision correction, put the
  // entity into rest for restDuration
  if (!m_resting &&
      abs(originalMovement[0]) < 0.0001 &&
      originalMovement[1] * gravity() <= 0.0 &&
      abs(originalMovement[1] + m_collisionCorrection[1]) < 0.0001) {
    m_resting = true;
    m_restTicks = m_parameters.restDuration.value(0);
  }

  if (*m_parameters.frictionEnabled) {
    Vec2F refVel;
    float friction = liquidPercentage() * *m_parameters.liquidFriction + (1.0f - liquidPercentage()) * *m_parameters.airFriction;
    if (onGround()) {
      friction = max(friction, *m_parameters.groundFriction);
      refVel = m_surfaceVelocity;
    }

    // The equation for friction here is effectively:
    // frictionForce = friction * (refVel - velocity)
    // but it is applied here as a multiplicitave factor from [0, 1] so it does
    // not induce oscillation at very high friction and so it cannot be
    // negative.
    float frictionFactor = clamp(friction / mass() * dt, 0.0f, 1.0f);
    newVelocity = lerp(frictionFactor, newVelocity, refVel);
  }
  
  setVelocity(newVelocity);

  updateForceRegions(dt);
}

void MovementController::tickSlave(float dt) {
  setTimestep(dt);
  if (auto movingCollisionId = m_surfaceMovingCollision.get()) {
    if (auto physicsEntity = world()->get<PhysicsEntity>(movingCollisionId->physicsEntityId)) {
      if (auto collision = physicsEntity->movingCollision(movingCollisionId->collisionIndex)) {
        m_xPosition.set(m_xRelativeSurfaceMovingCollisionPosition.get() + collision->position[0]);
        m_yPosition.set(m_yRelativeSurfaceMovingCollisionPosition.get() + collision->position[1]);
      }
    }
  }

  LiquidLevel cll = world()->liquidLevel(collisionBody().boundBox());
  m_liquidPercentage = clamp(cll.level, 0.0f, 1.0f);
  m_liquidId = cll.liquid;
}

void MovementController::setIgnorePhysicsEntities(Set<EntityId> ignorePhysicsEntities) {
  m_ignorePhysicsEntities = ignorePhysicsEntities;
}

void MovementController::forEachMovingCollision(RectF const& region, function<bool(MovingCollisionId id, PhysicsMovingCollision, PolyF, RectF)> callback) {
  auto geometry = world()->geometry();
  for (auto& physicsEntity : world()->query<PhysicsEntity>(region)) {
    if (m_ignorePhysicsEntities.contains(physicsEntity->entityId()))
      continue;
    for (size_t i = 0; i < physicsEntity->movingCollisionCount(); ++i) {
      if (auto mc = physicsEntity->movingCollision(i)) {
        if (mc->categoryFilter.check(m_parameters.physicsEffectCategories.value())) {
          PolyF poly = std::move(mc->collision);
          poly.translate(geometry.nearestTo(region.min(), mc->position));
          RectF polyBounds = poly.boundBox();

          if (region.intersects(polyBounds)) {
            // early exit if the callback returns false
            if(callback({physicsEntity->entityId(), i}, *mc, poly, polyBounds) == false)
              return;
          }
        }
      }
    }
  }
}

void MovementController::updateForceRegions(float) {
  auto geometry = world()->geometry();
  auto pos = position();
  auto body = collisionBody();
  RectF boundBox = body.boundBox();

  m_appliedForceRegion = false;
  auto handleForceRegions = [&](List<PhysicsForceRegion> const& forces) {
      for (auto const& force : forces) {
        bool categoryCheck = force.call([myCategories = m_parameters.physicsEffectCategories.value()](auto& fr) {
            return fr.categoryFilter.check(myCategories);
          });
        if (!categoryCheck)
          continue;

        bool boundsCheck = force.call([geometry, myBounds = collisionBoundBox()](auto& fr) {
            return geometry.rectIntersectsRect(myBounds, fr.boundBox());
          });
        if (!boundsCheck)
          continue;

        m_appliedForceRegion = true;
        if (auto directionalForceRegion = force.ptr<DirectionalForceRegion>()) {
          float forceEffect = geometry.polyOverlapArea(directionalForceRegion->region, body) / body.convexArea();
          if (directionalForceRegion->xTargetVelocity)
            approachXVelocity(*directionalForceRegion->xTargetVelocity, directionalForceRegion->controlForce * forceEffect);
          if (directionalForceRegion->yTargetVelocity)
            approachYVelocity(*directionalForceRegion->yTargetVelocity, directionalForceRegion->controlForce * forceEffect);

        } else if (auto radialForceRegion = force.ptr<RadialForceRegion>()) {
          Vec2F direction = geometry.diff(pos, radialForceRegion->center);
          float distance = vmag(direction);
          if (distance > 0 && distance < radialForceRegion->outerRadius) {
            float incidence = min(1.0f - (distance - radialForceRegion->innerRadius) / (radialForceRegion->outerRadius - radialForceRegion->innerRadius), distance / radialForceRegion->innerRadius);
            if (radialForceRegion->targetRadialVelocity < 0)
              direction = -direction;
            approachVelocityAlongAngle(direction.angle(),
                abs(radialForceRegion->targetRadialVelocity),
                radialForceRegion->controlForce * incidence,
                true);
          }
        } else if (auto gradientForceRegion = force.ptr<GradientForceRegion>()) {
          float overlapFactor = geometry.polyOverlapArea(gradientForceRegion->region, body) / body.convexArea();

          Vec2F gNorm = gradientForceRegion->gradient.direction();
          Vec2F pDiff = geometry.diff(pos, gradientForceRegion->gradient.min());
          float projected = pDiff[0] * gNorm[0] + pDiff[1] * gNorm[1];
          float gradientFactor = 1.0 - clamp(projected / gradientForceRegion->gradient.length(), -1.0f, 1.0f);

          approachVelocityAlongAngle(gradientForceRegion->gradient.angle(),
              gradientForceRegion->baseTargetVelocity * overlapFactor * gradientFactor,
              gradientForceRegion->baseControlForce * overlapFactor * gradientFactor,
              true);
        }
      }
    };

  for (auto& physicsEntity : world()->query<PhysicsEntity>(boundBox)) {
    if (m_ignorePhysicsEntities.contains(physicsEntity->entityId()))
      continue;

    handleForceRegions(physicsEntity->forceRegions());
  }

  handleForceRegions(world()->forceRegions());
}

void MovementController::updateLiquidPercentage() {
  auto pos = position();
  auto body = collisionBody();
  RectF boundBox = body.boundBox();

  LiquidLevel cll;
  if (boundBox.isEmpty())
    cll = world()->liquidLevel(Vec2I::floor(pos));
  else
    cll = world()->liquidLevel(boundBox);

  m_liquidPercentage = clamp(cll.level, 0.0f, 1.0f);
  m_liquidId = cll.liquid;
}

void MovementController::setOnGround(bool onGround) {
  m_onGround.set(onGround);
}

bool MovementController::appliedForceRegion() const {
  return m_appliedForceRegion;
}

Vec2F MovementController::collisionCorrection() const {
  return m_collisionCorrection;
}

Vec2F MovementController::surfaceSlope() const {
  return m_surfaceSlope;
}

Vec2F MovementController::surfaceVelocity() const {
  return m_surfaceVelocity;
}

World* MovementController::world() {
  if (!m_world)
    throw MovementControllerException("MovementController not initialized!");
  return m_world;
}

CollisionKind MovementController::maxOrNullCollision(CollisionKind a, CollisionKind b) {
  if (a == CollisionKind::Null || b == CollisionKind::Null)
    return CollisionKind::Null;
  else
    return max(a, b);
}

MovementController::CollisionResult MovementController::collisionMove(List<CollisionPoly>& collisionPolys, PolyF const& body, Vec2F const& movement,
    bool ignorePlatforms, bool enableSurfaceSlopeCorrection, float maximumCorrection, float maximumPlatformCorrection, Vec2F sortCenter, float dt) {
  unsigned const MaximumSeparationLoops = 3;
  float const SlideAngle = Constants::pi / 3;
  float const SlideCorrectionLimit = 0.2f;
  float separationTolerance = 0.001f * (dt * 60.f);
  maximumPlatformCorrection *= (dt * 60.f);

  if (body.isNull())
    return {movement, Vec2F(), {}, false, false, Vec2F(1, 0), CollisionKind::None};

  PolyF translatedBody = body;
  translatedBody.translate(movement);
  PolyF checkBody = translatedBody;
  Vec2F totalCorrection = Vec2F();
  CollisionKind maxCollided = CollisionKind::None;
  Maybe<MovingCollisionId> surfaceMovingCollisionId;

  CollisionSeparation separation = {};

  if (enableSurfaceSlopeCorrection) {
    // First try separating with our ground sliding cheat.
    separation = collisionSeparate(collisionPolys, checkBody, ignorePlatforms, maximumPlatformCorrection, sortCenter, true, separationTolerance);
    totalCorrection += separation.correction;
    checkBody.translate(separation.correction);
    maxCollided = maxOrNullCollision(maxCollided, separation.collisionKind);
    surfaceMovingCollisionId = separation.movingCollisionId;
    Vec2F upwardResult = movement + separation.correction;
    float upMag = upwardResult.magnitude();
    // Angle off of horizontal (minimum of either direction)
    float angleHoriz = std::min(Vec2F(1, 0).angleBetweenNormalized(upwardResult / upMag),
        Vec2F(-1, 0).angleBetweenNormalized(upwardResult / upMag));

    // We need to make sure that even if we found a solution with the sliding
    // cheat, we are not beyond the angle and correction limits for the ground
    // cheat correction.
    if (separation.solutionFound)
      separation.solutionFound = upMag < SlideCorrectionLimit || angleHoriz < SlideAngle;

    if (separation.solutionFound) {
      if (totalCorrection.magnitude() > maximumCorrection)
        separation.solutionFound = false;
    }
  }

  if (!separation.solutionFound) {
    checkBody = translatedBody;
    totalCorrection = Vec2F();
    for (size_t i = 0; i < MaximumSeparationLoops; ++i) {
      separation = collisionSeparate(collisionPolys, checkBody, ignorePlatforms,
          maximumPlatformCorrection, sortCenter, false, separationTolerance);
      totalCorrection += separation.correction;
      checkBody.translate(separation.correction);
      maxCollided = maxOrNullCollision(maxCollided, separation.collisionKind);
      surfaceMovingCollisionId = {};

      if (totalCorrection.magnitude() > maximumCorrection) {
        separation.solutionFound = false;
        break;
      }

      if (separation.solutionFound)
        break;
    }
  }

  if (!separation.solutionFound && movement != Vec2F()) {
    // No collision solution found! Move checkBody back to original body before
    // applying movement and try one last time to correct.
    checkBody = body;
    totalCorrection = -movement;
    for (size_t i = 0; i < MaximumSeparationLoops; ++i) {
      separation = collisionSeparate(collisionPolys, checkBody, true, maximumPlatformCorrection, sortCenter, false, separationTolerance);
      totalCorrection += separation.correction;
      checkBody.translate(separation.correction);
      maxCollided = maxOrNullCollision(maxCollided, separation.collisionKind);

      if (totalCorrection.magnitude() > maximumCorrection) {
        separation.solutionFound = false;
        break;
      }

      if (separation.solutionFound)
        break;
    }
  }

  if (separation.solutionFound) {
    CollisionResult result;
    result.movement = movement + totalCorrection;
    result.correction = totalCorrection;
    result.isStuck = false;
    result.onGround = result.correction[1] > separationTolerance;
    result.surfaceMovingCollisionId = surfaceMovingCollisionId;
    result.collisionKind = maxCollided;
    result.groundSlope = Vec2F(1, 0);
    if (result.onGround) {
      // If we are on the ground and need to find the ground slope, look for a
      // vertex on the body being moved that is touching an edge of one of the
      // collision polys.  We only want a slope to be produced from an edge of
      // colision geometry, not an edge of the colliding body.  Pick the
      // touching edge that is the most horizontally overlapped with the
      // geometry, rather than off to the side.
      float maxSideHorizontalOverlap = 0.0f;
      RectF touchingBounds = checkBody.boundBox();
      touchingBounds.pad(separationTolerance);
      for (auto const& cp : collisionPolys) {
        if (!cp.polyBounds.intersects(touchingBounds))
          continue;

        for (size_t i = 0; i < cp.poly.sides(); ++i) {
          auto side = cp.poly.side(i);
          RectF sideBounds = RectF::boundBoxOf(side.min(), side.max());
          float thisSideHorizontalOverlap = sideBounds.overlap(touchingBounds).width();

          if (thisSideHorizontalOverlap > maxSideHorizontalOverlap) {
            for (auto const& bodyVertex : checkBody) {
              float t = clamp(side.lineProjection(bodyVertex), 0.0f, 1.0f);
              Vec2F nearPoint = side.eval(t);
              if (nearPoint[1] > cp.sortPosition[1]) {
                if (vmagSquared(bodyVertex - nearPoint) <= square(separationTolerance)) {
                  maxSideHorizontalOverlap = thisSideHorizontalOverlap;
                  result.groundSlope = side.diff().normalized();
                  if (result.groundSlope[0] < 0)
                    result.groundSlope *= -1;
                }
              }
            }
          }
        }
      }
    }

    return result;
  } else {
    return {Vec2F(), -movement, {}, true, true, Vec2F(1, 0), maxCollided};
  }
}

MovementController::CollisionSeparation MovementController::collisionSeparate(List<CollisionPoly>& collisionPolys, PolyF const& poly,
    bool ignorePlatforms, float maximumPlatformCorrection, Vec2F const& sortCenter, bool upward, float separationTolerance) {

  CollisionSeparation separation = {};
  separation.collisionKind = CollisionKind::None;
  bool intersects = false;

  for (auto& cp : collisionPolys)
    cp.sortDistance = vmagSquared(cp.sortPosition - sortCenter);

  sort(collisionPolys, [](auto const& a, auto const& b) {
      return a.sortDistance < b.sortDistance;
    });

  PolyF::IntersectResult intersectResult;
  PolyF correctedPoly = poly;
  RectF correctedBoundBox = correctedPoly.boundBox();
  for (auto const& cp : collisionPolys) {
    if ((ignorePlatforms && cp.collisionKind == CollisionKind::Platform) || !correctedBoundBox.intersects(cp.polyBounds, false))
      continue;

    if (upward)
      intersectResult = correctedPoly.directionalSatIntersection(cp.poly, Vec2F(0, 1), false);
    else if (cp.collisionKind == CollisionKind::Platform)
      intersectResult = correctedPoly.directionalSatIntersection(cp.poly, Vec2F(0, 1), true);
    else
      intersectResult = correctedPoly.satIntersection(cp.poly);

    if (cp.collisionKind == CollisionKind::Platform && intersectResult.intersects) {
      if (intersectResult.overlap[1] <= 0 || intersectResult.overlap[1] > maximumPlatformCorrection)
        intersectResult.intersects = false;
    }

    if (intersectResult.intersects) {
      intersects = true;
      correctedPoly.translate(intersectResult.overlap);
      correctedBoundBox = correctedPoly.boundBox();
      separation.correction += intersectResult.overlap;
      if (cp.movingCollisionId)
        separation.movingCollisionId = cp.movingCollisionId;
      separation.collisionKind = maxOrNullCollision(separation.collisionKind, cp.collisionKind);
    }
  }

  separation.solutionFound = true;
  float separationToleranceSquared = square(separationTolerance);
  if (intersects) {
    for (auto const& cp : collisionPolys) {
      if (cp.collisionKind == CollisionKind::Platform || !correctedBoundBox.intersects(cp.polyBounds, false))
        continue;

      intersectResult = correctedPoly.satIntersection(cp.poly);
      if (intersectResult.intersects && intersectResult.overlap.magnitudeSquared() > separationToleranceSquared) {
        separation.collisionKind = maxOrNullCollision(separation.collisionKind, cp.collisionKind);
        separation.solutionFound = false;
        break;
      }
    }
  }

  return separation;
}

void MovementController::updateParameters(MovementParameters parameters) {
  m_parameters = std::move(parameters);
  m_collisionPoly.set(*m_parameters.collisionPoly);
  m_mass.set(*m_parameters.mass);
  updatePositionInterpolators();
}

void MovementController::updatePositionInterpolators() {
  if (m_world)
    m_xPosition.setInterpolator(m_world->geometry().xLerpFunction(m_parameters.discontinuityThreshold));
  else
    m_xPosition.setInterpolator(bind(lerpWithLimit<float, float>, m_parameters.discontinuityThreshold, _1, _2, _3));
  m_yPosition.setInterpolator(bind(lerpWithLimit<float, float>, m_parameters.discontinuityThreshold, _1, _2, _3));
}

void MovementController::queryCollisions(RectF const& region) {
  while (!m_workingCollisions.empty()) {
    m_collisionBuffers.append(m_workingCollisions.takeLast().poly);
  }

  auto newCollisionPoly = [this]() -> CollisionPoly& {
    if (!m_collisionBuffers.empty())
      return m_workingCollisions.emplaceAppend(CollisionPoly{
          m_collisionBuffers.takeLast(), {}, {}, {}, {}, {}
        });
    else
      return m_workingCollisions.emplaceAppend(CollisionPoly{});
  };

  auto geometry = world()->geometry();

  world()->forEachCollisionBlock(RectI::integral(region.padded(1)), [&](CollisionBlock const& block) {
      if (block.kind != CollisionKind::None && !block.poly.isNull()) {
        RectF polyBounds = block.polyBounds;
        Vec2F basePosition = block.poly.vertex(0);
        Vec2F nearTranslation = geometry.nearestTo(region.min(), basePosition) - basePosition;
        polyBounds.translate(nearTranslation);

        if (region.intersects(polyBounds)) {
          CollisionPoly& collisionPoly = newCollisionPoly();
          collisionPoly.poly = block.poly;
          collisionPoly.poly.translate(nearTranslation);
          collisionPoly.polyBounds = polyBounds;
          collisionPoly.sortPosition = centerOfTile(block.space);
          collisionPoly.movingCollisionId = {};
          collisionPoly.collisionKind = block.kind;
        }
      }
    });

  forEachMovingCollision(region, [&](MovingCollisionId id, PhysicsMovingCollision mc, PolyF poly, RectF bounds) {
    CollisionPoly& collisionPoly = newCollisionPoly();
    collisionPoly.poly = std::move(poly);
    collisionPoly.polyBounds = bounds;
    collisionPoly.sortPosition = collisionPoly.poly.center();
    collisionPoly.movingCollisionId = id;
    collisionPoly.collisionKind = mc.collisionKind;
    return true;
  });
}

float MovementController::gravity() {
  float buoyancy = *m_parameters.liquidBuoyancy * m_liquidPercentage + *m_parameters.airBuoyancy * (1.0f - liquidPercentage());
  return world()->gravity(position()) * *m_parameters.gravityMultiplier * (1.0f - buoyancy);
}

}