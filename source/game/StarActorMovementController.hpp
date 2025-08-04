#pragma once

#include "StarMovementController.hpp"
#include "StarPlatformerAStarTypes.hpp"
#include "StarAnchorableEntity.hpp"
#include "StarGameTimers.hpp"

namespace Star {

STAR_EXCEPTION(ActorMovementControllerException, MovementControllerException);

STAR_CLASS(ActorMovementController);
STAR_CLASS(PathController);

struct ActorJumpProfile {
  ActorJumpProfile();
  ActorJumpProfile(Json const& config);

  Json toJson() const;

  ActorJumpProfile merge(ActorJumpProfile const& rhs) const;

  Maybe<float> jumpSpeed;
  Maybe<float> jumpControlForce;
  Maybe<float> jumpInitialPercentage;

  // If this is greater than 0.0, jump hold time is limited by this factor.
  Maybe<float> jumpHoldTime;
  // If this is greater than 0.0, then the total jump time for *all jumps in a
  // multi jump set* is limited by this factor.
  Maybe<float> jumpTotalHoldTime;

  Maybe<bool> multiJump;
  Maybe<float> reJumpDelay;
  Maybe<bool> autoJump;
  Maybe<bool> collisionCancelled;
};

DataStream& operator>>(DataStream& ds, ActorJumpProfile& movementParameters);
DataStream& operator<<(DataStream& ds, ActorJumpProfile const& movementParameters);

// A not-quite superset of MovementParameters, with some fields from
// MovementParameters ignored because they make no sense, and other fields
// expanded out to different cases based on Actor specific things.
struct ActorMovementParameters {
  // Load sensible defaults from a config file.
  static ActorMovementParameters sensibleDefaults();

  // Construct parameters from config with only those specified in the config
  // set, if any.
  explicit ActorMovementParameters(Json const& config = Json());

  Json toJson() const;

  // Merge the given set of movement parameters on top of this one, with any
  // set parameters in rhs overwriting the ones in this set.
  ActorMovementParameters merge(ActorMovementParameters const& rhs) const;

  Maybe<float> mass;
  Maybe<float> gravityMultiplier;
  Maybe<float> liquidBuoyancy;
  Maybe<float> airBuoyancy;
  Maybe<float> bounceFactor;
  Maybe<bool> stopOnFirstBounce;
  Maybe<bool> enableSurfaceSlopeCorrection;
  Maybe<float> slopeSlidingFactor;
  Maybe<float> maxMovementPerStep;
  Maybe<float> maximumCorrection;
  Maybe<float> speedLimit;

  Maybe<PolyF> standingPoly;
  Maybe<PolyF> crouchingPoly;

  Maybe<bool> stickyCollision;
  Maybe<float> stickyForce;

  Maybe<float> walkSpeed;
  Maybe<float> runSpeed;
  Maybe<float> flySpeed;

  Maybe<float> airFriction;
  Maybe<float> liquidFriction;

  Maybe<float> minimumLiquidPercentage;
  Maybe<float> liquidImpedance;

  Maybe<float> normalGroundFriction;
  Maybe<float> ambulatingGroundFriction;

  Maybe<float> groundForce;
  Maybe<float> airForce;
  Maybe<float> liquidForce;

  ActorJumpProfile airJumpProfile;
  ActorJumpProfile liquidJumpProfile;

  Maybe<float> fallStatusSpeedMin;
  Maybe<int> fallThroughSustainFrames;
  Maybe<float> maximumPlatformCorrection;
  Maybe<float> maximumPlatformCorrectionVelocityFactor;

  Maybe<StringSet> physicsEffectCategories;

  Maybe<float> groundMovementMinimumSustain;
  Maybe<float> groundMovementMaximumSustain;
  Maybe<float> groundMovementCheckDistance;

  Maybe<bool> collisionEnabled;
  Maybe<bool> frictionEnabled;
  Maybe<bool> gravityEnabled;

  Maybe<float> pathExploreRate;
};

DataStream& operator>>(DataStream& ds, ActorMovementParameters& movementParameters);
DataStream& operator<<(DataStream& ds, ActorMovementParameters const& movementParameters);

// A set of normalized values that act as "modifiers" or "bonuses" to movement,
// and can be combined sensibly.  A modifier of 0.0 represents a 0% change, a
// modifier of 0.2 represents a 20% increase, and a modifier of -0.2 represents
// a 20% decrease.  Also includes some flags that disable functionality
// combined with logical OR.
struct ActorMovementModifiers {
  explicit ActorMovementModifiers(Json const& config = Json());

