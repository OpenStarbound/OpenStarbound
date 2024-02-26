#pragma once

#include "StarList.hpp"

STAR_STRUCT(PreviewTile);

STAR_CLASS(PreviewTileTool);

namespace Star {

class PreviewTileTool {
public:
  virtual ~PreviewTileTool() {}
  virtual List<PreviewTile> previewTiles(bool shifting) const = 0;
};

}
