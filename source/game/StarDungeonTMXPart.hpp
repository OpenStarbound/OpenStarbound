#ifndef STAR_DUNGEON_TMX_PART_HPP
#define STAR_DUNGEON_TMX_PART_HPP

#include "StarRect.hpp"
#include "StarDungeonGenerator.hpp"
#include "StarTilesetDatabase.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

namespace Dungeon {
  STAR_CLASS(TMXTilesets);
  STAR_CLASS(TMXTileLayer);
  STAR_CLASS(TMXObject);
  STAR_CLASS(TMXObjectGroup);
  STAR_CLASS(TMXMap);

  class TMXTilesets {
  public:
    TMXTilesets(Json const& tmx);

    Tiled::Tile const& getTile(unsigned gid, TileLayer layer) const;

    Tiled::Tile const& nullTile() const {
      return *m_nullTile;
    }

  private:
    struct TilesetInfo {
      Tiled::Tileset const* tileset;
      size_t firstGid;
      size_t lastGid;
    };

    static bool tilesetComparator(TilesetInfo const& a, TilesetInfo const& b);

    // The default empty background tile has clear=true.  (If you use the pink
    // tile in the background, clear will be false instead.) Analogous to
    // EmptyMaterialId.
    Tiled::TileConstPtr m_emptyBackTile;
    // The default foreground tile doesn't have a 'clear' property.  Also
    // returned by tile layers when given coordinates outside the bounds of the
    // layer.  Analogous to the NullMaterialId that mission maps are initially
    // filled with.
    Tiled::TileConstPtr m_nullTile;

    List<Tiled::TilesetConstPtr> m_tilesets;
    List<Tiled::Tile const*> m_foregroundTilesByGid;
    List<Tiled::Tile const*> m_backgroundTilesByGid;
  };

  class TMXTileLayer {
  public:
    TMXTileLayer(Json const& tmx);

    Tiled::Tile const& getTile(TMXTilesetsPtr const& tilesets, Vec2I pos) const;

    unsigned width() const {
      return m_rect.xMax() - m_rect.xMin() + 1;
    }
    unsigned height() const {
      return m_rect.yMax() - m_rect.yMin() + 1;
    }

    RectI const& rect() const {
      return m_rect;
    }
    String const& name() const {
      return m_name;
    }

    TileLayer layer() const {
      return m_layer;
    }

    bool forEachTile(TMXMap const* map, TileCallback const& callback) const;
    bool forEachTileAt(Vec2I pos, TMXMap const* map, TileCallback const& callback) const;

  private:
    RectI m_rect;
    String m_name;
    TileLayer m_layer;
    List<unsigned> m_tileData;
  };

  enum class ObjectKind {
    Tile,
    Rectangle,
    Ellipse,
    Polygon,
    Polyline,
    Stagehand
  };

  enum TileFlip {
    Horizontal = 0x80000000u,
    Vertical = 0x40000000u,
    Diagonal = 0x20000000u,
    AllBits = 0xe0000000u
  };

  class TMXObject {
  public:
    TMXObject(Maybe<Json> const& groupProperties, Json const& tmx, TMXTilesetsPtr tilesets);

    Vec2I const& pos() const {
      return m_rect.min();
    }
    RectI const& rect() const {
      return m_rect;
    }
    Tiled::Tile const& tile() const {
      return *m_tile;
    }
    ObjectKind kind() const {
      return m_kind;
    }

    bool forEachTile(TMXMap const* map, TileCallback const& callback) const;
    bool forEachTileAt(Vec2I pos, TMXMap const* map, TileCallback const& callback) const;

  private:
    // "Tile Objects" in Tiled are objects that contain an image from a tileset,
    // and have a bunch of their own Tile Object-specific properties.
    struct TileObjectInfo {
      Tiled::Properties tileProperties;
      unsigned flipBits;
    };

    static Vec2I getSize(Json const& tmx);
    static Vec2I getImagePosition(Tiled::Properties const& properties);
    static ObjectKind getObjectKind(Json const& tmx, Maybe<Json >const& objectProperties);
    static Maybe<TileObjectInfo> getTileObjectInfo(Json const& tmx, TMXTilesetsPtr tilesets, TileLayer layer);
    static TileLayer getLayer(Maybe<Json> const& groupProperties, Maybe<Json> const& objectProperties);

    static Vec2I getPos(Json const& tmx);
    static StarException tmxObjectError(Json const& tmx, String const& msg);

    RectI m_rect;
    Tiled::TileConstPtr m_tile;
    TileLayer m_layer;
    ObjectKind m_kind;
    unsigned m_objectId;
    List<Vec2I> m_polyline;
  };

  class TMXObjectGroup {
  public:
    TMXObjectGroup(Json const& tmx, TMXTilesetsPtr tilesets);

    List<TMXObjectPtr> const& objects() const {
      return m_objects;
    }

    String name() const;

    bool forEachTile(TMXMap const* map, TileCallback const& callback) const;
    bool forEachTileAt(Vec2I pos, TMXMap const* map, TileCallback const& callback) const;

  private:
    String m_name;
    List<TMXObjectPtr> m_objects;
  };

  class TMXMap {
  public:
    TMXMap(Json const& tmx);

    List<TMXTileLayerPtr> const& tileLayers() const {
      return m_tileLayers;
    }
    List<TMXObjectGroupPtr> const& objectGroups() const {
      return m_objectGroups;
    }
    TMXTilesetsPtr const& tilesets() const {
      return m_tilesets;
    }
    unsigned width() const {
      return m_width;
    }
    unsigned height() const {
      return m_height;
    }

    bool forEachTile(TileCallback const& callback) const;
    bool forEachTileAt(Vec2I pos, TileCallback const& callback) const;

  private:
    List<TMXTileLayerPtr> m_tileLayers;
    List<TMXObjectGroupPtr> m_objectGroups;

    TMXTilesetsPtr m_tilesets;
    unsigned m_width, m_height;
  };

  class TMXPartReader : public PartReader {
  public:
    virtual void readAsset(String const& asset) override;

    virtual Vec2U size() const override;

    virtual void forEachTile(TileCallback const& callback) const override;
    virtual void forEachTileAt(Vec2I pos, TileCallback const& callback) const override;

  private:
    // Return true in the callback to exit early without processing later maps
    void forEachMap(function<bool(TMXMapConstPtr const&)> func) const;

    List<pair<String, TMXMapConstPtr>> m_maps;
  };
}

}

#endif
