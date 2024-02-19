#ifndef STAR_ENTITY_MAP_HPP
#define STAR_ENTITY_MAP_HPP

#include "StarSpatialHash2D.hpp"
#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(EntityMap);
STAR_CLASS(TileEntity);
STAR_CLASS(InteractiveEntity);

STAR_EXCEPTION(EntityMapException, StarException);

// Class used by WorldServer and WorldClient to store entites organized in a
// spatial hash.  Provides convenient ways of querying entities based on
// different selection criteria.
//
// Several of the methods in EntityMap take callbacks or filters that will be
// called while iterating over internal structures.  They are all designed so
// that adding new entities is safe to do from the callback, but removing
// entities is never safe to do from any callback function.
class EntityMap {
public:
  static float const SpatialHashSectorSize;
  static int const MaximumEntityBoundBox;

  // beginIdSpace and endIdSpace is the *inclusive* range for new enittyIds.
  EntityMap(Vec2U const& worldSize, EntityId beginIdSpace, EntityId endIdSpace);

  // Get the next free id in the entity id space.
  EntityId reserveEntityId();
  // Or a specific one, can fail.
  Maybe<EntityId> maybeReserveEntityId(EntityId entityId);
  // If it doesn't matter that we don't get the one want
  EntityId reserveEntityId(EntityId entityId);

  // Add an entity to this EntityMap.  The entity must already be initialized
  // and have a unique EntityId returned by reserveEntityId.
  void addEntity(EntityPtr entity);
  EntityPtr removeEntity(EntityId entityId);

  size_t size() const;
  List<EntityId> entityIds() const;

  // Iterates through the entity map optionally in the given order, updating
  // the spatial information for each entity along the way.
  void updateAllEntities(EntityCallback const& callback = {}, function<bool(EntityPtr const&, EntityPtr const&)> sortOrder = {});

  // If the given unique entity is in this map, then return its entity id
  EntityId uniqueEntityId(String const& uniqueId) const;

  EntityPtr entity(EntityId entityId) const;
  EntityPtr uniqueEntity(String const& uniqueId) const;

  // Queries entities based on metaBoundBox
  List<EntityPtr> entityQuery(RectF const& boundBox, EntityFilter const& filter = {}) const;

  // A fuzzy query of the entities at this position, sorted by closeness.
  List<EntityPtr> entitiesAt(Vec2F const& pos, EntityFilter const& filter = {}) const;

  List<TileEntityPtr> entitiesAtTile(Vec2I const& pos, EntityFilterOf<TileEntity> const& filter = {}) const;

  // Sort of a fuzzy line intersection test.  Tests if a given line intersects
  // the bounding box of any entities, and returns them.
  List<EntityPtr> entityLineQuery(Vec2F const& begin, Vec2F const& end, EntityFilter const& filter = {}) const;

  // Callback versions of query functions.
  void forEachEntity(RectF const& boundBox, EntityCallback const& callback) const;
  void forEachEntityLine(Vec2F const& begin, Vec2F const& end, EntityCallback const& callback) const;
  // Returns tile-based entities that occupy the given tile position.
  void forEachEntityAtTile(Vec2I const& pos, EntityCallbackOf<TileEntity> const& callback) const;

  // Iterate through all the entities, optionally in the given sort order.
  void forAllEntities(EntityCallback const& callback, function<bool(EntityPtr const&, EntityPtr const&)> sortOrder = {}) const;

  // Stops searching when filter returns true, and returns the entity which
  // caused it.
  EntityPtr findEntity(RectF const& boundBox, EntityFilter const& filter) const;
  EntityPtr findEntityLine(Vec2F const& begin, Vec2F const& end, EntityFilter const& filter) const;
  EntityPtr findEntityAtTile(Vec2I const& pos, EntityFilterOf<TileEntity> const& filter) const;

  // Closest entity that satisfies the given selector, if given.
  EntityPtr closestEntity(Vec2F const& center, float radius, EntityFilter const& filter = {}) const;

