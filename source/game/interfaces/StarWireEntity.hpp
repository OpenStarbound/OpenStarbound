#pragma once

#include "StarWiring.hpp"
#include "StarTileEntity.hpp"

namespace Star {

STAR_CLASS(WireEntity);

class WireEntity : public virtual TileEntity {
public:
  virtual ~WireEntity() {}

  virtual size_t nodeCount(WireDirection direction) const = 0;
  virtual Vec2I nodePosition(WireNode wireNode) const = 0;
  virtual List<WireConnection> connectionsForNode(WireNode wireNode) const = 0;
  virtual bool nodeState(WireNode wireNode) const = 0;

  virtual Color nodeColor(WireNode wireNode) const = 0;
  virtual String nodeIcon(WireNode wireNode) const = 0;

  virtual void addNodeConnection(WireNode wireNode, WireConnection nodeConnection) = 0;
  virtual void removeNodeConnection(WireNode wireNode, WireConnection nodeConnection) = 0;

  virtual void evaluate(WireCoordinator* coordinator) = 0;
};

}
