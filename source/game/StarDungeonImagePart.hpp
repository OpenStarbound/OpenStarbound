#pragma once

#include "StarDungeonGenerator.hpp"

namespace Star {

namespace Dungeon {

  STAR_CLASS(ImagePartReader);
  STAR_CLASS(ImageTileset);

  class ImagePartReader : public PartReader {
  public:
    ImagePartReader(ImageTilesetConstPtr tileset) : m_tileset(tileset) {}

    virtual void readAsset(String const& asset) override;
    virtual Vec2U size() const override;

    virtual void forEachTile(TileCallback const& callback) const override;
    virtual void forEachTileAt(Vec2I pos, TileCallback const& callback) const override;

  private:
    List<pair<String, ImageConstPtr>> m_images;
    ImageTilesetConstPtr m_tileset;
  };

  class ImageTileset {
  public:
    ImageTileset(Json const& tileset);

    Tile const* getTile(Vec4B color) const;

  private:
    unsigned colorAsInt(Vec4B color) const;

    Map<unsigned, Tile> m_tiles;
  };
}
}