  Json toJson() const;

  // Combines each modifier value through addition.
  ActorMovementModifiers combine(ActorMovementModifiers const& rhs) const;

  float groundMovementModifier;
  float liquidMovementModifier;
  float speedModifier;
  float airJumpModifier;
  float liquidJumpModifier;

  bool runningSuppressed;
  bool jumpingSuppressed;
  // Suppresses left, right, down, crouch, jump, and fly controls
  bool movementSuppressed;
  bool facingSuppressed;
};

DataStream& operator>>(DataStream& ds, ActorMovementModifiers& movementModifiers);
DataStream& operator<<(DataStream& ds, ActorMovementModifiers const& movementModifiers);

class ActorMovementController : public virtual MovementController {
public:
  // Constructs an ActorMovementController with parameters loaded from sensible
  // defaults, and the given parameters (if any) applied on top of them.
  explicit ActorMovementController(ActorMovementParameters const& parameters = ActorMovementParameters());

  // Currently active parameters.
  ActorMovementParameters const& baseParameters() const;

  // Apply any set parameters from the given set on top of the current set.
  void updateBaseParameters(ActorMovementParameters const& parameters);

  // Reset the parameters from the sensible defaults, and apply the given
  // parameters (if any) on top of them.
  void resetBaseParameters(ActorMovementParameters const& parameters = ActorMovementParameters());

  // Currently active modifiers.
  ActorMovementModifiers const& baseModifiers() const;

  // Combine the given modifiers with the already active modifiers.
  void updateBaseModifiers(ActorMovementModifiers const& modifiers);

  // Reset all modifiers to the given values
  void resetBaseModifiers(ActorMovementModifiers const& modifiers = ActorMovementModifiers());

  // Stores and loads position, velocity, rotation, movingDirection,
  // facingDirection, and crouching
  Json storeState() const;
  void loadState(Json const& state);

  // Optionaly anchor this ActorMovementController to the given
  // AnchorableEntity.  position, rotation, and facing direction will be set
  // based on the entity anchor alone every tick, and on slaved
  // ActorMovementControllers it will be updated based on the actual slave-side
  // AnchorableEntity state.
  void setAnchorState(EntityAnchorState anchorState);
  void resetAnchorState();
  Maybe<EntityAnchorState> anchorState() const;
  EntityAnchorConstPtr entityAnchor() const;

  // ActorMovementController position and rotation honor the entity anchor, if
  // an anchor is set.
  Vec2F position() const override;
  float rotation() const override;

  bool walking() const;
  bool running() const;
  Direction movingDirection() const;
  Direction facingDirection() const;
  bool crouching() const;
  bool flying() const;
  bool falling() const;
  bool canJump() const;
  bool jumping() const;
  // Slightly different than onGround, in that this is sustained for a few
  // extra frames of movement before it becomes false.
  bool groundMovement() const;
  bool liquidMovement() const;
  bool pathfinding() const;

  // Basic direct physics controls that can be called multiple times per
  // update and will be combined.
  void controlRotation(float rotationRate);
  void controlAcceleration(Vec2F const& acceleration);
  void controlForce(Vec2F const& force);
  void controlApproachVelocity(Vec2F const& targetVelocity, float maxControlForce);
  void controlApproachVelocityAlongAngle(float angle, float targetVelocity, float maxControlForce, bool positiveOnly = false);
  void controlApproachXVelocity(float targetXVelocity, float maxControlForce);
  void controlApproachYVelocity(float targetYVelocity, float maxControlForce);

  // Apply ActorMovementParameters / ActorMovementModifiers only as long as
  // the controls are active.  Can be called multiple times per update and
  // will be combined.
  void controlParameters(ActorMovementParameters const& parameters);
  void controlModifiers(ActorMovementModifiers const& modifiers);

  // Higher level movement controls that use forces defined in the
  // ActorMovementParameters.  Calling more than once per update will override
  // previous calls.
  void controlMove(Direction direction, bool run = true);
  void controlFace(Direction direction);
  void controlDown();
  void controlCrouch();
  void controlJump(bool jumpEvenIfUnable = false);
  void controlFly(Vec2F const& velocity);

