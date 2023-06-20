#include "StarWorld.hpp"
#include "StarScriptedEntity.hpp"

namespace Star {

bool World::isServer() const {
  return connection() == ServerConnectionId;
}

bool World::isClient() const {
  return !isServer();
}

List<EntityPtr> World::entityQuery(RectF const& boundBox, EntityFilter selector) const {
  List<EntityPtr> list;
  forEachEntity(boundBox, [&](EntityPtr const& entity) {
      if (!selector || selector(entity))
        list.append(entity);
    });
  return list;
}

List<EntityPtr> World::entityLineQuery(Vec2F const& begin, Vec2F const& end, EntityFilter selector) const {
  List<EntityPtr> list;
  forEachEntityLine(begin, end, [&](EntityPtr const& entity) {
      if (!selector || selector(entity))
        list.append(entity);
    });
  return list;
}

List<TileEntityPtr> World::entitiesAtTile(Vec2I const& pos, EntityFilter selector) const {
  List<TileEntityPtr> list;
  forEachEntityAtTile(pos, [&](TileEntityPtr entity) {
      if (!selector || selector(entity))
        list.append(move(entity));
    });
  return list;
}

List<Vec2I> World::findEmptyTiles(Vec2I pos, unsigned maxDist, size_t maxAmount, bool excludeEphemeral) const {
  List<Vec2I> res;
  if (!tileIsOccupied(pos, TileLayer::Foreground, excludeEphemeral))
    res.append(pos);

  if (res.size() >= maxAmount)
    return res;

  // searches manhattan distance counterclockwise from right
  for (int distance = 1; distance <= (int)maxDist; distance++) {
    const int totalSpots = 4 * distance;
    int xDiff = distance;
    int yDiff = 0;
    int dx = -1;
    int dy = 1;
    for (int i = 0; i < totalSpots; i++) {
      if (!tileIsOccupied(pos + Vec2I(xDiff, yDiff), TileLayer::Foreground)) {
        res.append(pos + Vec2I(xDiff, yDiff));
        if (res.size() >= maxAmount) {
          return res;
        }
      }
      xDiff += dx;
      yDiff += dy;
      if (abs(xDiff) == distance)
        dx *= -1;
      if (abs(yDiff) == distance)
        dy *= -1;
    }
  }

  return res;
}

bool World::canModifyTile(Vec2I const& pos, TileModification const& modification, bool allowEntityOverlap) const {
  return !validTileModifications({{pos, modification}}, allowEntityOverlap).empty();
}

bool World::modifyTile(Vec2I const& pos, TileModification const& modification, bool allowEntityOverlap) {
  return applyTileModifications({{pos, modification}}, allowEntityOverlap).empty();
}

TileDamageResult World::damageTile(Vec2I const& tilePosition, TileLayer layer, Vec2F const& sourcePosition, TileDamage const& tileDamage, Maybe<EntityId> sourceEntity) {
  return damageTiles({tilePosition}, layer, sourcePosition, tileDamage, sourceEntity);
}

EntityPtr World::closestEntityInSight(Vec2F const& center, float radius, CollisionSet const& collisionSet, EntityFilter selector) const {
  return closestEntity(center, radius, [=](EntityPtr const& entity) {
      return selector(entity) && !lineTileCollision(center, entity->position(), collisionSet);
    });
}

bool World::pointCollision(Vec2F const& point, CollisionSet const& collisionSet) const {
  bool collided = false;

  forEachCollisionBlock(RectI::withCenter(Vec2I(point), {3, 3}), [&](CollisionBlock const& block) {
      if (collided || !isColliding(block.kind, collisionSet))
        return;

      if (block.poly.contains(point))
        collided = true;
    });

  return collided;
}

Maybe<pair<Vec2F, Maybe<Vec2F>>> World::lineCollision(Line2F const& line, CollisionSet const& collisionSet) const {
  auto geometry = this->geometry();
  Maybe<PolyF> intersectPoly;
  Maybe<PolyF::LineIntersectResult> closestIntersection;

  forEachCollisionBlock(RectI::integral(RectF::boundBoxOf(line.min(), line.max()).padded(1)), [&](CollisionBlock const& block) {
      if (block.poly.isNull() || !isColliding(block.kind, collisionSet))
        return;

      Vec2F nearMin = geometry.nearestTo(block.poly.center(), line.min());
      auto intersection = block.poly.lineIntersection(Line2F(nearMin, nearMin + line.diff()));
      if (intersection && (!closestIntersection || intersection->along < closestIntersection->along)) {
        intersectPoly = block.poly;
        closestIntersection = intersection;
      }
    });

  if (closestIntersection) {
    auto point = line.eval(closestIntersection->along);
    auto normal = closestIntersection->intersectedSide.apply([&](uint64_t side) { return intersectPoly->normal(side); });
    return make_pair(point, normal);
  }
  return {};
}

bool World::polyCollision(PolyF const& poly, CollisionSet const& collisionSet) const {
  auto geometry = this->geometry();
  Vec2F polyCenter = poly.center();
  PolyF translatedPoly;
  bool collided = false;

  forEachCollisionBlock(RectI::integral(poly.boundBox()).padded(1), [&](CollisionBlock const& block) {
      if (collided || !isColliding(block.kind, collisionSet))
        return;

      Vec2F center = block.poly.center();
      Vec2F newCenter = geometry.nearestTo(polyCenter, center);
      translatedPoly = block.poly;
      translatedPoly.translate(newCenter - center);
      if (poly.intersects(translatedPoly))
        collided = true;
    });

  return collided;
}

}
