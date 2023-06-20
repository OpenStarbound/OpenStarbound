#include "StarWiring.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

WireDirection otherWireDirection(WireDirection direction) {
  return direction == WireDirection::Input ? WireDirection::Output : WireDirection::Input;
}

bool WireConnection::operator==(WireConnection const& wireConnection) const {
  return tie(entityLocation, nodeIndex) == tie(wireConnection.entityLocation, wireConnection.nodeIndex);
}

size_t hash<WireConnection>::operator()(WireConnection const& wireConnection) const {
  return hashOf(wireConnection.entityLocation, wireConnection.nodeIndex);
}

DataStream& operator>>(DataStream& ds, WireConnection& wireConnection) {
  ds.viread(wireConnection.entityLocation[0]);
  ds.viread(wireConnection.entityLocation[1]);
  ds.vuread(wireConnection.nodeIndex);
  return ds;
}

DataStream& operator<<(DataStream& ds, WireConnection const& wireConnection) {
  ds.viwrite(wireConnection.entityLocation[0]);
  ds.viwrite(wireConnection.entityLocation[1]);
  ds.vuwrite(wireConnection.nodeIndex);
  return ds;
}

DataStream& operator>>(DataStream& ds, WireNode& wireNode) {
  ds.read(wireNode.direction);
  ds.vuread(wireNode.nodeIndex);
  return ds;
}

DataStream& operator<<(DataStream& ds, WireNode const& wireNode) {
  ds.write(wireNode.direction);
  ds.vuwrite(wireNode.nodeIndex);
  return ds;
}

}