  Maybe<pair<Vec2F, bool>> pathMove(Vec2F const& pathPosition, bool run = false, Maybe<PlatformerAStar::Parameters> const& parameters = {});
  Maybe<pair<Vec2F, bool>> controlPathMove(Vec2F const& pathPosition, bool run = false, Maybe<PlatformerAStar::Parameters> const& parameters = {});

  // Used for user controller input.
  void setMoveSpeedMultiplier(float multiplier = 1.0f);

  // Clears all control data.
  void clearControls();

  // Integrates the ActorMovementController and applies all
  // the control data and clears it for the next step.
  void tickMaster(float dt);

  void tickSlave(float dt);


private:
  struct ApproachVelocityCommand {
    Vec2F targetVelocity;
    float maxControlForce;
  };

  struct ApproachVelocityAlongAngleCommand {
    float alongAngle;
    float targetVelocity;
    float maxControlForce;
    bool positiveOnly;
  };

  void applyMCParameters(ActorMovementParameters const& parameters);
  void doSetAnchorState(Maybe<EntityAnchorState> anchorState);

  ActorMovementParameters m_baseParameters;
  ActorMovementModifiers m_baseModifiers;

  // State data

  NetElementBool m_walking;
  NetElementBool m_running;
  NetElementEnum<Direction> m_movingDirection;
  NetElementEnum<Direction> m_facingDirection;
  NetElementBool m_crouching;
  NetElementBool m_flying;
  NetElementBool m_falling;
  NetElementBool m_canJump;
  NetElementBool m_jumping;
  NetElementBool m_groundMovement;
  NetElementBool m_liquidMovement;
  NetElementData<Maybe<EntityAnchorState>> m_anchorState;
  EntityAnchorConstPtr m_entityAnchor;

  // Command data

  float m_controlRotationRate;
  Vec2F m_controlAcceleration;
  Vec2F m_controlForce;
  List<ApproachVelocityCommand> m_controlApproachVelocities;
  List<ApproachVelocityAlongAngleCommand> m_controlApproachVelocityAlongAngles;

  Maybe<Direction> m_controlMove;
  Maybe<Direction> m_controlFace;
  bool m_controlRun;
  bool m_controlCrouch;
  bool m_controlDown;
  bool m_controlJump;
  bool m_controlJumpAnyway;

  Maybe<Vec2F> m_controlFly;

  Maybe<pair<Vec2F, bool>> m_controlPathMove;
  Maybe<pair<Vec2F, bool>> m_pathMoveResult;
  PathControllerPtr m_pathController;

  ActorMovementParameters m_controlParameters;
  ActorMovementModifiers m_controlModifiers;

  // Internal state data

  int m_fallThroughSustain;
  bool m_lastControlJump;
  bool m_lastControlDown;
  float m_moveSpeedMultiplier;

  GameTimer m_reJumpTimer;
  Maybe<GameTimer> m_jumpHoldTimer;
  GameTimer m_groundMovementSustainTimer;

  // Target horizontal velocity for walking / running
  float m_targetHorizontalAmbulatingVelocity;
};

class PathController {
public:
  PathController(World* world);

  PlatformerAStar::Parameters const& parameters();
  void setParameters(PlatformerAStar::Parameters const& parameters);
  void reset();
  bool pathfinding() const;
  Maybe<Vec2F> targetPosition() const;
  Maybe<Direction> facing() const;
  Maybe<PlatformerAStar::Action> curAction() const;

  // return true for reaching goal, false for failing to find path, nothing for running
  Maybe<bool> findPath(ActorMovementController& movementController, Vec2F const& targetPosition);
  Maybe<bool> move(ActorMovementController& movementController, ActorMovementParameters const& parameters, ActorMovementModifiers const& modifiers, bool run, float dt);
private:
  bool validateEdge(ActorMovementController& movementController, PlatformerAStar::Edge const& edge);
  bool movingCollision(ActorMovementController& movementController, PolyF const& collisionPoly);

private:
  bool onGround(ActorMovementController const& movementController, Vec2F const& position, CollisionSet const& collisionSet) const;

  World* m_world;
  PlatformerAStar::Parameters m_parameters;

  Maybe<Vec2F> m_startPosition;
  Maybe<Vec2F> m_targetPosition;
  PlatformerAStar::PathFinderPtr m_pathFinder;

  Maybe<Direction> m_controlFace;

  size_t m_edgeIndex;
  float m_edgeTimer;
  Maybe<PlatformerAStar::Path> m_path;
};

}
