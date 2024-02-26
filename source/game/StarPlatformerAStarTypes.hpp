#pragma once

#include "StarVector.hpp"
#include "StarRect.hpp"
#include "StarBiMap.hpp"
#include "StarAStar.hpp"

namespace Star {
namespace PlatformerAStar {

STAR_CLASS(PathFinder);

struct Node {
  Node withVelocity(Vec2F velocity) const;

  bool operator<(Node const& other) const;

  Vec2F position;
  Maybe<Vec2F> velocity; // Only valid when jumping/falling
};

enum class Action { Walk, Jump, Arc, Drop, Swim, Fly, Land };
extern EnumMap<Action> const ActionNames;

struct Edge {
  float cost;
  Action action;
  Vec2F jumpVelocity;
  Node source;
  Node target;
};

typedef AStar::Path<Edge> Path;

struct Parameters {
  // Maximum distance from the start node to search for a path to the target
  // node
  Maybe<float> maxDistance;
  // If true, returns the path to the closest node to the target found, if a
  // path to the target itself could not be found.
  // Otherwise, findPath will return a None value.
  bool returnBest;
  // If true, end the path only on ground
  bool mustEndOnGround;
  // If true, allows jumps to have the entity's walk speed as horizontal
  // velocity
  bool enableWalkSpeedJumps;
  // if true, allows perfectly vertical jumps to change horizontal velocity at
  // the peak
  bool enableVerticalJumpAirControl;
  // Multiplies the cost of edges going through liquids. Can be used to
  // penalize or promote paths involving swiming.
  Maybe<float> swimCost;
  // The cost of jump edges.
  Maybe<float> jumpCost;
  // The cost of jump edges that start in liquids.
  Maybe<float> liquidJumpCost;
  // The cost of dropping through a platform.
  Maybe<float> dropCost;
  // If set, will be the default bounding box, otherwise will use
  // movementParameters.standingPoly.
  Maybe<RectF> boundBox;
  // The bound box used for checking if the entity can stand at a position
  // Should be thinner than the full bound box
  Maybe<RectF> standingBoundBox;
  // The bound box used for checking if the entity can drop at a position
  // Should be wider than the full bound box
  Maybe<RectF> droppingBoundBox;
  // Pathing simulates jump arcs for two Y velocities: 1.0 * jumpSpeed and
  // smallJumpMultiplier * jumpSpeed. This value should be in the range
  // 0 < smallJumpMultiplier < 1.0
  Maybe<float> smallJumpMultiplier;
  // Mid-jump, at the peak, entities can choose to change horizontal velocity.
  // The velocities they can switch to are runSpeed, walkSpeed, and
  // (walkSpeed * jumpDropXMultiplier). The purpose of the latter option is to
  // make a vertical drop (if 0) or disable dropping (if 1). Inbetween values
  // can be used to make less angular-looking arcs.
  Maybe<float> jumpDropXMultiplier;
  // If provided, the following fields can be supplied to put a limit on how
  // long findPath calls can take:
  Maybe<double> maxFScore;
  Maybe<unsigned> maxNodesToSearch;
  // Upper bound on the (negative) velocity that entities can land on
  // platforms
  // and ledges with. This is used to ensure there is a small amount of
  // clearance
  // over ledges to improve the scripts' chances of landing the same way we
  // simulated the jump.
  Maybe<float> maxLandingVelocity;
};

bool operator==(Parameters const& lhs, Parameters const& rhs);
bool operator!=(Parameters const& lhs, Parameters const& rhs);

inline bool operator==(Node const& a, Node const& b) {
  return a.position == b.position && a.velocity == b.velocity;
}

inline std::ostream& operator<<(std::ostream& os, Node const& node) {
  return os << strf("Node{position = {}, velocity = {}}", node.position, node.velocity);
}

inline std::ostream& operator<<(std::ostream& os, Action action) {
  return os << ActionNames.getRight(action);
}

inline std::ostream& operator<<(std::ostream& os, Edge const& edge) {
  return os << strf("Edge{cost = %f, action = {}, jumpVelocity = {}, source = {}, target = {}}",
          edge.cost,
          edge.action,
          edge.jumpVelocity,
          edge.source,
          edge.target);
}

inline bool Node::operator<(Node const& other) const {
  if (position == other.position)
    return velocity < other.velocity;
  return position < other.position;
}

inline bool operator==(Parameters const& lhs, Parameters const& rhs) {
  return lhs.maxDistance == rhs.maxDistance
    && lhs.returnBest == rhs.returnBest
    && lhs.mustEndOnGround == rhs.mustEndOnGround
    && lhs.enableWalkSpeedJumps == rhs.enableWalkSpeedJumps
    && lhs.enableVerticalJumpAirControl == rhs.enableVerticalJumpAirControl
    && lhs.swimCost == rhs.swimCost
    && lhs.jumpCost == rhs.jumpCost
    && lhs.liquidJumpCost == rhs.liquidJumpCost
    && lhs.dropCost == rhs.dropCost
    && lhs.boundBox == rhs.boundBox
    && lhs.standingBoundBox == rhs.standingBoundBox
    && lhs.droppingBoundBox == rhs.droppingBoundBox
    && lhs.smallJumpMultiplier == rhs.smallJumpMultiplier
    && lhs.jumpDropXMultiplier == rhs.jumpDropXMultiplier
    && lhs.maxFScore == rhs.maxFScore
    && lhs.maxNodesToSearch == rhs.maxNodesToSearch
    && lhs.maxLandingVelocity == rhs.maxLandingVelocity;
}

inline bool operator!=(Parameters const& lhs, Parameters const& rhs) {
  return !(lhs == rhs);
}

}
}

template <> struct fmt::formatter<Star::PlatformerAStar::Node> : ostream_formatter {};
template <> struct fmt::formatter<Star::PlatformerAStar::Action> : ostream_formatter {};
template <> struct fmt::formatter<Star::PlatformerAStar::Edge> : ostream_formatter {};
