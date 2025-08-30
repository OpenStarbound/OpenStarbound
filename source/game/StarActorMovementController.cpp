#include "StarActorMovementController.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarPlatformerAStar.hpp"
#include "StarObject.hpp"

namespace Star {

ActorJumpProfile::ActorJumpProfile() {}

ActorJumpProfile::ActorJumpProfile(Json const& config) {
  jumpSpeed = config.optFloat("jumpSpeed");
  jumpControlForce = config.optFloat("jumpControlForce");
  jumpInitialPercentage = config.optFloat("jumpInitialPercentage");
  jumpHoldTime = config.optFloat("jumpHoldTime");
  jumpTotalHoldTime = config.optFloat("jumpTotalHoldTime");
  multiJump = config.optBool("multiJump");
  reJumpDelay = config.optFloat("reJumpDelay");
  autoJump = config.optBool("autoJump");
  collisionCancelled = config.optBool("collisionCancelled");
}

Json ActorJumpProfile::toJson() const {
  return JsonObject{
    {"jumpSpeed", jsonFromMaybe(jumpSpeed)},
    {"jumpControlForce", jsonFromMaybe(jumpControlForce)},
    {"jumpInitialPercentage", jsonFromMaybe(jumpInitialPercentage)},
    {"jumpHoldTime", jsonFromMaybe(jumpHoldTime)},
    {"jumpTotalHoldTime", jsonFromMaybe(jumpTotalHoldTime)},
    {"multiJump", jsonFromMaybe(multiJump)},
    {"reJumpDelay", jsonFromMaybe(reJumpDelay)},
    {"autoJump", jsonFromMaybe(autoJump)},
    {"collisionCancelled", jsonFromMaybe(collisionCancelled)}
  };
}

ActorJumpProfile ActorJumpProfile::merge(ActorJumpProfile const& rhs) const {
  ActorJumpProfile merged;

  merged.jumpSpeed = rhs.jumpSpeed.orMaybe(jumpSpeed);
  merged.jumpControlForce = rhs.jumpControlForce.orMaybe(jumpControlForce);
  merged.jumpInitialPercentage = rhs.jumpInitialPercentage.orMaybe(jumpInitialPercentage);
  merged.jumpHoldTime = rhs.jumpHoldTime.orMaybe(jumpHoldTime);
  merged.jumpTotalHoldTime = rhs.jumpTotalHoldTime.orMaybe(jumpTotalHoldTime);
  merged.multiJump = rhs.multiJump.orMaybe(multiJump);
  merged.reJumpDelay = rhs.reJumpDelay.orMaybe(reJumpDelay);
  merged.autoJump = rhs.autoJump.orMaybe(autoJump);
  merged.collisionCancelled = rhs.collisionCancelled.orMaybe(collisionCancelled);

  return merged;
}

DataStream& operator>>(DataStream& ds, ActorJumpProfile& movementParameters) {
  ds.read(movementParameters.jumpSpeed);
  ds.read(movementParameters.jumpControlForce);
  ds.read(movementParameters.jumpInitialPercentage);
  ds.read(movementParameters.jumpHoldTime);
  ds.read(movementParameters.jumpTotalHoldTime);
  ds.read(movementParameters.multiJump);
  ds.read(movementParameters.reJumpDelay);
  ds.read(movementParameters.autoJump);
  ds.read(movementParameters.collisionCancelled);

  return ds;
}

DataStream& operator<<(DataStream& ds, ActorJumpProfile const& movementParameters) {
  ds.write(movementParameters.jumpSpeed);
  ds.write(movementParameters.jumpControlForce);
  ds.write(movementParameters.jumpInitialPercentage);
  ds.write(movementParameters.jumpHoldTime);
  ds.write(movementParameters.jumpTotalHoldTime);
  ds.write(movementParameters.multiJump);
  ds.write(movementParameters.reJumpDelay);
  ds.write(movementParameters.autoJump);
  ds.write(movementParameters.collisionCancelled);

  return ds;
}

ActorMovementParameters ActorMovementParameters::sensibleDefaults() {
  return ActorMovementParameters(Root::singleton().assets()->json("/default_actor_movement.config").toObject());
}

ActorMovementParameters::ActorMovementParameters(Json const& config) {
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

  // "collisionPoly" is used as a synonym for setting both the standing and
  // crouching polys.

  auto collisionPolyConfig = config.get("collisionPoly", {});
  auto standingPolyConfig = config.get("standingPoly", {});
  auto crouchingPolyConfig = config.get("crouchingPoly", {});

  if (!standingPolyConfig.isNull())
    standingPoly = jsonToPolyF(standingPolyConfig);
  else if (!collisionPolyConfig.isNull())
    standingPoly = jsonToPolyF(collisionPolyConfig);

  if (!crouchingPolyConfig.isNull())
    crouchingPoly = jsonToPolyF(crouchingPolyConfig);
  else if (!collisionPolyConfig.isNull())
    crouchingPoly = jsonToPolyF(collisionPolyConfig);

  stickyCollision = config.optBool("stickyCollision");
  stickyForce = config.optFloat("stickyForce");

  walkSpeed = config.optFloat("walkSpeed");
  runSpeed = config.optFloat("runSpeed");
  flySpeed = config.optFloat("flySpeed");
  airFriction = config.optFloat("airFriction");
  liquidFriction = config.optFloat("liquidFriction");
  minimumLiquidPercentage = config.optFloat("minimumLiquidPercentage");
  liquidImpedance = config.optFloat("liquidImpedance");
  normalGroundFriction = config.optFloat("normalGroundFriction");
  ambulatingGroundFriction = config.optFloat("ambulatingGroundFriction");
  groundForce = config.optFloat("groundForce");
  airForce = config.optFloat("airForce");
  liquidForce = config.optFloat("liquidForce");

  airJumpProfile = config.opt("airJumpProfile").apply(construct<ActorJumpProfile>()).value();
  liquidJumpProfile = config.opt("liquidJumpProfile").apply(construct<ActorJumpProfile>()).value();

  fallStatusSpeedMin = config.optFloat("fallStatusSpeedMin");
  fallThroughSustainFrames = config.optInt("fallThroughSustainFrames");
  maximumPlatformCorrection = config.optFloat("maximumPlatformCorrection");
  maximumPlatformCorrectionVelocityFactor = config.optFloat("maximumPlatformCorrectionVelocityFactor");

  physicsEffectCategories = config.opt("physicsEffectCategories").apply(jsonToStringSet);

  groundMovementMinimumSustain = config.optFloat("groundMovementMinimumSustain");
  groundMovementMaximumSustain = config.optFloat("groundMovementMaximumSustain");
  groundMovementCheckDistance = config.optFloat("groundMovementCheckDistance");

  collisionEnabled = config.optBool("collisionEnabled");
  frictionEnabled = config.optBool("frictionEnabled");
  gravityEnabled = config.optBool("gravityEnabled");

  pathExploreRate = config.optFloat("pathExploreRate");
}

Json ActorMovementParameters::toJson() const {
  return JsonObject{{"mass", jsonFromMaybe(mass)},
      {"gravityMultiplier", jsonFromMaybe(gravityMultiplier)},
      {"liquidBuoyancy", jsonFromMaybe(liquidBuoyancy)},
      {"airBuoyancy", jsonFromMaybe(airBuoyancy)},
      {"bounceFactor", jsonFromMaybe(bounceFactor)},
      {"stopOnFirstBounce", jsonFromMaybe(stopOnFirstBounce)},
      {"enableSurfaceSlopeCorrection", jsonFromMaybe(enableSurfaceSlopeCorrection)},
      {"slopeSlidingFactor", jsonFromMaybe(slopeSlidingFactor)},
      {"maxMovementPerStep", jsonFromMaybe(maxMovementPerStep)},
      {"maximumCorrection", jsonFromMaybe(maximumCorrection)},
      {"speedLimit", jsonFromMaybe(speedLimit)},
      {"standingPoly", jsonFromMaybe(standingPoly, jsonFromPolyF)},
      {"crouchingPoly", jsonFromMaybe(crouchingPoly, jsonFromPolyF)},
      {"stickyCollision", jsonFromMaybe(stickyCollision)},
      {"stickyForce", jsonFromMaybe(stickyForce)},
      {"walkSpeed", jsonFromMaybe(walkSpeed)},
      {"runSpeed", jsonFromMaybe(runSpeed)},
      {"flySpeed", jsonFromMaybe(flySpeed)},
      {"airFriction", jsonFromMaybe(airFriction)},
      {"liquidFriction", jsonFromMaybe(liquidFriction)},
      {"minimumLiquidPercentage", jsonFromMaybe(minimumLiquidPercentage)},
      {"liquidImpedance", jsonFromMaybe(liquidImpedance)},
      {"normalGroundFriction", jsonFromMaybe(normalGroundFriction)},
      {"ambulatingGroundFriction", jsonFromMaybe(ambulatingGroundFriction)},
      {"groundForce", jsonFromMaybe(groundForce)},
      {"airForce", jsonFromMaybe(airForce)},
      {"liquidForce", jsonFromMaybe(liquidForce)},
      {"airJumpProfile", airJumpProfile.toJson()},
      {"liquidJumpProfile", liquidJumpProfile.toJson()},
      {"fallStatusSpeedMin", jsonFromMaybe(fallStatusSpeedMin)},
      {"fallThroughSustainFrames", jsonFromMaybe(fallThroughSustainFrames)},
      {"maximumPlatformCorrection", jsonFromMaybe(maximumPlatformCorrection)},
      {"maximumPlatformCorrectionVelocityFactor", jsonFromMaybe(maximumPlatformCorrectionVelocityFactor)},
      {"physicsEffectCategories", jsonFromMaybe(physicsEffectCategories, jsonFromStringSet)},
      {"groundMovementMinimumSustain", jsonFromMaybe(groundMovementMinimumSustain)},
      {"groundMovementMaximumSustain", jsonFromMaybe(groundMovementMaximumSustain)},
      {"groundMovementCheckDistance", jsonFromMaybe(groundMovementCheckDistance)},
      {"collisionEnabled", jsonFromMaybe(collisionEnabled)},
      {"frictionEnabled", jsonFromMaybe(frictionEnabled)},
      {"gravityEnabled", jsonFromMaybe(gravityEnabled)},
      {"pathExploreRate", jsonFromMaybe(pathExploreRate)}};
}

ActorMovementParameters ActorMovementParameters::merge(ActorMovementParameters const& rhs) const {
  ActorMovementParameters merged;

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
  merged.standingPoly = rhs.standingPoly.orMaybe(standingPoly);
  merged.crouchingPoly = rhs.crouchingPoly.orMaybe(crouchingPoly);
  merged.stickyCollision = rhs.stickyCollision.orMaybe(stickyCollision);
  merged.stickyForce = rhs.stickyForce.orMaybe(stickyForce);
  merged.walkSpeed = rhs.walkSpeed.orMaybe(walkSpeed);
  merged.runSpeed = rhs.runSpeed.orMaybe(runSpeed);
  merged.flySpeed = rhs.flySpeed.orMaybe(flySpeed);
  merged.airFriction = rhs.airFriction.orMaybe(airFriction);
  merged.liquidFriction = rhs.liquidFriction.orMaybe(liquidFriction);
  merged.minimumLiquidPercentage = rhs.minimumLiquidPercentage.orMaybe(minimumLiquidPercentage);
  merged.liquidImpedance = rhs.liquidImpedance.orMaybe(liquidImpedance);
  merged.normalGroundFriction = rhs.normalGroundFriction.orMaybe(normalGroundFriction);
  merged.ambulatingGroundFriction = rhs.ambulatingGroundFriction.orMaybe(ambulatingGroundFriction);
  merged.groundForce = rhs.groundForce.orMaybe(groundForce);
  merged.airForce = rhs.airForce.orMaybe(airForce);
  merged.liquidForce = rhs.liquidForce.orMaybe(liquidForce);

  merged.airJumpProfile = airJumpProfile.merge(rhs.airJumpProfile);
  merged.liquidJumpProfile = liquidJumpProfile.merge(rhs.liquidJumpProfile);

  merged.fallStatusSpeedMin = rhs.fallStatusSpeedMin.orMaybe(fallStatusSpeedMin);
  merged.fallThroughSustainFrames = rhs.fallThroughSustainFrames.orMaybe(fallThroughSustainFrames);
  merged.maximumPlatformCorrection = rhs.maximumPlatformCorrection.orMaybe(maximumPlatformCorrection);
  merged.maximumPlatformCorrectionVelocityFactor = rhs.maximumPlatformCorrectionVelocityFactor.orMaybe(maximumPlatformCorrectionVelocityFactor);

  merged.physicsEffectCategories = rhs.physicsEffectCategories.orMaybe(physicsEffectCategories);

  merged.groundMovementMinimumSustain = rhs.groundMovementMinimumSustain.orMaybe(groundMovementMinimumSustain);
  merged.groundMovementMaximumSustain = rhs.groundMovementMaximumSustain.orMaybe(groundMovementMaximumSustain);
  merged.groundMovementCheckDistance = rhs.groundMovementCheckDistance.orMaybe(groundMovementCheckDistance);

  merged.collisionEnabled = rhs.collisionEnabled.orMaybe(collisionEnabled);
  merged.frictionEnabled = rhs.frictionEnabled.orMaybe(frictionEnabled);
  merged.gravityEnabled = rhs.gravityEnabled.orMaybe(gravityEnabled);

  merged.pathExploreRate = rhs.pathExploreRate.orMaybe(pathExploreRate);

  return merged;
}

DataStream& operator>>(DataStream& ds, ActorMovementParameters& movementParameters) {
  ds.read(movementParameters.mass);
  ds.read(movementParameters.gravityMultiplier);
  ds.read(movementParameters.liquidBuoyancy);
  ds.read(movementParameters.airBuoyancy);
  ds.read(movementParameters.bounceFactor);
  ds.read(movementParameters.stopOnFirstBounce);
  ds.read(movementParameters.enableSurfaceSlopeCorrection);
  ds.read(movementParameters.slopeSlidingFactor);
  ds.read(movementParameters.maxMovementPerStep);
  ds.read(movementParameters.maximumCorrection);
  ds.read(movementParameters.speedLimit);
  ds.read(movementParameters.standingPoly);
  ds.read(movementParameters.crouchingPoly);
  ds.read(movementParameters.stickyCollision);
  ds.read(movementParameters.stickyForce);
  ds.read(movementParameters.walkSpeed);
  ds.read(movementParameters.runSpeed);
  ds.read(movementParameters.flySpeed);
  ds.read(movementParameters.airFriction);
  ds.read(movementParameters.liquidFriction);
  ds.read(movementParameters.minimumLiquidPercentage);
  ds.read(movementParameters.liquidImpedance);
  ds.read(movementParameters.normalGroundFriction);
  ds.read(movementParameters.ambulatingGroundFriction);
  ds.read(movementParameters.groundForce);
  ds.read(movementParameters.airForce);
  ds.read(movementParameters.liquidForce);
  ds.read(movementParameters.airJumpProfile);
  ds.read(movementParameters.liquidJumpProfile);
  ds.read(movementParameters.fallStatusSpeedMin);
  ds.read(movementParameters.fallThroughSustainFrames);
  ds.read(movementParameters.maximumPlatformCorrection);
  ds.read(movementParameters.maximumPlatformCorrectionVelocityFactor);
  ds.read(movementParameters.physicsEffectCategories);
  ds.read(movementParameters.collisionEnabled);
  ds.read(movementParameters.frictionEnabled);
  ds.read(movementParameters.gravityEnabled);
  ds.read(movementParameters.pathExploreRate);

  return ds;
}

DataStream& operator<<(DataStream& ds, ActorMovementParameters const& movementParameters) {
  ds.write(movementParameters.mass);
  ds.write(movementParameters.gravityMultiplier);
  ds.write(movementParameters.liquidBuoyancy);
  ds.write(movementParameters.airBuoyancy);
  ds.write(movementParameters.bounceFactor);
  ds.write(movementParameters.stopOnFirstBounce);
  ds.write(movementParameters.enableSurfaceSlopeCorrection);
  ds.write(movementParameters.slopeSlidingFactor);
  ds.write(movementParameters.maxMovementPerStep);
  ds.write(movementParameters.maximumCorrection);
  ds.write(movementParameters.speedLimit);
  ds.write(movementParameters.standingPoly);
  ds.write(movementParameters.crouchingPoly);
  ds.write(movementParameters.stickyCollision);
  ds.write(movementParameters.stickyForce);
  ds.write(movementParameters.walkSpeed);
  ds.write(movementParameters.runSpeed);
  ds.write(movementParameters.flySpeed);
  ds.write(movementParameters.airFriction);
  ds.write(movementParameters.liquidFriction);
  ds.write(movementParameters.minimumLiquidPercentage);
  ds.write(movementParameters.liquidImpedance);
  ds.write(movementParameters.normalGroundFriction);
  ds.write(movementParameters.ambulatingGroundFriction);
  ds.write(movementParameters.groundForce);
  ds.write(movementParameters.airForce);
  ds.write(movementParameters.liquidForce);
  ds.write(movementParameters.airJumpProfile);
  ds.write(movementParameters.liquidJumpProfile);
  ds.write(movementParameters.fallStatusSpeedMin);
  ds.write(movementParameters.fallThroughSustainFrames);
  ds.write(movementParameters.maximumPlatformCorrection);
  ds.write(movementParameters.maximumPlatformCorrectionVelocityFactor);
  ds.write(movementParameters.physicsEffectCategories);
  ds.write(movementParameters.collisionEnabled);
  ds.write(movementParameters.frictionEnabled);
  ds.write(movementParameters.gravityEnabled);
  ds.write(movementParameters.pathExploreRate);

  return ds;
}

ActorMovementModifiers::ActorMovementModifiers(Json const& config) {
  groundMovementModifier = 1.0f;
  liquidMovementModifier = 1.0f;
  speedModifier = 1.0f;
  airJumpModifier = 1.0f;
  liquidJumpModifier = 1.0f;
  runningSuppressed = false;
  jumpingSuppressed = false;
  facingSuppressed = false;
  movementSuppressed = false;

  if (!config.isNull()) {
    groundMovementModifier = config.getFloat("groundMovementModifier", 1.0f);
    liquidMovementModifier = config.getFloat("liquidMovementModifier", 1.0f);
    speedModifier = config.getFloat("speedModifier", 1.0f);
    airJumpModifier = config.getFloat("airJumpModifier", 1.0f);
    liquidJumpModifier = config.getFloat("liquidJumpModifier", 1.0f);
    runningSuppressed = config.getBool("runningSuppressed", false);
    jumpingSuppressed = config.getBool("jumpingSuppressed", false);
    facingSuppressed = config.getBool("facingSuppressed", false);
    movementSuppressed = config.getBool("movementSuppressed", false);
  }
}

Json ActorMovementModifiers::toJson() const {
  return JsonObject{
    {"groundMovementModifier", groundMovementModifier},
    {"liquidMovementModifier", liquidMovementModifier},
    {"speedModifier", speedModifier},
    {"airJumpModifier", airJumpModifier},
    {"liquidJumpModifier", liquidJumpModifier},
    {"runningSuppressed", runningSuppressed},
    {"jumpingSuppressed", jumpingSuppressed},
    {"facingSuppressed", facingSuppressed},
    {"movementSuppressed", movementSuppressed}
  };
}

ActorMovementModifiers ActorMovementModifiers::combine(ActorMovementModifiers const& rhs) const {
  ActorMovementModifiers res = *this;

  res.groundMovementModifier *= rhs.groundMovementModifier;
  res.liquidMovementModifier *= rhs.liquidMovementModifier;
  res.speedModifier *= rhs.speedModifier;
  res.airJumpModifier *= rhs.airJumpModifier;
  res.liquidJumpModifier *= rhs.liquidJumpModifier;
  res.runningSuppressed = res.runningSuppressed || rhs.runningSuppressed;
  res.jumpingSuppressed = res.jumpingSuppressed || rhs.jumpingSuppressed;
  res.facingSuppressed = res.facingSuppressed || rhs.facingSuppressed;
  res.movementSuppressed = res.movementSuppressed || rhs.movementSuppressed;

  return res;
}

DataStream& operator>>(DataStream& ds, ActorMovementModifiers& movementModifiers) {
  ds.read(movementModifiers.groundMovementModifier);
  ds.read(movementModifiers.liquidMovementModifier);
  ds.read(movementModifiers.speedModifier);
  ds.read(movementModifiers.airJumpModifier);
  ds.read(movementModifiers.liquidJumpModifier);
  ds.read(movementModifiers.runningSuppressed);
  ds.read(movementModifiers.jumpingSuppressed);
  ds.read(movementModifiers.facingSuppressed);
  ds.read(movementModifiers.movementSuppressed);

  return ds;
}

DataStream& operator<<(DataStream& ds, ActorMovementModifiers const& movementModifiers) {
  ds.write(movementModifiers.groundMovementModifier);
  ds.write(movementModifiers.liquidMovementModifier);
  ds.write(movementModifiers.speedModifier);
  ds.write(movementModifiers.airJumpModifier);
  ds.write(movementModifiers.liquidJumpModifier);
  ds.write(movementModifiers.runningSuppressed);
  ds.write(movementModifiers.jumpingSuppressed);
  ds.write(movementModifiers.facingSuppressed);
  ds.write(movementModifiers.movementSuppressed);

  return ds;
}

ActorMovementController::ActorMovementController(ActorMovementParameters const& parameters) {
  m_controlRotationRate = 0.0f;
  m_controlRun = false;
  m_controlCrouch = false;
  m_controlDown = false;
  m_controlJump = false;
  m_controlJumpAnyway = false;
  m_controlPathMove = {};

  addNetElement(&m_walking);
  addNetElement(&m_running);
  addNetElement(&m_movingDirection);
  addNetElement(&m_facingDirection);
  addNetElement(&m_crouching);
  addNetElement(&m_flying);
  addNetElement(&m_falling);
  addNetElement(&m_canJump);
  addNetElement(&m_jumping);
  addNetElement(&m_groundMovement);
  addNetElement(&m_liquidMovement);
  addNetElement(&m_anchorState);

  m_fallThroughSustain = 0;
  m_lastControlJump = false;
  m_lastControlDown = false;
  m_targetHorizontalAmbulatingVelocity = 0.0f;
  m_moveSpeedMultiplier = 1.0f;

  resetBaseParameters(parameters);
}

ActorMovementParameters const& ActorMovementController::baseParameters() const {
  return m_baseParameters;
}

void ActorMovementController::updateBaseParameters(ActorMovementParameters const& parameters) {
  m_baseParameters = m_baseParameters.merge(parameters);
  applyMCParameters(m_baseParameters);
}

void ActorMovementController::resetBaseParameters(ActorMovementParameters const& parameters) {
  m_baseParameters = ActorMovementParameters::sensibleDefaults().merge(parameters);
  applyMCParameters(m_baseParameters);
}

ActorMovementModifiers const& ActorMovementController::baseModifiers() const {
  return m_baseModifiers;
}

void ActorMovementController::updateBaseModifiers(ActorMovementModifiers const& modifiers) {
  m_baseModifiers = m_baseModifiers.combine(modifiers);
}

void ActorMovementController::resetBaseModifiers(ActorMovementModifiers const& modifiers) {
  m_baseModifiers = modifiers;
}

Json ActorMovementController::storeState() const {
  return JsonObject{{"position", jsonFromVec2F(MovementController::position())},
      {"velocity", jsonFromVec2F(velocity())},
      {"rotation", MovementController::rotation()},
      {"movingDirection", DirectionNames.getRight(m_movingDirection.get())},
      {"facingDirection", DirectionNames.getRight(m_facingDirection.get())},
      {"crouching", m_crouching.get()},
      {"scale", getScale()}};
}

void ActorMovementController::loadState(Json const& state) {
  setPosition(jsonToVec2F(state.get("position")));
  setVelocity(jsonToVec2F(state.get("velocity")));
  setRotation(state.getFloat("rotation"));
  m_movingDirection.set(DirectionNames.getLeft(state.getString("movingDirection")));
  m_facingDirection.set(DirectionNames.getLeft(state.getString("facingDirection")));
  m_crouching.set(state.getBool("crouching"));
  scale(state.getFloat("scale", 1.0f));
}

void ActorMovementController::setAnchorState(EntityAnchorState anchorState) {
  doSetAnchorState(anchorState);
}

void ActorMovementController::resetAnchorState() {
  doSetAnchorState({});
}

Maybe<EntityAnchorState> ActorMovementController::anchorState() const {
  return m_anchorState.get();
}

EntityAnchorConstPtr ActorMovementController::entityAnchor() const {
  return m_entityAnchor;
}

Vec2F ActorMovementController::position() const {
  if (m_entityAnchor)
    return m_entityAnchor->position;
  return MovementController::position();
}

float ActorMovementController::rotation() const {
  if (m_entityAnchor)
    return m_entityAnchor->angle;
  return MovementController::rotation();
}

bool ActorMovementController::walking() const {
  return m_walking.get();
}

bool ActorMovementController::running() const {
  return m_running.get();
}

Direction ActorMovementController::movingDirection() const {
  return m_movingDirection.get();
}

Direction ActorMovementController::facingDirection() const {
  if (m_entityAnchor)
    return m_entityAnchor->direction;
  return m_facingDirection.get();
}

bool ActorMovementController::crouching() const {
  return m_crouching.get();
}

bool ActorMovementController::flying() const {
  return m_flying.get();
}

bool ActorMovementController::falling() const {
  return m_falling.get();
}

bool ActorMovementController::canJump() const {
  return m_canJump.get();
}

bool ActorMovementController::jumping() const {
  return m_jumping.get();
}

bool ActorMovementController::groundMovement() const {
  return m_groundMovement.get();
}

bool ActorMovementController::liquidMovement() const {
  return m_liquidMovement.get();
}

bool ActorMovementController::pathfinding() const {
  if (m_pathController)
    return m_pathController->pathfinding();
  else
    return false;
}

void ActorMovementController::controlRotation(float rotationRate) {
  m_controlRotationRate += rotationRate;
}

void ActorMovementController::controlAcceleration(Vec2F const& acceleration) {
  m_controlAcceleration += acceleration;
}

void ActorMovementController::controlForce(Vec2F const& force) {
  m_controlForce += force;
}

void ActorMovementController::controlApproachVelocity(Vec2F const& targetVelocity, float maxControlForce) {
  m_controlApproachVelocities.append({targetVelocity, maxControlForce});
}

void ActorMovementController::controlApproachVelocityAlongAngle(
    float angle, float targetVelocity, float maxControlForce, bool positiveOnly) {
  m_controlApproachVelocityAlongAngles.append({angle, targetVelocity, maxControlForce, positiveOnly});
}

void ActorMovementController::controlApproachXVelocity(float targetXVelocity, float maxControlForce) {
  controlApproachVelocityAlongAngle(0, targetXVelocity, maxControlForce);
}

void ActorMovementController::controlApproachYVelocity(float targetYVelocity, float maxControlForce) {
  controlApproachVelocityAlongAngle(Constants::pi / 2, targetYVelocity, maxControlForce);
}

void ActorMovementController::controlParameters(ActorMovementParameters const& parameters) {
  m_controlParameters = m_controlParameters.merge(parameters);
}

void ActorMovementController::controlModifiers(ActorMovementModifiers const& modifiers) {
  m_controlModifiers = m_controlModifiers.combine(modifiers);
}

void ActorMovementController::controlMove(Direction direction, bool run) {
  m_controlMove = direction;
  m_controlRun = run;
}

void ActorMovementController::controlFace(Direction direction) {
  m_controlFace = direction;
}

void ActorMovementController::controlDown() {
  m_controlDown = true;
}

void ActorMovementController::controlCrouch() {
  m_controlCrouch = true;
}

void ActorMovementController::controlJump(bool jumpEvenIfUnable) {
  m_controlJump = true;
  m_controlJumpAnyway |= jumpEvenIfUnable;
}

void ActorMovementController::controlFly(Vec2F const& velocity) {
  m_controlFly = velocity;
}

Maybe<pair<Vec2F, bool>> ActorMovementController::pathMove(Vec2F const& position, bool, Maybe<PlatformerAStar::Parameters> const& parameters) {
  if (!m_pathController)
    m_pathController = make_shared<PathController>(world());

  // set new parameters if they have changed
  if (m_pathController->targetPosition().isNothing() || (parameters && m_pathController->parameters() != *parameters)) {
    if (parameters)
      m_pathController->setParameters(*parameters);
    m_pathMoveResult = m_pathController->findPath(*this, position).apply([position](bool result) { return pair<Vec2F, bool>(position, result); });
  }

  // update target position if it has changed
  if (m_pathController) {
    m_pathController->findPath(*this, position);
  }

  if (m_pathMoveResult.isValid()) {
    // path controller failed or succeeded, return the result and reset the controller
    m_pathController->reset();
  }

  return take(m_pathMoveResult);
}

Maybe<pair<Vec2F, bool>> ActorMovementController::controlPathMove(Vec2F const& position, bool run, Maybe<PlatformerAStar::Parameters> const& parameters) {
  auto result = pathMove(position, run, parameters);

  if (result.isNothing())
    m_controlPathMove = pair<Vec2F, bool>(position, run);

  return result;
}

void ActorMovementController::setMoveSpeedMultiplier(float scale) {
  m_moveSpeedMultiplier = scale;
}

void ActorMovementController::clearControls() {
  m_controlRotationRate = 0.0f;
  m_controlAcceleration = Vec2F();
  m_controlForce = Vec2F();
  m_controlApproachVelocities.clear();
  m_controlApproachVelocityAlongAngles.clear();
  m_controlMove = {};
  m_controlFace = {};
  m_controlRun = false;
  m_controlCrouch = false;
  m_controlDown = false;
  m_controlJump = false;
  m_controlJumpAnyway = false;
  m_controlFly = {};
  m_controlPathMove = {};
  m_controlParameters = ActorMovementParameters();
  m_controlModifiers = ActorMovementModifiers();
}

void ActorMovementController::tickMaster(float dt) {
  EntityAnchorConstPtr newAnchor;
  if (auto anchorState = m_anchorState.get()) {
    if (auto anchorableEntity = as<AnchorableEntity>(world()->entity(anchorState->entityId)))
      newAnchor = anchorableEntity->anchor(anchorState->positionIndex);
  }

  if (newAnchor)
    m_entityAnchor = newAnchor;
  else
    resetAnchorState();

  if (m_entityAnchor) {
    m_walking.set(false);
    m_running.set(false);
    m_crouching.set(false);
    m_flying.set(false);
    m_falling.set(false);
    m_canJump.set(false);
    m_jumping.set(false);
    m_groundMovement.set(false);
    m_liquidMovement.set(false);

    setVelocity((m_entityAnchor->position - MovementController::position()) / dt);
    MovementController::tickMaster(dt);
    setPosition(m_entityAnchor->position);
  } else {
    auto activeParameters = m_baseParameters.merge(m_controlParameters);
    auto activeModifiers = m_baseModifiers.combine(m_controlModifiers);

    if (activeModifiers.movementSuppressed) {
      m_controlMove = {};
      m_controlRun = false;
      m_controlCrouch = false;
      m_controlDown = false;
      m_controlJump = false;
      m_controlFly = {};
      m_controlPathMove = {};
    }

    if (m_controlMove.isValid()
        || m_controlCrouch
        || m_controlDown
        || m_controlJump
        || m_controlFly.isValid()
        || !m_controlApproachVelocities.empty()
        || !m_controlApproachVelocityAlongAngles.empty()) {
      // controlling any other movement overrides the pathing
      m_controlPathMove.reset();
    }

    if (m_controlPathMove && m_pathMoveResult.isNothing()) {
      if (appliedForceRegion()) {
        m_pathController->reset();
      } else if (!m_pathController->pathfinding()) {
        m_pathMoveResult = m_pathController->move(*this, activeParameters, activeModifiers, m_controlPathMove->second, dt)
          .apply([this](bool result) { return pair<Vec2F, bool>(m_controlPathMove->first, result); });

        auto action = m_pathController->curAction();
        bool onGround = false;
        if (auto a = action) {
          using namespace PlatformerAStar;
          m_walking.set(a == Action::Walk && !m_controlPathMove->second);
          m_running.set(a == Action::Walk && m_controlPathMove->second);
          m_flying.set(a == Action::Fly || a == Action::Swim);
          m_falling.set((a == Action::Arc && yVelocity() < 0.0f) || a == Action::Drop);
          m_jumping.set(a == Action::Arc && yVelocity() >= 0.0f);

          onGround = a == Action::Walk || a == Action::Drop || a == Action::Jump;

          if (a == Action::Land || a== Action::Jump) {
            auto inLiquid = liquidPercentage() >= activeParameters.minimumLiquidPercentage.value(1.0);
            m_liquidMovement.set(inLiquid);
            m_groundMovement.set(!inLiquid);
            onGround = !inLiquid && onGround;
          } else {
            m_liquidMovement.set(a == Action::Swim);
            m_groundMovement.set(a != Action::Arc && a != Action::Swim);
          }
        } else {
          m_walking.set(false);
          m_running.set(false);
        }
        auto facing = m_controlFace.orMaybe(m_pathController->facing()).value(m_facingDirection.get());
        m_facingDirection.set(facing);
        m_movingDirection.set(m_pathController->facing().value(m_facingDirection.get()));

        applyMCParameters(activeParameters);

        // MovementController still handles updating liquid percentage and updating force regions
        updateLiquidPercentage();
        updateForceRegions(dt);
        // onGround flag needs to be manually set, won't be set by MovementController::tickMaster
        setOnGround(onGround);
        clearControls();
        return;
      } else {
        m_pathMoveResult = m_pathController->findPath(*this, m_controlPathMove->first).apply([this](bool result) {
            return pair<Vec2F, bool>(m_controlPathMove->first, result);
          });
      }
    } else {
      m_pathController = {};
    }

    // Do some basic movement consistency checks.
    if (m_controlFly)
      m_controlMove = {};

    if ((m_controlDown && !m_lastControlDown) || m_controlFly)
      m_fallThroughSustain = *activeParameters.fallThroughSustainFrames;
    else if (m_fallThroughSustain > 0)
      --m_fallThroughSustain;

    applyMCParameters(activeParameters);

    m_targetHorizontalAmbulatingVelocity = 0.0f;

    rotate(m_controlRotationRate);
    accelerate(m_controlAcceleration);
    force(m_controlForce);

    for (auto const& approach : m_controlApproachVelocities)
      approachVelocity(approach.targetVelocity * activeModifiers.speedModifier, approach.maxControlForce);

    for (auto const& approach : m_controlApproachVelocityAlongAngles)
      approachVelocityAlongAngle(approach.alongAngle, approach.targetVelocity * activeModifiers.speedModifier, approach.maxControlForce, approach.positiveOnly);

    m_liquidMovement.set(liquidPercentage() >= *activeParameters.minimumLiquidPercentage);
    float liquidImpedance = *activeParameters.liquidImpedance * liquidPercentage();

    Maybe<Direction> updatedMovingDirection;
    bool running = m_controlRun && !activeModifiers.runningSuppressed;

    if (m_controlFly) {
      Vec2F flyVelocity = *m_controlFly;
      if (flyVelocity.magnitudeSquared() != 0)
        flyVelocity = flyVelocity.normalized() * *activeParameters.flySpeed * getScale();

      if (m_liquidMovement.get())
        approachVelocity(flyVelocity * (1.0f - liquidImpedance) * activeModifiers.speedModifier, *activeParameters.liquidForce * activeModifiers.liquidMovementModifier * getScale());
      else
        approachVelocity(flyVelocity * activeModifiers.speedModifier, *activeParameters.airForce * getScale());

      if (flyVelocity[0] > 0)
        updatedMovingDirection = Direction::Right;
      else if (flyVelocity[0] < 0)
        updatedMovingDirection = Direction::Left;

      m_groundMovementSustainTimer = GameTimer(0);

    } else {
      float jumpModifier;
      ActorJumpProfile jumpProfile;
      if (m_liquidMovement.get()) {
        jumpModifier = activeModifiers.liquidJumpModifier;
        jumpProfile = activeParameters.liquidJumpProfile;
        *jumpProfile.jumpSpeed *= (1.0f - liquidImpedance) * getScale();
      } else {
        jumpModifier = activeModifiers.airJumpModifier;
        jumpProfile = activeParameters.airJumpProfile;
      }

      bool startJump = false;
      bool holdJump = false;

      // If we are on the ground, then reset the ground movement sustain timer
      // to the maximum.  If we are not on the ground or near the ground
      // according to the nearGroundCheckDistance, and we are past the minimum
      // sustain time, then go ahead and immediately clear the ground movement
      // sustain timer.
      float minGroundSustain = *activeParameters.groundMovementMinimumSustain;
      float maxGroundSustain = *activeParameters.groundMovementMaximumSustain;
      float groundCheckDistance = *activeParameters.groundMovementCheckDistance;
      m_groundMovementSustainTimer.tick(dt);
      if (onGround()) {
        m_groundMovementSustainTimer = GameTimer(maxGroundSustain);
      } else if (!m_groundMovementSustainTimer.ready() && groundCheckDistance > 0.0f && maxGroundSustain - m_groundMovementSustainTimer.timer > minGroundSustain) {
        PolyF collisionBody = MovementController::collisionBody();
        collisionBody.translate(Vec2F(0, -groundCheckDistance));
        if (!world()->polyCollision(collisionBody, {CollisionKind::Block, CollisionKind::Dynamic, CollisionKind::Platform, CollisionKind::Slippery}))
          m_groundMovementSustainTimer = GameTimer(0);
      }

      bool standingJumpable = !m_groundMovementSustainTimer.ready();
      bool controlJump = m_controlJump && (!activeModifiers.jumpingSuppressed || m_controlJumpAnyway);

      // We are doing a jump if m_reJumpTimer has run out and there has been a
      // new m_controlJump command which was just recently triggered.  If
      // jumpProfile.autoJump is set, then we don't care whether it is a new
      // m_controlJump command, m_controlJump can be held.
      if (m_reJumpTimer.ready() && controlJump && (*jumpProfile.autoJump || !m_lastControlJump)) {
        if (standingJumpable || *jumpProfile.multiJump || m_controlJumpAnyway)
          startJump = true;
      } else if (m_jumping.get() && controlJump && (!m_jumpHoldTimer || !m_jumpHoldTimer->ready())) {
        if (!*jumpProfile.collisionCancelled || collisionCorrection()[1] >= 0.0f)
          holdJump = true;
      }

      if (startJump) {
        m_jumping.set(true);

        m_reJumpTimer = GameTimer(*jumpProfile.reJumpDelay);
        if (*jumpProfile.jumpHoldTime >= 0.0f)
          m_jumpHoldTimer = GameTimer(*jumpProfile.jumpHoldTime);
        else
          m_jumpHoldTimer = {};

        setYVelocity(yVelocity() + *jumpProfile.jumpSpeed * *jumpProfile.jumpInitialPercentage * jumpModifier * getScale());

        m_groundMovementSustainTimer = GameTimer(0);

      } else if (holdJump) {
        m_reJumpTimer.tick(dt);
        if (m_jumpHoldTimer)
          m_jumpHoldTimer->tick(dt);

        approachYVelocity(*jumpProfile.jumpSpeed * jumpModifier * getScale(), *jumpProfile.jumpControlForce * jumpModifier * getScale());

      } else {
        m_jumping.set(false);
        m_reJumpTimer.tick(dt);
      }

      if (m_controlMove == Direction::Left) {
        updatedMovingDirection = Direction::Left;
        m_targetHorizontalAmbulatingVelocity =
            -1.0f * (running ? *activeParameters.runSpeed * activeModifiers.speedModifier * getScale()
                             : *activeParameters.walkSpeed * activeModifiers.speedModifier * getScale());
      } else if (m_controlMove == Direction::Right) {
        updatedMovingDirection = Direction::Right;
        m_targetHorizontalAmbulatingVelocity =
            1.0f * (running ? *activeParameters.runSpeed * activeModifiers.speedModifier * getScale()
                            : *activeParameters.walkSpeed * activeModifiers.speedModifier * getScale());
      }

      m_targetHorizontalAmbulatingVelocity *= m_moveSpeedMultiplier;

      if (m_liquidMovement.get())
        m_targetHorizontalAmbulatingVelocity *= (1.0f - liquidImpedance);

      Vec2F surfaceVelocity = MovementController::surfaceVelocity();

      // don't ambulate if we're already moving faster than the target velocity in the direction of ambulation
      bool ambulationWouldAccelerate = abs(m_targetHorizontalAmbulatingVelocity + surfaceVelocity[0]) > abs(xVelocity())
          || (m_targetHorizontalAmbulatingVelocity < 0) != (xVelocity() < 0);

      if (m_targetHorizontalAmbulatingVelocity != 0.0f && ambulationWouldAccelerate) {
        float ambulatingAccel;
        if (onGround())
          ambulatingAccel = *activeParameters.groundForce * activeModifiers.groundMovementModifier;
        else if (m_liquidMovement.get())
          ambulatingAccel = *activeParameters.liquidForce * activeModifiers.liquidMovementModifier;
        else
          ambulatingAccel = *activeParameters.airForce;

        approachXVelocity(m_targetHorizontalAmbulatingVelocity + surfaceVelocity[0], ambulatingAccel * getScale());
      }
    }

    if (updatedMovingDirection)
      m_movingDirection.set(*updatedMovingDirection);

    if (!activeModifiers.facingSuppressed) {
      if (m_controlFace)
        m_facingDirection.set(*m_controlFace);
      else if (updatedMovingDirection)
        m_facingDirection.set(*updatedMovingDirection);
      else if (m_controlPathMove && m_pathController && m_pathController->facing())
        m_facingDirection.set(*m_pathController->facing());
    }

    m_groundMovement.set(!m_groundMovementSustainTimer.ready());
    if (m_groundMovement.get()) {
      m_running.set(running && m_controlMove);
      m_walking.set(!running && m_controlMove);
      m_crouching.set(m_controlCrouch && !m_controlMove);
    }
    m_flying.set((bool)m_controlFly);

    bool falling = (yVelocity() < *activeParameters.fallStatusSpeedMin) && !m_groundMovement.get();
    m_falling.set(falling);

    MovementController::tickMaster(dt);

    m_lastControlJump = m_controlJump;
    m_lastControlDown = m_controlDown;

    if (m_liquidMovement.get())
      m_canJump.set(m_reJumpTimer.ready() && (!m_groundMovementSustainTimer.ready() || *activeParameters.liquidJumpProfile.multiJump));
    else
      m_canJump.set(m_reJumpTimer.ready() && (!m_groundMovementSustainTimer.ready() || *activeParameters.airJumpProfile.multiJump));
  }

  clearControls();
}

void ActorMovementController::tickSlave(float dt) {
  MovementController::tickSlave(dt);

  m_entityAnchor.reset();
  if (auto anchorState = m_anchorState.get()) {
    if (auto anchorableEntity = as<AnchorableEntity>(world()->entity(anchorState->entityId)))
      m_entityAnchor = anchorableEntity->anchor(anchorState->positionIndex);
  }
}

void ActorMovementController::applyMCParameters(ActorMovementParameters const& parameters) {
  MovementParameters mcParameters;

  mcParameters.mass = parameters.mass;

  if (!onGround() && yVelocity() < 0)
    mcParameters.gravityMultiplier = *parameters.gravityMultiplier;
  else
    mcParameters.gravityMultiplier = parameters.gravityMultiplier;

  mcParameters.liquidBuoyancy = parameters.liquidBuoyancy;
  mcParameters.airBuoyancy = parameters.airBuoyancy;
  mcParameters.bounceFactor = parameters.bounceFactor;
  mcParameters.stopOnFirstBounce = parameters.stopOnFirstBounce;
  mcParameters.enableSurfaceSlopeCorrection = parameters.enableSurfaceSlopeCorrection;
  mcParameters.slopeSlidingFactor = parameters.slopeSlidingFactor;
  mcParameters.maxMovementPerStep = parameters.maxMovementPerStep;

  if (m_crouching.get())
    mcParameters.collisionPoly = *parameters.crouchingPoly;
  else
    mcParameters.collisionPoly = *parameters.standingPoly;

  mcParameters.stickyCollision = parameters.stickyCollision;
  mcParameters.stickyForce = parameters.stickyForce;

  mcParameters.airFriction = parameters.airFriction;
  mcParameters.liquidFriction = parameters.liquidFriction;

  // If we are traveling in the correct direction while in a movement mode that
  // requires contact with the ground (ambulating i.e. walking or running), and
  // not traveling faster than our target horizontal movement, then apply the
  // special 'ambulatingGroundFriction'.
  float relativeXVelocity = xVelocity() - surfaceVelocity()[0];
  bool useAmbulatingGroundFriction = (m_walking.get() || m_running.get())
      && copysign(1, m_targetHorizontalAmbulatingVelocity) == copysign(1, relativeXVelocity)
      && fabs(relativeXVelocity) <= fabs(m_targetHorizontalAmbulatingVelocity);

  if (useAmbulatingGroundFriction)
    mcParameters.groundFriction = parameters.ambulatingGroundFriction;
  else
    mcParameters.groundFriction = parameters.normalGroundFriction;

  mcParameters.collisionEnabled = parameters.collisionEnabled;
  mcParameters.frictionEnabled = parameters.frictionEnabled;
  mcParameters.gravityEnabled = parameters.gravityEnabled;

  mcParameters.ignorePlatformCollision = m_fallThroughSustain > 0 || m_controlFly || m_controlDown;
  mcParameters.maximumPlatformCorrection = parameters.maximumPlatformCorrection;
  mcParameters.maximumPlatformCorrectionVelocityFactor = parameters.maximumPlatformCorrectionVelocityFactor;

  mcParameters.physicsEffectCategories = parameters.physicsEffectCategories;

  mcParameters.maximumCorrection = parameters.maximumCorrection;
  mcParameters.speedLimit = parameters.speedLimit;

  MovementController::applyParameters(mcParameters);
}

void ActorMovementController::doSetAnchorState(Maybe<EntityAnchorState> anchorState) {
  EntityAnchorConstPtr entityAnchor;
  if (anchorState) {
    auto anchorableEntity = as<AnchorableEntity>(world()->entity(anchorState->entityId));
    if (!anchorableEntity)
      throw ActorMovementControllerException::format("No such anchorable entity id {} in ActorMovementController::setAnchorState", anchorState->entityId);
    entityAnchor = anchorableEntity->anchor(anchorState->positionIndex);
    if (!entityAnchor)
      throw ActorMovementControllerException::format("Anchor position {} is disabled ActorMovementController::setAnchorState", anchorState->positionIndex);
  }
  auto prevAnchor = m_entityAnchor;
  m_anchorState.set(anchorState);
  m_entityAnchor = std::move(entityAnchor);

  if (!entityAnchor && prevAnchor && prevAnchor->exitBottomPosition) {
    auto boundBox = MovementController::localBoundBox();
    Vec2F bottomMid = {boundBox.center()[0], boundBox.yMin()};
    setPosition(*prevAnchor->exitBottomPosition - bottomMid);
  }

  if (m_entityAnchor)
    setPosition(m_entityAnchor->position);
}


PathController::PathController(World* world)
  : m_world(world), m_edgeTimer(0.0) { }

PlatformerAStar::Parameters const& PathController::parameters() {
  return m_parameters;
}

void PathController::setParameters(PlatformerAStar::Parameters const& parameters) {
  m_parameters = parameters;
}

void PathController::reset() {
  m_startPosition = {};
  m_targetPosition = {};
  m_controlFace = {};
  m_pathFinder = {};
  m_path = {};
  m_edgeIndex = 0;
  m_edgeTimer = 0.0;
}

bool PathController::pathfinding() const {
  return m_path.isNothing();
}

Maybe<Vec2F> PathController::targetPosition() const {
  return m_targetPosition;
}

Maybe<Direction> PathController::facing() const {
  return m_controlFace;
}

Maybe<PlatformerAStar::Action> PathController::curAction() const {
  if (m_path && m_edgeIndex < m_path->size()) {
    return m_path->at(m_edgeIndex).action;
  }
  return {};
}

Maybe<bool> PathController::findPath(ActorMovementController& movementController, Vec2F const& targetPosition) {
  using namespace PlatformerAStar;

  // reached the end of the last path and we have a new target position to move toward
  if (m_path && m_edgeIndex == m_path->size() && m_world->geometry().diff(*m_targetPosition, targetPosition).magnitude() > 0.001) {
    reset();
    m_targetPosition = targetPosition;
  }

  // starting a new path, or the target position moved by more than 2 blocks
  if (!m_targetPosition || (!m_path && !m_pathFinder) || m_world->geometry().diff(*m_targetPosition, targetPosition).magnitude() > 2.0) {
    auto grounded = movementController.onGround();
    if (m_path) {
      // if already moving on a path, collision will be disabled and we can't use MovementController::onGround() to check for ground collision
      auto const groundCollision = CollisionSet{ CollisionKind::Null, CollisionKind::Block, CollisionKind::Slippery, CollisionKind::Platform };
      grounded = onGround(movementController, movementController.position(), groundCollision);
    }
    if (*movementController.parameters().gravityEnabled && !grounded && !movementController.liquidMovement()) {
      return {};
    }
    m_startPosition = movementController.position();
    m_targetPosition = targetPosition;
    m_pathFinder = make_shared<PathFinder>(m_world, movementController.position(), *m_targetPosition, movementController.baseParameters(), m_parameters);
  }

  if (!m_pathFinder && m_path && m_edgeIndex == m_path->size())
    return true; // Reached goal

  if (m_pathFinder) {
    auto explored = m_pathFinder->explore(movementController.baseParameters().pathExploreRate.value(100.0));
    if (explored) {
      auto pathfinder = m_pathFinder;
      m_pathFinder = {};

      if (*explored) {
        auto path = *pathfinder->result();

        float newEdgeTimer = 0.0;
        float newEdgeIndex = 0.0;

        // if we have a path already, see if our paths can be merged either by splicing or fast forwarding
        bool merged = false;
        if (m_path && !path.empty()) {
          // try to fast forward on the new path
          size_t i = 0;
          // fast forward from current edge, or the last edge of the path
          auto curEdgeIndex = min(m_edgeIndex, m_path->size() - 1);
          auto& curEdge = m_path->at(curEdgeIndex);
          for (auto& edge : path) {
            if (curEdge.action == edge.action && curEdge.source.position == edge.source.position && edge.target.position == edge.target.position) {
              // fast forward on the new path
              newEdgeTimer = m_edgeTimer;
              newEdgeIndex = i;

              merged = true;
              break;
            }
            i++;
          }

          if (!merged) {
            // try to splice the new path onto the current path
            auto& newPathStart = path.at(0);
            for (size_t i = m_edgeIndex; i < m_path->size(); i += 2) {
              auto& edge = m_path->at(i);
              if (edge.target.position == newPathStart.source.position) {
                // splice the new path onto our current path up to this index
                auto newPath = m_path->slice(0, i + 1);
                newPath.appendAll(path);
                path = newPath;

                newEdgeTimer = m_edgeTimer;
                newEdgeIndex = m_edgeIndex;

                merged = true;
                break;
              }
            }
          }
        }

        if (!merged && movementController.position() != *m_startPosition) {
          // merging the paths failed, and the entity has moved from the path start position
          // try to bridge the gap from the current position to the new path
          auto bridgePathFinder = make_shared<PathFinder>(m_world, movementController.position(), *m_startPosition, movementController.baseParameters(), m_parameters);
          auto explored = bridgePathFinder->explore(movementController.baseParameters().pathExploreRate.value(100.0));

          if (explored && *explored) {
            // concatenate the bridge path with the new path
            auto newPath = *bridgePathFinder->result();
            newPath.appendAll(path);
            path = newPath;
          } else {
            // if the gap isn't bridged in a single tick, reset and start over
            reset();
            return {};
          }
        }

        if(!path.empty() && !validateEdge(movementController, path[0])) {
          // reset if the first edge is invalid
          reset();
          return false;
        }

        m_edgeTimer = newEdgeTimer;
        m_edgeIndex = newEdgeIndex;
        m_path = path;
        if (m_path->empty())
          return true;
      } else {
        reset();
        return false;
      }
    }
  }
  return {};
}

Maybe<bool> PathController::move(ActorMovementController& movementController, ActorMovementParameters const& parameters, ActorMovementModifiers const& modifiers, bool run, float dt) {
  using namespace PlatformerAStar;

  // pathfind to a new target position in the background while moving on the current path
  if (m_pathFinder && m_targetPosition)
    findPath(movementController, *m_targetPosition);

  if (!m_path)
    return {};

  m_controlFace = {};

  while (m_edgeIndex < m_path->size()) {
    auto& edge = m_path->at(m_edgeIndex);
    Vec2F delta =  m_world->geometry().diff(edge.target.position, edge.source.position);

    Vec2F sourceVelocity;
    Vec2F targetVelocity;
    switch (edge.action) {
      case Action::Jump:
        if (modifiers.jumpingSuppressed) {
          reset();
          return {};
        }
        break;
      case Action::Arc:
        sourceVelocity = edge.source.velocity.value(Vec2F());
        targetVelocity = edge.target.velocity.value(Vec2F());
        break;
      case Action::Drop:
        targetVelocity = edge.target.velocity.value(Vec2F());
        break;
      case Action::Fly:
        {
          // accelerate along path using airForce
          float angleFactor = movementController.velocity().normalized() * delta.normalized();
          float speedAlongAngle = angleFactor * movementController.velocity().magnitude();
          auto acc = parameters.airForce.value(0.0) / movementController.mass() * movementController.getScale();
          sourceVelocity = delta.normalized() * fmin(parameters.flySpeed.value(0.0) * movementController.getScale(), speedAlongAngle + acc * dt);
          targetVelocity = sourceVelocity;
        }
        break;
      case Action::Swim:
        sourceVelocity = targetVelocity = delta.normalized() * parameters.flySpeed.value(0.0f) * (1.0f - parameters.liquidImpedance.value(0.0f)) * movementController.getScale();
        break;
      case Action::Walk:
        sourceVelocity = delta.normalized() * (run ? parameters.runSpeed.value(0.0f) : parameters.walkSpeed.value(0.0f));
        sourceVelocity *= modifiers.speedModifier * movementController.getScale();
        targetVelocity = sourceVelocity;
        break;
      default: {}
    }

    Vec2F avgVelocity = (sourceVelocity + targetVelocity) / 2.0;
    float avgSpeed = avgVelocity.magnitude();
    auto edgeTime = avgSpeed > 0.0f ? delta.magnitude() / avgSpeed : 0.2;

    auto edgeProgress = m_edgeTimer / edgeTime;
    if (edgeProgress > 1.0f) {
      m_edgeTimer -= edgeTime;
      m_edgeIndex++;
      if (m_edgeIndex < m_path->size()) {
        if (!validateEdge(movementController, m_path->at(m_edgeIndex))) {
          // Logger::info("Path invalidated on {} {} {}", ActionNames.getRight(nextEdge.action), nextEdge.source.position, nextEdge.target.position);
          reset();
          return {};
        }
      }
      continue;
    }

    auto curVelocity = sourceVelocity + ((targetVelocity - sourceVelocity) * edgeProgress);
    movementController.setVelocity(curVelocity);
    auto movement = (curVelocity + sourceVelocity) / 2.0 * m_edgeTimer;
    movementController.setPosition(edge.source.position + movement);

    // Shows path and current step
    // for (size_t i = 0; i < m_path->size(); i++) {
    //   auto debugEdge = m_path->get(i);
    //   SpatialLogger::logPoint("world", debugEdge.source.position, Color::Blue.toRgba());
    //   SpatialLogger::logPoint("world", debugEdge.target.position, Color::Blue.toRgba());
    //   SpatialLogger::logLine("world", debugEdge.source.position, debugEdge.target.position, Color::Blue.toRgba());

    //   if (i == m_edgeIndex) {
    //     Vec2F velocity = debugEdge.source.velocity.orMaybe(debugEdge.target.velocity).value({ 0.0, 0.0 });
    //     SpatialLogger::logPoint("world", debugEdge.source.position, Color::Yellow.toRgba());
    //     SpatialLogger::logLine("world", debugEdge.source.position, debugEdge.target.position, Color::Yellow.toRgba());
    //     SpatialLogger::logText("world", strf("{} {}", ActionNames.getRight(debugEdge.action), curVelocity), debugEdge.source.position, Color::Yellow.toRgba());
    //   }
    // }

    if (auto direction = directionOf(delta[0]))
      m_controlFace = direction;

    m_edgeTimer += dt;
    return {};
  }

  if (auto lastEdge = m_path->maybeLast()) {
    movementController.setPosition(lastEdge->target.position);
    movementController.setVelocity(Vec2F());
  }

  // reached the end of the path, success unless we're also currently pathfinding to a new position
  if (m_pathFinder)
    return {};
  else
    return true;
}

bool PathController::validateEdge(ActorMovementController& movementController, PlatformerAStar::Edge const& edge) {
  using namespace PlatformerAStar;

  auto const groundCollision = CollisionSet{ CollisionKind::Null, CollisionKind::Block, CollisionKind::Slippery, CollisionKind::Platform };
  auto const solidCollision = CollisionSet{ CollisionKind::Null, CollisionKind::Block, CollisionKind::Slippery };

  auto const openDoors = [&](RectF const& bounds) {
  auto objects = m_world->entityQuery(bounds, entityTypeFilter<Object>());
  auto opened = objects.filtered([&](EntityPtr const& e) -> bool {
      if (auto object = as<Object>(e)) {
        if (object->isMaster()) {
          auto arg = m_world->luaRoot()->luaEngine().createString("closedDoor");
          auto res = object->callScript("hasCapability", LuaVariadic<LuaValue>{arg});
          if (res && res->is<LuaBoolean>() && res->get<LuaBoolean>()){
            m_world->sendEntityMessage(e->entityId(), "openDoor");
            return true;
          }
        }
      }
      return false;
    });
    return !opened.empty();
  };

  auto poly = movementController.collisionPoly();
  poly.translate(edge.target.position);
  if (m_world->polyCollision(poly) || movingCollision(movementController, poly)) {
    auto bounds = RectI::integral(poly.boundBox());
    // for (auto line : bounds.edges()) {
    //   SpatialLogger::logLine("world", Line2F(line), Color::Magenta.toRgba());
    // }
    if (m_world->rectTileCollision(bounds) && !m_world->rectTileCollision(bounds, solidCollision)) {
      if (!openDoors(poly.boundBox())) {
        // SpatialLogger::logPoly("world", poly, Color::Yellow.toRgba());
        return false;
      }
    } else {
      // SpatialLogger::logPoly("world", poly, Color::Red.toRgba());
      return false;
    }
  }
  //SpatialLogger::logPoly("world", poly, Color::Blue.toRgba());

  auto inLiquid = [&](Vec2F const& position) -> bool {
    auto bounds = movementController.localBoundBox().translated(position);
    auto liquidLevel = m_world->liquidLevel(bounds);
    if (liquidLevel.level >= movementController.baseParameters().minimumLiquidPercentage.value(1.0)) {
      // for (auto line : bounds.edges()) {
      //   SpatialLogger::logLine("world", line, Color::Blue.toRgba());
      // }
      return true;
    } else {
      // for (auto line : bounds.edges()) {
      //   SpatialLogger::logLine("world", line, Color::Red.toRgba());
      // }
      return false;
    }
  };

  switch (edge.action) {
    case Action::Walk:
      return onGround(movementController, edge.source.position, groundCollision);
    case Action::Swim:
      return inLiquid(edge.target.position);
    case Action::Land:
      return onGround(movementController, edge.target.position, groundCollision) || inLiquid(edge.target.position);
    case Action::Drop:
      return onGround(movementController, edge.source.position, groundCollision) && !onGround(movementController, edge.source.position, solidCollision);
    default:
      return true;
  }
}

bool PathController::movingCollision(ActorMovementController& movementController, PolyF const& collisionPoly) {
  bool collided = false;
  movementController.forEachMovingCollision(collisionPoly.boundBox(), [&](MovingCollisionId, PhysicsMovingCollision, PolyF poly, RectF) {
      if (poly.intersects(collisionPoly)) {
        // set collided and stop iterating
        collided = true;
        return false;
      }
      return true;
    });
  return collided;
}

bool PathController::onGround(ActorMovementController const& movementController, Vec2F const& position, CollisionSet const& collisionSet) const {
    auto bounds = RectI::integral(movementController.localBoundBox().translated(position));
    Vec2I min = Vec2I(bounds.xMin(), bounds.yMin() - 1);
    Vec2I max = Vec2I(bounds.xMax(), bounds.yMin());
    // for (auto line : RectF(Vec2F(min), Vec2F(max)).edges()) {
    //   SpatialLogger::logLine("world", line, m_world->rectTileCollision(RectI(min, max), collisionSet) ? Color::Blue.toRgba() : Color::Red.toRgba());
    // }
    return m_world->rectTileCollision(RectI(min, max), collisionSet);
}

}
