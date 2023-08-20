#ifndef STAR_PREVIEW_TILE_TOOL_HPP
#define STAR_PREVIEW_TILE_TOOL_HPP

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

#endif
