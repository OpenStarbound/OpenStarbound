#pragma once

#include "StarBiMap.hpp"
#include "StarAStar.hpp"
#include "StarWorld.hpp"
#include "StarActorMovementController.hpp"
#include "StarPlatformerAStarTypes.hpp"

namespace Star {
namespace PlatformerAStar {

  STAR_CLASS(PathFinder);

  class PathFinder {
  public:
    PathFinder(World* world,
        Vec2F searchFrom,
        Vec2F searchTo,
        ActorMovementParameters movementParameters,
        Parameters searchParameters = Parameters());

    // Does not preserve current search state.
    PathFinder(PathFinder const& rhs);
    PathFinder& operator=(PathFinder const& rhs);

    Maybe<bool> explore(Maybe<unsigned> maxExploreNodes = {});
    Maybe<Path> const& result() const;

  private:
    enum class BoundBoxKind { Full, Drop, Stand };

    void initAStar();

    float heuristicCost(Vec2F const& fromPosition, Vec2F const& toPosition) const;
    Edge defaultCostEdge(Action action, Node const& source, Node const& target) const;
    void neighbors(Node const& node, List<Edge>& neighbors) const;

    void getDropNeighbors(Node const& node, List<Edge>& neighbors) const; // drop through a platform
    void getWalkingNeighborsInDirection(Node const& node, List<Edge>& neighbors, float direction) const;
    void getWalkingNeighbors(Node const& node, List<Edge>& neighbors) const;
    void getFallingNeighbors(Node const& node, List<Edge>& neighbors) const; // freefall
    void getJumpingNeighbors(Node const& node, List<Edge>& neighbors) const;
    void getArcNeighbors(Node const& node, List<Edge>& neighbors) const;
    void getSwimmingNeighbors(Node const& node, List<Edge>& neighbors) const;
    void getFlyingNeighbors(Node const& node, List<Edge>& neighbors) const;

    void forEachArcVelocity(float yVelocity, function<void(Vec2F)> func) const;
    void forEachArcNeighbor(Node const& node, float yVelocity, function<void(Node, bool)> func) const;
    Vec2F acceleration(Vec2F pos) const;
    Vec2F simulateArcCollision(Vec2F position, Vec2F velocity, float dt, bool& collidedX, bool& collidedY) const;
    void simulateArc(Node const& node, function<void(Node, bool)> func) const;

    bool validPosition(Vec2F pos, BoundBoxKind boundKind = BoundBoxKind::Full) const;
    bool onGround(Vec2F pos, BoundBoxKind boundKind = BoundBoxKind::Full) const; // Includes non-solids: platforms, objects, etc.
    bool onSolidGround(Vec2F pos) const; // Includes only solids
    bool inLiquid(Vec2F pos) const;

    RectF boundBox(Vec2F pos, BoundBoxKind boundKind = BoundBoxKind::Full) const;
    // Returns a rect that covers the tiles below the entity's feet if it was at
    // pos
    RectI groundCollisionRect(Vec2F pos, BoundBoxKind boundKind) const;
    // Returns a rect that covers a 1 tile wide space below the entity's feet at
    // node pos
    Vec2I groundNodePosition(Vec2F pos) const;

    Vec2F roundToNode(Vec2F pos) const;
    float distance(Vec2F a, Vec2F b) const;

    World* m_world;
    Vec2F m_searchFrom;
    Vec2F m_searchTo;
    ActorMovementParameters m_movementParams;
    Parameters m_searchParams;
    Maybe<AStar::Search<Edge, Node>> m_astar;
  };
}
}
