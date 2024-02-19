#include "StarLogging.hpp"
#include "StarRandom.hpp"
#include "StarPlatformerAStar.hpp"
#include "StarWorld.hpp"
#include "StarLiquidTypes.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

namespace PlatformerAStar {

  // The desired spacing between nodes:
  float const NodeGranularity = 1.f;
  float const SimulateArcGranularity = 0.5f;

  float const DefaultMaxDistance = 50.0f;
  float const DefaultSmallJumpMultiplier = 0.75f;
  float const DefaultJumpDropXMultiplier = 0.125f;

  float const DefaultSwimCost = 40.0f;
  float const DefaultJumpCost = 3.0f;
  float const DefaultLiquidJumpCost = 10.0f;
  float const DefaultDropCost = 3.0f;

  float const DefaultMaxLandingVelocity = -5.0f;

  // Bounding boxes are shrunk slightly to work around floating point rounding
  // errors.
  float const BoundBoxRoundingErrorScaling = 0.99f;

  CollisionSet const CollisionSolid{CollisionKind::Null, CollisionKind::Slippery, CollisionKind::Block, CollisionKind::Slippery};

  CollisionSet const CollisionFloorOnly{CollisionKind::Null, CollisionKind::Block, CollisionKind::Slippery, CollisionKind::Platform};

  CollisionSet const CollisionDynamic{CollisionKind::Dynamic};

  CollisionSet const CollisionAny{CollisionKind::Null,
      CollisionKind::Platform,
      CollisionKind::Dynamic,
      CollisionKind::Slippery,
      CollisionKind::Block};

  PathFinder::PathFinder(World* world,
      Vec2F searchFrom,
      Vec2F searchTo,
      ActorMovementParameters movementParameters,
      Parameters searchParameters)
    : m_world(world),
      m_searchFrom(searchFrom),
      m_searchTo(searchTo),
      m_movementParams(std::move(movementParameters)),
      m_searchParams(std::move(searchParameters)) {
    initAStar();
  }

  PathFinder::PathFinder(PathFinder const& rhs) {
    operator=(rhs);
  }

  PathFinder& PathFinder::operator=(PathFinder const& rhs) {
    m_world = rhs.m_world;
    m_searchFrom = rhs.m_searchFrom;
    m_searchTo = rhs.m_searchTo;
    m_movementParams = rhs.m_movementParams;
    m_searchParams = rhs.m_searchParams;
    initAStar();
    return *this;
  }

  Maybe<bool> PathFinder::explore(Maybe<unsigned> maxExploreNodes) {
    return m_astar->explore(maxExploreNodes);
  }

  Maybe<Path> const& PathFinder::result() const {
    return m_astar->result();
  }

  void PathFinder::initAStar() {
    auto heuristicCostFn = [this](Node const& fromNode, Node const& toNode) -> float {
      return heuristicCost(fromNode.position, toNode.position);
    };
    auto goalReachedFn = [this](Node const& node) -> bool {
      if (m_searchParams.mustEndOnGround && (!onGround(node.position) || node.velocity.isValid()))
        return false;
      return distance(node.position, m_searchTo) < NodeGranularity;
    };
    auto neighborsFn = [this](Node const& node, List<Edge>& result) {
      auto neighborFilter = [this](Edge const& edge) -> bool {
        return distance(edge.source.position, m_searchFrom) <= m_searchParams.maxDistance.value(DefaultMaxDistance);
      };
      neighbors(node, result);
      result.filter(neighborFilter);
    };
    auto validateEndFn = [this](Edge const& edge) -> bool {
      if (!m_searchParams.mustEndOnGround)
        return true;
      return onGround(edge.target.position) && edge.action != Action::Jump;
    };

    Vec2F roundedFrom = roundToNode(m_searchFrom);
    Vec2F roundedTo = roundToNode(m_searchTo);

    m_astar = AStar::Search<Edge, Node>(heuristicCostFn,
        neighborsFn,
        goalReachedFn,
        m_searchParams.returnBest,
        {validateEndFn},
        m_searchParams.maxFScore,
        m_searchParams.maxNodesToSearch);
    m_astar->start(Node{roundedFrom, {}}, Node{roundedTo, {}});
  }