  // Returns interactive entity that is near the given world position
  InteractiveEntityPtr interactiveEntityNear(Vec2F const& pos, float maxRadius = 1.5f) const;

  // Whether or not any tile entity occupies this tile
  bool tileIsOccupied(Vec2I const& pos, bool includeEphemeral = false) const;

  // Intersects any entity's collision area
  bool spaceIsOccupied(RectF const& rect, bool includeEphemeral = false) const;

  // Convenience template methods that filter based on dynamic cast and deal
  // with pointers to a derived entity type.

  template <typename EntityT>
  shared_ptr<EntityT> get(EntityId entityId) const;

  template <typename EntityT>
  shared_ptr<EntityT> getUnique(String const& uniqueId) const;

  template <typename EntityT>
  List<shared_ptr<EntityT>> query(RectF const& boundBox, EntityFilterOf<EntityT> const& filter = {}) const;

  template <typename EntityT>
  List<shared_ptr<EntityT>> all(EntityFilterOf<EntityT> const& filter = {}) const;

  template <typename EntityT>
  List<shared_ptr<EntityT>> lineQuery(Vec2F const& begin, Vec2F const& end, EntityFilterOf<EntityT> const& filter = {}) const;

  template <typename EntityT>
  shared_ptr<EntityT> closest(Vec2F const& center, float radius, EntityFilterOf<EntityT> const& filter = {}) const;

  template <typename EntityT>
  List<shared_ptr<EntityT>> atTile(Vec2I const& pos) const;

private:
  typedef SpatialHash2D<EntityId, float, EntityPtr> SpatialMap;

  WorldGeometry m_geometry;

  SpatialMap m_spatialMap;
  BiHashMap<String, EntityId> m_uniqueMap;

  EntityId m_nextId;
  EntityId m_beginIdSpace;
  EntityId m_endIdSpace;

  List<SpatialMap::Entry const*> m_entrySortBuffer;
};

template <typename EntityT>
shared_ptr<EntityT> EntityMap::get(EntityId entityId) const {
  return as<EntityT>(entity(entityId));
}

template <typename EntityT>
shared_ptr<EntityT> EntityMap::getUnique(String const& uniqueId) const {
  return as<EntityT>(uniqueEntity(uniqueId));
}

template <typename EntityT>
List<shared_ptr<EntityT>> EntityMap::query(RectF const& boundBox, EntityFilterOf<EntityT> const& filter) const {
  List<shared_ptr<EntityT>> entities;
  for (auto const& entity : entityQuery(boundBox, entityTypeFilter(filter)))
    entities.append(as<EntityT>(entity));

  return entities;
}

template <typename EntityT>
List<shared_ptr<EntityT>> EntityMap::all(EntityFilterOf<EntityT> const& filter) const {
  List<shared_ptr<EntityT>> entities;
  forAllEntities([&](EntityPtr const& entity) {
    if (auto e = as<EntityT>(entity)) {
      if (!filter || filter(e))
        entities.append(e);
    }
  });

  return entities;
}

template <typename EntityT>
List<shared_ptr<EntityT>> EntityMap::lineQuery(Vec2F const& begin, Vec2F const& end, EntityFilterOf<EntityT> const& filter) const {
  List<shared_ptr<EntityT>> entities;
  for (auto const& entity : entityLineQuery(begin, end, entityTypeFilter(filter)))
    entities.append(as<EntityT>(entity));

  return entities;
}

template <typename EntityT>
shared_ptr<EntityT> EntityMap::closest(Vec2F const& center, float radius, EntityFilterOf<EntityT> const& filter) const {
  return as<EntityT>(closestEntity(center, radius, entityTypeFilter(filter)));
}

template <typename EntityT>
List<shared_ptr<EntityT>> EntityMap::atTile(Vec2I const& pos) const {
  List<shared_ptr<EntityT>> list;
  forEachEntityAtTile(pos, [&](TileEntityPtr const& entity) {
      if (auto e = as<EntityT>(entity))
        list.append(std::move(e));
      return false;
    });
  return list;
}

}

#endif
