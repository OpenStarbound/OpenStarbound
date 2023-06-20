#include "StarAnchorableEntity.hpp"

namespace Star {

bool EntityAnchorState::operator==(EntityAnchorState const& eas) const {
  return tie(entityId, positionIndex) == tie(eas.entityId, eas.positionIndex);
}

DataStream& operator>>(DataStream& ds, EntityAnchorState& anchorState) {
  ds.read(anchorState.entityId);
  ds.readVlqS(anchorState.positionIndex);
  return ds;
}

DataStream& operator<<(DataStream& ds, EntityAnchorState const& anchorState) {
  ds.write(anchorState.entityId);
  ds.writeVlqS(anchorState.positionIndex);
  return ds;
}

}