  float PathFinder::heuristicCost(Vec2F const& fromPosition, Vec2F const& toPosition) const {
    // This function is used to estimate the cost of travel between two nodes.
    // Underestimating the actual cost results in A* giving the optimal path.
    // Overestimating results in A* finding a non-optimal path, but terminating
    // more quickly when there is a route to the target.
    // We don't really care all that much about getting the optimal path as long
    // as we get one that looks feasible, so we deliberately overestimate here.
    Vec2F diff = m_world->geometry().diff(fromPosition, toPosition);
    // Manhattan distance * 2:
    return 2.0f * (abs(diff[0]) + abs(diff[1]));
  }

  Edge PathFinder::defaultCostEdge(Action action, Node const& source, Node const& target) const {
    return Edge{distance(source.position, target.position), action, Vec2F(0, 0), source, target};
  }

  void PathFinder::neighbors(Node const& node, List<Edge>& neighbors) const {
    if (node.velocity.isValid()) {
      // Follow the current trajectory. Most of the time, this will only produce
      // one neighbor to avoid massive search space explosion, however one
      // change of X velocity is allowed at the peak of a jump.
      getArcNeighbors(node, neighbors);

    } else if (inLiquid(node.position)) {
      getSwimmingNeighbors(node, neighbors);

    } else if (acceleration(node.position)[1] == 0.0f) {
      getFlyingNeighbors(node, neighbors);

    } else if (onGround(node.position)) {
      getWalkingNeighbors(node, neighbors);

      if (!onSolidGround(node.position)) {
        // Add a node for dropping through a platform.
        // When that node is explored, if it's not onGround, its neighbors will
        // be falling to the ground.
        getDropNeighbors(node, neighbors);
      }

      getJumpingNeighbors(node, neighbors);
    } else {
      // We're in the air, and can only fall now
      getFallingNeighbors(node, neighbors);
    }
  }

  void PathFinder::getDropNeighbors(Node const& node, List<Edge>& neighbors) const {
    auto dropPosition = node.position + Vec2F(0, -1);
    // The physics of platforms don't allow us to drop through platforms resting
    // directly on solid surfaces. So if there is solid ground below the
    // platform, don't allow dropping through the platform:
    if (!onSolidGround(dropPosition)) {
      float dropCost = m_searchParams.dropCost.value(DefaultDropCost);
      float acc = acceleration(node.position)[1];
      float dropSpeed = acc * sqrt(2.0 / abs(acc));
      neighbors.append(Edge{dropCost, Action::Drop, Vec2F(0, 0), node, Node{dropPosition, Vec2F(0, dropSpeed)}});
    }
  }

