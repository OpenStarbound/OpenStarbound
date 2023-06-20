#include "StarLoungingEntities.hpp"
#include "StarWorld.hpp"

namespace Star {

EnumMap<LoungeOrientation> const LoungeOrientationNames{{LoungeOrientation::None, "none"},
    {LoungeOrientation::Sit, "sit"},
    {LoungeOrientation::Lay, "lay"},
    {LoungeOrientation::Stand, "stand"}};

EnumMap<LoungeControl> const LoungeControlNames{{LoungeControl::Left, "Left"},
    {LoungeControl::Right, "Right"},
    {LoungeControl::Down, "Down"},
    {LoungeControl::Up, "Up"},
    {LoungeControl::Jump, "Jump"},
    {LoungeControl::PrimaryFire, "PrimaryFire"},
    {LoungeControl::AltFire, "AltFire"},
    {LoungeControl::Special1, "Special1"},
    {LoungeControl::Special2, "Special2"},
    {LoungeControl::Special3, "Special3"}};

EntityAnchorConstPtr LoungeableEntity::anchor(size_t anchorPositionIndex) const {
  return loungeAnchor(anchorPositionIndex);
}

void LoungeableEntity::loungeControl(size_t, LoungeControl) {}

void LoungeableEntity::loungeAim(size_t, Vec2F const&) {}

Set<EntityId> LoungeableEntity::entitiesLoungingIn(size_t positionIndex) const {
  Set<EntityId> loungingInEntities;
  for (auto const& p : entitiesLounging()) {
    if (p.second == positionIndex)
      loungingInEntities.add(p.first);
  }
  return loungingInEntities;
}

Set<pair<EntityId, size_t>> LoungeableEntity::entitiesLounging() const {
  Set<pair<EntityId, size_t>> loungingInEntities;
  world()->forEachEntity(metaBoundBox().translated(position()),
      [&](EntityPtr const& entity) {
        if (auto lounger = as<LoungingEntity>(entity)) {
          if (auto anchorStatus = lounger->loungingIn()) {
            if (anchorStatus->entityId == entityId())
              loungingInEntities.add({entity->entityId(), anchorStatus->positionIndex});
          }
        }
      });
  return loungingInEntities;
}

bool LoungingEntity::inConflictingLoungeAnchor() const {
  if (auto loungeAnchorState = loungingIn()) {
    if (auto loungeableEntity = world()->get<LoungeableEntity>(loungeAnchorState->entityId)) {
      auto entitiesLoungingIn = loungeableEntity->entitiesLoungingIn(loungeAnchorState->positionIndex);
      return entitiesLoungingIn.size() > 1 || !entitiesLoungingIn.contains(entityId());
    }
  }
  return false;
}

}
