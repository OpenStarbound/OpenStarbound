#pragma once

#include "StarGameTypes.hpp"
#include "StarWorldGeometry.hpp"
#include "StarDataStream.hpp"

namespace Star {

STAR_CLASS(WireCoordinator);
STAR_CLASS(WireConnector);

enum class WireDirection {
  Input,
  Output
};

WireDirection otherWireDirection(WireDirection direction);

// Identifier for a specific WireNode in a WireEntity, node indexes for input
// and output nodes are separate.
struct WireNode {
  WireDirection direction;
  size_t nodeIndex;
};

DataStream& operator>>(DataStream& ds, WireNode& wireNode);
DataStream& operator<<(DataStream& ds, WireNode const& wireNode);

// Connection from a given WireNode to another WireNode, the direction must be
// implied based on the context.
struct WireConnection {
  Vec2I entityLocation;
  size_t nodeIndex;

  bool operator==(WireConnection const& wireConnection) const;
};

template <>
struct hash<WireConnection> {
  size_t operator()(WireConnection const& wireConnection) const;
};

DataStream& operator>>(DataStream& ds, WireConnection& wireConnection);
DataStream& operator<<(DataStream& ds, WireConnection const& wireConnection);

class WireCoordinator {
public:
  virtual ~WireCoordinator() {}

  virtual bool readInputConnection(WireConnection const& connection) = 0;
};

class WireConnector {
public:
  enum SwingResult {
    Connect,
    Mismatch,
    Protected,
    Nothing
  };

  virtual ~WireConnector() {}

  virtual SwingResult swing(WorldGeometry const& geometry, Vec2F position, FireMode mode) = 0;
  virtual bool connecting() = 0;
};

}