  void PathFinder::getWalkingNeighborsInDirection(Node const& node, List<Edge>& neighbors, float direction) const {
    auto addNode = [this, &node, &neighbors](Node const& target) {
      neighbors.append(defaultCostEdge(Action::Walk, node, target));
    };

    Vec2F forward = node.position + Vec2F(direction, 0);
    Vec2F forwardAndUp = node.position + Vec2F(direction, 1);
    Vec2F forwardAndDown = node.position + Vec2F(direction, -1);

    RectF bounds = boundBox(node.position);

    bool slopeDown = false;
    bool slopeUp = false;
    Vec2F forwardGroundPos = direction > 0 ? Vec2F(bounds.xMax(), bounds.yMin()) : Vec2F(bounds.xMin(), bounds.yMin());
    Vec2F backGroundPos = direction < 0 ? Vec2F(bounds.xMax(), bounds.yMin()) : Vec2F(bounds.xMin(), bounds.yMin());
    m_world->forEachCollisionBlock(groundCollisionRect(node.position, BoundBoxKind::Full).padded(1), [&](CollisionBlock const& block) {
      if (slopeUp || slopeDown) return;
      for (size_t i = 0; i < block.poly.sides(); ++i) {
        auto side = block.poly.side(i);
        auto sideDir = side.direction();

        auto lower = side.min()[1] < side.max()[1] ? side.min() : side.max();
        auto upper = side.min()[1] > side.max()[1] ? side.min() : side.max();
        if (sideDir[0] != 0 && sideDir[1] != 0 && (lower[1] == round(forwardGroundPos[1]) || upper[1] == round(forwardGroundPos[1]))) {
          float yDir = (sideDir[1] / sideDir[0]) * direction;
          if (abs(m_world->geometry().diff(forwardGroundPos, lower)[0]) < 0.5 && yDir > 0)
            slopeUp = true;
          else if (abs(m_world->geometry().diff(backGroundPos, upper)[0]) < 0.5 && yDir < 0)
            slopeDown = true;

          if (slopeUp || slopeDown) break;
        }
      }
    });

    Maybe<float> walkSpeed = m_movementParams.walkSpeed;
    Maybe<float> runSpeed = m_movementParams.runSpeed;

    // Check if it's possible to walk up a block like a ramp first
    if (slopeUp && onGround(forwardAndUp) && validPosition(forwardAndUp)) {
      // Walk up a slope
      addNode(Node{forwardAndUp, {}});
    } else if (validPosition(forward) && onGround(forward)) {
      // Walk along a flat plane
      addNode(Node{forward, {}});
    } else if (slopeDown && validPosition(forward) && validPosition(forwardAndDown) && onGround(forwardAndDown)) {
      // Walk down a slope
      addNode(Node{forwardAndDown, {}});
    } else if (validPosition(forward)) {
      // Fall off a ledge
      bounds = m_movementParams.standingPoly->boundBox();
      float back = direction > 0 ? bounds.xMin() : bounds.xMax();
      forward[0] -= (1 - fmod(abs(back), 1.0f)) * direction;
      if (walkSpeed.isValid())
        addNode(Node{forward, Vec2F{copysign(*walkSpeed, direction), 0.0f}});
      if (runSpeed.isValid())
        addNode(Node{forward, Vec2F{copysign(*runSpeed, direction), 0.0f}});
    }
  }

  void PathFinder::getWalkingNeighbors(Node const& node, List<Edge>& neighbors) const {
    getWalkingNeighborsInDirection(node, neighbors, NodeGranularity);
    getWalkingNeighborsInDirection(node, neighbors, -NodeGranularity);
  }

  void PathFinder::getFallingNeighbors(Node const& node, List<Edge>& neighbors) const {
    forEachArcNeighbor(node,  0.0f, [this, &node, &neighbors](Node const& target, bool landed) {
      neighbors.append(defaultCostEdge(Action::Arc, node, target));
      if (landed) {
        neighbors.append(defaultCostEdge(Action::Land, target, Node{target.position, {}}));
      }
    });
  }

  void PathFinder::getJumpingNeighbors(Node const& node, List<Edge>& neighbors) const {
    if (Maybe<float> jumpSpeed = m_movementParams.airJumpProfile.jumpSpeed) {
      float jumpCost = m_searchParams.jumpCost.value(DefaultJumpCost);
      if (inLiquid(node.position))
        jumpCost = m_searchParams.liquidJumpCost.value(DefaultLiquidJumpCost);
      auto addVel = [jumpCost, &node, &neighbors](Vec2F const& vel) {
        neighbors.append(Edge{jumpCost, Action::Jump, vel, node, node.withVelocity(vel)});
      };

      forEachArcVelocity(*jumpSpeed, addVel);
      forEachArcVelocity(*jumpSpeed * m_searchParams.smallJumpMultiplier.value(DefaultSmallJumpMultiplier), addVel);
    }
  }

  void PathFinder::getSwimmingNeighbors(Node const& node, List<Edge>& neighbors) const {
    // TODO avoid damaging liquids, e.g. lava

    // We assume when we're swimming we can move freely against gravity
    getFlyingNeighbors(node, neighbors);

    // Also allow jumping out of the water if we're at the surface:
    RectF box = boundBox(node.position);
    if (acceleration(node.position)[1] != 0.0f && m_world->liquidLevel(box).level < 1.0f)
      getJumpingNeighbors(node, neighbors);

    neighbors.filter([this](Edge& edge) -> bool {
      return inLiquid(edge.target.position);
    });
    neighbors.transform([this](Edge& edge) -> Edge& {
      if (edge.action == Action::Fly)
        edge.action = Action::Swim;
      edge.cost *= m_searchParams.swimCost.value(DefaultSwimCost);
      return edge;
    });
  }

  void PathFinder::getFlyingNeighbors(Node const& node, List<Edge>& neighbors) const {
    auto addNode = [this, &node, &neighbors](
        Node const& target) { neighbors.append(defaultCostEdge(Action::Fly, node, target)); };

    Vec2F roundedPosition = roundToNode(node.position);
    for (int dx = -1; dx < 2; ++dx) {
      for (int dy = -1; dy < 2; ++dy) {
        Vec2F newPosition = roundedPosition + Vec2F(dx, dy) * NodeGranularity;
        if (validPosition(newPosition)) {
          addNode(Node{newPosition, {}});
        }
      }
    }
  }

  void PathFinder::getArcNeighbors(Node const& node, List<Edge>& neighbors) const {
    auto addNode = [this, &node, &neighbors](Node const& target, bool landed) {
      neighbors.append(defaultCostEdge(Action::Arc, node, target));
      if (landed) {
        neighbors.append(defaultCostEdge(Action::Land, target, Node{target.position, {}}));
      }
    };

    simulateArc(node, addNode);
  }

  void PathFinder::forEachArcVelocity(float yVelocity, function<void(Vec2F)> func) const {
    Maybe<float> walkSpeed = m_movementParams.walkSpeed;
    Maybe<float> runSpeed = m_movementParams.runSpeed;

    func(Vec2F(0, yVelocity));
    if (m_searchParams.enableWalkSpeedJumps && walkSpeed.isValid()) {
      func(Vec2F(*walkSpeed, yVelocity));
      func(Vec2F(-*walkSpeed, yVelocity));
    }
    if (runSpeed.isValid()) {
      func(Vec2F(*runSpeed, yVelocity));
      func(Vec2F(-*runSpeed, yVelocity));
    }
  }

  void PathFinder::forEachArcNeighbor(Node const& node, float yVelocity, function<void(Node, bool)> func) const {
    Vec2F position = roundToNode(node.position);
    forEachArcVelocity(yVelocity,
        [this, &position, &func](Vec2F const& vel) {
          simulateArc(Node{position, vel}, func);
        });
  }

  Vec2F PathFinder::acceleration(Vec2F pos) const {
    auto const& parameters = m_movementParams;
    float gravity = m_world->gravity(pos) * parameters.gravityMultiplier.value(1.0f);
    if (!parameters.gravityEnabled.value(true) || parameters.mass.value(0.0f) == 0.0f)
      gravity = 0.0f;
    float buoyancy = parameters.airBuoyancy.value(0.0f);
    return Vec2F(0, -gravity * (1.0f - buoyancy));
  }

  Vec2F PathFinder::simulateArcCollision(Vec2F position, Vec2F velocity, float dt, bool& collidedX, bool& collidedY) const {
    // Returns the new position and whether a collision in the Y axis occurred.
    // We avoid actual collision detection / resolution as that would make
    // pathfinding very expensive.

    Vec2F newPosition = position + velocity * dt;
    if (validPosition(newPosition)) {
      collidedX = collidedY = false;
      return newPosition;
    } else {
      collidedX = collidedY = true;
      
      if (validPosition(Vec2F(newPosition[0], position[1]))) {
        collidedX = false;
        position[0] = newPosition[0];
      } else if (validPosition(Vec2F(position[0], newPosition[1]), BoundBoxKind::Stand)) {
        collidedY = false;
        position[1] = newPosition[1];
      }
    }

    return position;
  }

  void PathFinder::simulateArc(Node const& node, function<void(Node, bool)> func) const {
    Vec2F position = node.position;
    Vec2F velocity = *node.velocity;
    bool jumping = velocity[1] > 0.0f;
    float maxLandingVelocity = m_searchParams.maxLandingVelocity.value(DefaultMaxLandingVelocity);
    
    Vec2F acc = acceleration(position);
    if (acc[1] == 0.0f)
      return;

    // Simulate until we're roughly NodeGranularity distance from the previous
    // node
    Vec2F start = roundToNode(node.position);
    Vec2F rounded = start;
    while (rounded == start) {
      float speed = velocity.magnitude();
      float dt = fmin(0.2f, speed != 0.0f ? SimulateArcGranularity / speed : sqrt(SimulateArcGranularity * 2.0 * abs(acc[1])));

      bool collidedX = false, collidedY = false;
      position = simulateArcCollision(position, velocity, dt, collidedX, collidedY);
      rounded = roundToNode(position);

      if (collidedY) {
        // We've either landed or hit our head on the ceiling
        if (!jumping) {
          // Landed
          if (velocity[1] < maxLandingVelocity)
            func(Node{rounded, velocity}, true);
          return;
        } else if (onGround(rounded, BoundBoxKind::Stand)) {
          // Simultaneously hit head and landed -- this is a gap we can *just*
          // fit
          // through. No checking of the maxLandingVelocity, since the tiles'
          // polygons are rounded, making this an easier target to hit than it
          // seems.
          func(Node{rounded, velocity}, true);
          return;
        }
        // Hit ceiling. Remove y velocity
        velocity[1] = 0.0f;

      } else if (collidedX) {
        // Hit a wall, just fall down
        velocity[0] = 0.0f;
        if (jumping) {
          velocity[1] = 0.0f;
          jumping = false;
        }
      }

      velocity += acc * dt;
      if (jumping && velocity[1] <= 0.0f) {
        // We've reached a peak in the jump and the entity can now choose to
        // change direction.
        Maybe<float> runSpeed = m_movementParams.runSpeed;
        Maybe<float> walkSpeed = m_movementParams.walkSpeed;
        float crawlMultiplier = m_searchParams.jumpDropXMultiplier.value(DefaultJumpDropXMultiplier);

        if ((*node.velocity)[0] != 0.0f || m_searchParams.enableVerticalJumpAirControl) {
          if (runSpeed.isValid())
            func(Node{position, Vec2F{copysign(*runSpeed, velocity[0]), 0.0f}}, false);
          if (m_searchParams.enableWalkSpeedJumps && walkSpeed.isValid()) {
            func(Node{position, Vec2F{copysign(*walkSpeed, velocity[0]), 0.0f}}, false);
            func(Node{position, Vec2F{copysign(*walkSpeed * crawlMultiplier, velocity[0]), 0.0f}}, false);
          }
        }
        // Only fall straight down if we were going straight up originally.
        // Going from an arc to falling straight down looks unnatural.
        if ((*node.velocity)[0] == 0.0f) {
          func(Node{position, Vec2F(0.0f, 0.0f)}, false);
        }
        return;
      }
    }

    if (!jumping) {
      if (velocity[1] < maxLandingVelocity) {
        if (onGround(rounded, BoundBoxKind::Stand) || inLiquid(rounded)) {
          // Collision with platform
          func(Node{rounded, velocity}, true);
          return;
        }
      }
    }

    starAssert(velocity[1] != 0.0f);
    func(Node{position, velocity}, false);
    return;
  }

  bool PathFinder::validPosition(Vec2F pos, BoundBoxKind boundKind) const {
    return !m_world->rectTileCollision(RectI::integral(boundBox(pos, boundKind)), CollisionSolid);
  }

  bool PathFinder::onGround(Vec2F pos, BoundBoxKind boundKind) const {
    auto groundRect = groundCollisionRect(pos, boundKind);
    // Check there is something under the feet.
    // We allow walking over the tops of objects (e.g. trapdoors) without being
    // able to float inside objects.
    if (m_world->rectTileCollision(RectI::integral(boundBox(pos, boundKind)), CollisionDynamic))
      // We're inside an object. Don't collide with object directly below our
      // feet:
      return m_world->rectTileCollision(groundRect, CollisionFloorOnly);
    // Not inside an object, allow colliding with objects below our feet:
    // We need to be for sure above platforms, but can be up to a full tile
    // below the top of solid blocks because rounded collision polys
    return m_world->rectTileCollision(groundRect, CollisionAny) || m_world->rectTileCollision(groundRect.translated(Vec2I(0, 1)), CollisionSolid);
  }

  bool PathFinder::onSolidGround(Vec2F pos) const {
    return m_world->rectTileCollision(groundCollisionRect(pos, BoundBoxKind::Drop), CollisionSolid);
  }

  bool PathFinder::inLiquid(Vec2F pos) const {
    RectF box = boundBox(pos);
    return m_world->liquidLevel(box).level >= m_movementParams.minimumLiquidPercentage.value(0.5f);
  }

  RectF PathFinder::boundBox(Vec2F pos, BoundBoxKind boundKind) const {
    RectF boundBox;
    if (boundKind == BoundBoxKind::Drop && m_searchParams.droppingBoundBox) {
      boundBox = *m_searchParams.droppingBoundBox;
    } else if (boundKind == BoundBoxKind::Stand && m_searchParams.standingBoundBox) {
      boundBox = *m_searchParams.standingBoundBox;
    } else if (m_searchParams.boundBox.isValid()) {
      boundBox = *m_searchParams.boundBox;
    } else {
      boundBox = m_movementParams.standingPoly->boundBox();
    }

    boundBox.scale(BoundBoxRoundingErrorScaling);
    boundBox.translate(pos);
    return boundBox;
  }

  RectI PathFinder::groundCollisionRect(Vec2F pos, BoundBoxKind boundKind) const {
    RectI bounds = RectI::integral(boundBox(pos, boundKind));

    Vec2I min = Vec2I(bounds.xMin(), bounds.yMin() - 1);
    Vec2I max = Vec2I(bounds.xMax(), bounds.yMin());
    // Return a 1-tile-thick rectangle below the 'feet' of the entity
    return RectI(min, max);
  }

  Vec2I PathFinder::groundNodePosition(Vec2F pos) const {
    RectI bounds = RectI::integral(boundBox(pos));

    return Vec2I(floor(pos[0]), bounds.yMin() - 1);
  }

  Vec2F PathFinder::roundToNode(Vec2F pos) const {
    // Round pos to the nearest node.

    // Work out the distance from the entity's origin to the bottom of its
    // feet. We round Y relative to this so that we ensure we're able to
    // generate
    // paths through gaps that are *just* tall enough for the entity to fit
    // through.
    RectF boundBox;
    if (m_searchParams.boundBox.isValid()) {
      boundBox = *m_searchParams.boundBox;
    } else {
      boundBox = m_movementParams.standingPoly->boundBox();
    }
    float bottom = boundBox.yMin();

    float x = round(pos[0] / NodeGranularity) * NodeGranularity;
    float y = round((pos[1] + bottom) / NodeGranularity) * NodeGranularity - bottom;
    return {x, y};
  }

  float PathFinder::distance(Vec2F a, Vec2F b) const {
    return m_world->geometry().diff(a, b).magnitude();
  }
}

}
