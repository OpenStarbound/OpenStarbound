#include "StarCompression.hpp"
#include "StarEncode.hpp"
#include "StarRoot.hpp"
#include "StarGameTypes.hpp"
#include "StarAssets.hpp"
#include "StarCasting.hpp"
#include "StarLexicalCast.hpp"
#include "StarRect.hpp"
#include "StarObjectDatabase.hpp"
#include "StarDungeonTMXPart.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

namespace Dungeon {

  bool TMXMap::forEachTile(TileCallback const& callback) const {
    for (auto const& layer : tileLayers()) {
      if (layer->forEachTile(this, callback))
        return true;
    }

    for (auto const& group : objectGroups()) {
      if (group->forEachTile(this, callback))
        return true;
    }

    return false;
  }

  bool TMXTileLayer::forEachTile(TMXMap const* map, TileCallback const& callback) const {
    auto const& tilesets = map->tilesets();
    unsigned height = map->height();

    for (int y = m_rect.yMin(); y <= m_rect.yMax(); ++y) {
      for (int x = m_rect.xMin(); x <= m_rect.xMax(); ++x) {
        if (callback(Vec2I(x, height - 1 - y), getTile(tilesets, Vec2I(x, y))))
          return true;
      }
    }

    return false;
  }

  String TMXObjectGroup::name() const {
    return m_name;
  }

  bool TMXObjectGroup::forEachTile(TMXMap const* map, TileCallback const& callback) const {
    for (auto const& object : objects()) {
      if (object->forEachTile(map, callback))
        return true;
    }

    return false;
  }

  bool TMXObject::forEachTile(TMXMap const* map, TileCallback const& callback) const {
    if (m_kind == ObjectKind::Stagehand) {
      auto cPos = Vec2I(rect().center()[0], ceilf(RectF(rect()).center()[1]));
      return callback(Vec2I(cPos[0], map->height() - cPos[1]), tile());
    }

    if (m_kind == ObjectKind::Tile) {
      // Used for placing Starbound-Tiles and Starbound-Objects
      Vec2I position(pos().x(), map->height() - pos().y());
      return callback(position, tile());
    }

    if (m_kind == ObjectKind::Rectangle) {
      // Used for creating custom brushes and rules
      for (int x = m_rect.min().x(); x < m_rect.max().x(); ++x) {
        for (int y = m_rect.min().y(); y < m_rect.max().y(); ++y) {
          Vec2I position(x, map->height() - 1 - y);
          if (callback(position, tile()))
            return true;
        }
      }
      return false;
    }

    starAssert(m_kind == ObjectKind::Polyline);
    // Used for wiring. Treat each vertex in the polyline as a tile with the
    // wire brush.
    for (Vec2I point : m_polyline) {
      Vec2I position(m_rect.min().x() + point.x(), map->height() - 1 - m_rect.min().y() - point.y());
      if (callback(position, tile()))
        return true;
    }
    return false;
  }

  bool TMXMap::forEachTileAt(Vec2I pos, TileCallback const& callback) const {
    for (auto const& layer : tileLayers()) {
      if (layer->forEachTileAt(pos, this, callback))
        return true;
    }

    for (auto const& group : objectGroups()) {
      if (group->forEachTileAt(pos, this, callback))
        return true;
    }

    return false;
  }

  bool TMXTileLayer::forEachTileAt(Vec2I pos, TMXMap const* map, TileCallback const& callback) const {
    Vec2I tilePos(pos.x(), map->height() - 1 - pos.y());
    if (!m_rect.contains(tilePos))
      return false;

    auto const& tile = getTile(map->tilesets(), tilePos);
    if (callback(pos, tile))
      return true;

    return false;
  }

  bool TMXObjectGroup::forEachTileAt(Vec2I pos, TMXMap const* map, TileCallback const& callback) const {
    for (auto const& object : objects()) {
      if (object->forEachTileAt(pos, map, callback))
        return true;
    }

    return false;
  }

  bool TMXObject::forEachTileAt(Vec2I pos, TMXMap const* map, TileCallback const& callback) const {
    if (m_kind == ObjectKind::Stagehand) {
      Vec2I cPos = Vec2I(rect().center()[0], ceilf(RectF(rect()).center()[1]));
      if (pos == cPos)
        return callback(Vec2I(pos[0], map->height() - 1 - pos[1]), tile());
      return false;
    }

    if (m_kind == ObjectKind::Tile) {
      Vec2I vertexPos(pos.x(), map->height() - pos.y());
      if (vertexPos != m_rect.min())
        return false;
      return callback(pos, tile());
    }

    if (m_kind == ObjectKind::Rectangle) {
      if (!m_rect.contains(Vec2I(pos.x(), map->height() - 1 - pos.y())))
        return false;
      return callback(pos, tile());
    }

    starAssert(m_kind == ObjectKind::Polyline);
    for (Vec2I point : m_polyline) {
      Vec2I pointPos(m_rect.min().x() + point.x(), map->height() - 1 - m_rect.min().y() - point.y());
      if (pos == pointPos && callback(pos, tile()))
        return true;
    }
    return false;
  }

  TMXTileLayer::TMXTileLayer(Json const& layer) {
    unsigned width = layer.getInt("width"), height = layer.getInt("height");
    int x = layer.getInt("x", 0), y = layer.getInt("y", 0);
    m_rect = RectI({x, y}, {x + (int)width - 1, y + (int)height - 1});

    m_name = layer.getString("name");
    m_layer = Tiled::LayerNames.getLeft(m_name);

    if (layer.optString("compression") == String("zlib")) {
      ByteArray compressedData = base64Decode(layer.getString("data"));
      ByteArray bytes = uncompressData(compressedData);
      for (size_t i = 0; i + 3 < bytes.size(); i += 4) {
        uint32_t gid = (uint8_t)bytes[i] | ((uint8_t)bytes[i + 1] << 8) | ((uint8_t)bytes[i + 2] << 16) | ((uint8_t)bytes[i + 3] << 24);
        m_tileData.append(gid & ~TileFlip::AllBits);
      }
    } else if (!layer.contains("compression")) {
      for (Json const& index : layer.getArray("data")) {
        // Ignore flipped tiles. Tiled can flip selected regions with X, but
        // this
        // also flips individual tiles (setting the high bits on the GID).
        // Starbound has no support for flipped tiles, but being able to flip
        // regions is still useful.
        m_tileData.append(index.toUInt() & ~TileFlip::AllBits);
      }
    } else {
      throw StarException::format("TMXTileLayer does not support compression mode %s", layer.getString("compression"));
    }

    if (m_tileData.count() != width * height)
      throw StarException("TMXTileLayer data length was inconsistent with width/height");
  }

  TMXMap::TMXMap(Json const& tmx) {
    if (tmx.getUInt("tileheight") != 8 || tmx.getUInt("tilewidth") != 8)
      throw StarException("Invalid tile size");

    m_width = tmx.getUInt("width");
    m_height = tmx.getUInt("height");

    m_tilesets = make_shared<TMXTilesets>(tmx.getArray("tilesets"));

    for (Json const& tmxLayer : tmx.get("layers").iterateArray()) {
      String layerType = tmxLayer.getString("type");

      if (layerType == "tilelayer") {
        TMXTileLayerPtr layer = make_shared<TMXTileLayer>(tmxLayer);
        m_tileLayers.append(layer);

      } else if (layerType == "objectgroup") {
        TMXObjectGroupPtr group = make_shared<TMXObjectGroup>(tmxLayer, m_tilesets);
        m_objectGroups.append(group);

      } else {
        throw StarException(strf("Unknown layer type '%s'", layerType.utf8Ptr()));
      }
    }
  }

  Tiled::Tile const& TMXTileLayer::getTile(TMXTilesetsPtr const& tilesets, Vec2I pos) const {
    if (!m_rect.contains(pos))
      return tilesets->nullTile();

    int dx = pos.x() - m_rect.xMin();
    int dy = pos.y() - m_rect.yMin();
    unsigned tileIndex = dx + dy * width();

    return tilesets->getTile(m_tileData[tileIndex], m_layer);
  }

  String tilesetAssetPath(String const& relativePath) {
    // Tiled stores tileset paths relative to the map file, which can go below
    // the assets root if it's referencing a tileset in another asset package.
    // The solution chosen here is to ignore everything in the path up until a
    // known path segment, e.g.:
    //  "source" : "..\/..\/..\/..\/packed\/tilesets\/packed\/materials.json"
    // We ignore everything up until the 'tilesets' path segment, and the asset
    // we actually load is located at:
    //  /tilesets/packed/materials.json

    size_t i = relativePath.findLast("/tilesets/", String::CaseInsensitive);
    if (i == NPos)
      // Couldn't extract the right path, try the one we were given in the Json.
      return relativePath;
    return relativePath.slice(i);
  }

  TMXTilesets::TMXTilesets(Json const& tmx) {
    for (Json const& tilesetJson : tmx.iterateArray()) {
      if (!tilesetJson.contains("source"))
        throw StarException::format("Tiled map has embedded tileset %s", tilesetJson.optString("name"));

      String sourcePath = tilesetAssetPath(tilesetJson.getString("source"));
      Tiled::TilesetConstPtr tileset = Root::singleton().tilesetDatabase()->get(sourcePath);
      m_tilesets.append(tileset);

      size_t firstGid = tilesetJson.getUInt("firstgid");
      for (size_t i = 0; i < tileset->size(); ++i) {
        m_foregroundTilesByGid.set(firstGid + i, tileset->getTile(i, TileLayer::Foreground).get());
        m_backgroundTilesByGid.set(firstGid + i, tileset->getTile(i, TileLayer::Background).get());
      }
    }

    m_nullTile = make_shared<Tiled::Tile>(Tiled::Properties(), TileLayer::Foreground);

    JsonObject emptyBackProperties;
    emptyBackProperties["clear"] = "true";
    m_emptyBackTile = make_shared<Tiled::Tile>(Tiled::Properties(emptyBackProperties), TileLayer::Background);
  }

  Tiled::Tile const& TMXTilesets::getTile(unsigned gid, TileLayer layer) const {
    Tiled::Tile const* tilePtr;
    if (layer == TileLayer::Foreground)
      tilePtr = m_foregroundTilesByGid[gid];
    else
      tilePtr = m_backgroundTilesByGid[gid];

    if (tilePtr)
      return *tilePtr;
    if (layer == TileLayer::Foreground)
      return *m_nullTile;
    return *m_emptyBackTile;
  }

  bool TMXTilesets::tilesetComparator(TilesetInfo const& a, TilesetInfo const& b) {
    return a.firstGid > b.firstGid;
  }

  TMXObject::TMXObject(Maybe<Json> const& groupProperties, Json const& tmx, TMXTilesetsPtr tilesets) {
    m_objectId = tmx.getUInt("id");

    // convert object properties in array format to object format
    Maybe<Json> objectProperties = tmx.opt("properties").apply([](Json const& properties) -> Json {
        if (properties.type() == Json::Type::Array) {
          JsonObject objectProperties;
          for (auto& p : properties.toArray())
            objectProperties.set(p.getString("name"), p.get("value"));
          return objectProperties;
        } else {
          return properties.toObject();
        }
      });

    m_layer = getLayer(groupProperties, objectProperties);

    Maybe<TileObjectInfo> tileObjectInfo = getTileObjectInfo(tmx, tilesets, m_layer);

    // Merge properties in this order:
    //   Object
    //   Tile (and tileset by proxy)
    //   ObjectGroup
    Tiled::Properties properties;
    if (objectProperties)
      properties = properties.inherit(*objectProperties);
    if (tileObjectInfo.isValid())
      properties = properties.inherit(tileObjectInfo->tileProperties);
    if (groupProperties.isValid())
      properties = properties.inherit(*groupProperties);

    // Check whether the object was flipped horizontally before creating this
    // object's Tile
    bool flipX = tileObjectInfo.isValid() && (tileObjectInfo->flipBits & TileFlip::Horizontal) != 0;

    m_kind = getObjectKind(tmx, objectProperties);

    Vec2I pos = getPos(tmx) - getImagePosition(properties);
    Vec2I size = getSize(tmx);
    m_rect = RectI(pos, Vec2I(pos.x() + size.x(), pos.y() + size.y()));

    JsonObject computedProperties;
    if (m_kind == ObjectKind::Stagehand) {
      Vec2I cPos = rect().center();
      RectI broadcastArea(rect().min() - cPos, rect().max() - cPos);
      computedProperties["broadcastArea"] = jsonFromRectI(broadcastArea).repr();
    }

    Maybe<float> rotation = tmx.optFloat("rotation");
    if (rotation.isValid() && *rotation != 0.0f)
      throw tmxObjectError(tmx, "object is rotated, which is not supported");

    Maybe<JsonArray> polyline = tmx.optArray("polyline");
    if (polyline.isValid()) {
      for (Json const& point : *polyline)
        m_polyline.append(getPos(point));
      computedProperties["wire"] = "_polylineWire" + toString(m_objectId);
      computedProperties["local"] = "true";
    }

    properties = properties.inherit(computedProperties);
    m_tile = make_shared<Tiled::Tile>(properties, m_layer, flipX);
  }

  Vec2I TMXObject::getSize(Json const& tmx) {
    Vec2I size;
    if (tmx.contains("width") && tmx.contains("height"))
      size = Vec2I(tmx.getUInt("width"), tmx.getUInt("height")) / TilePixels;
    return size;
  }

  Vec2I TMXObject::getImagePosition(Tiled::Properties const& properties) {
    int x = (int)(properties.opt<float>("imagePositionX").value(0) / TilePixels);
    int y = (int)(properties.opt<float>("imagePositionY").value(0) / TilePixels);
    return Vec2I(x, -y);
  }

  ObjectKind TMXObject::getObjectKind(Json const& tmx, Maybe<Json> const& objectProperties) {
    if (objectProperties && objectProperties->contains("stagehand"))
      return ObjectKind::Stagehand;
    else if (tmx.contains("gid"))
      // Tile / object
      return ObjectKind::Tile;
    else if (tmx.contains("ellipse"))
      throw tmxObjectError(tmx, "object has unsupported ellipse shape");
    else if (tmx.contains("polygon"))
      throw tmxObjectError(tmx, "object has unsupported polygon shape");
    else if (tmx.contains("polyline"))
      // Wiring
      return ObjectKind::Polyline;
    else
      // Custom brush
      return ObjectKind::Rectangle;
  }

  Maybe<TMXObject::TileObjectInfo> TMXObject::getTileObjectInfo(
      Json const& tmx, TMXTilesetsPtr tilesets, TileLayer layer) {
    Maybe<unsigned> optGid = tmx.optUInt("gid");
    if (optGid.isNothing())
      return {};

    unsigned flipBits = *optGid & TileFlip::AllBits;
    unsigned gid = *optGid & ~TileFlip::AllBits;

    if (flipBits & (TileFlip::Vertical | TileFlip::Diagonal))
      throw tmxObjectError(tmx, "object contains vertical or diagonal flips, which are not supported");

    Tiled::Tile const& gidTile = tilesets->getTile(gid, layer);
    return TileObjectInfo{gidTile.properties, flipBits};
  }

  TileLayer TMXObject::getLayer(Maybe<Json> const& groupProperties, Maybe<Json> const& objectProperties) {
    if (objectProperties.isValid() && objectProperties->contains("layer"))
      return Tiled::LayerNames.getLeft(objectProperties->getString("layer"));
    if (groupProperties.isValid() && groupProperties->contains("layer"))
      return Tiled::LayerNames.getLeft(groupProperties->getString("layer"));
    return TileLayer::Foreground;
  }

  Vec2I TMXObject::getPos(Json const& tmx) {
    return Vec2I(tmx.getInt("x"), tmx.getInt("y")) / TilePixels;
  }

  StarException TMXObject::tmxObjectError(Json const& tmx, String const& msg) {
    Vec2I pos = getPos(tmx);
    return StarException::format("At %d,%d: %s", pos[0], pos[1], msg);
  }

  TMXObjectGroup::TMXObjectGroup(Json const& tmx, TMXTilesetsPtr tilesets) {
    m_name = tmx.getString("name");
    
    // convert group properties in array format to object format
    Maybe<JsonObject> groupProperties = tmx.opt("properties").apply([](Json const& properties) {
        if (properties.type() == Json::Type::Array) {
          JsonObject objectProperties;
          for (auto& p : properties.toArray())
            objectProperties.set(p.getString("name"), p.get("value"));
          return objectProperties;
        } else {
          return properties.toObject();
        }
      });
    for (Json const& tmxObject : tmx.getArray("objects")) {
      TMXObjectPtr object = make_shared<TMXObject>(groupProperties, tmxObject, tilesets);
      m_objects.append(object);
    }
  }

  void TMXPartReader::readAsset(String const& asset) {
    auto assets = Root::singleton().assets();
    m_maps.append(make_pair(asset, make_shared<const TMXMap>(assets->json(asset))));
  }

  Vec2U TMXPartReader::size() const {
    Vec2U size;
    forEachMap([&size](TMXMapConstPtr const& map) {
      size = Vec2U{map->width(), map->height()};
      return true;
    });
    return size;
  }

  void TMXPartReader::forEachTile(TileCallback const& callback) const {
    forEachMap([callback](TMXMapConstPtr const& map) {
        return map->forEachTile(callback);
      });
  }

  void TMXPartReader::forEachTileAt(Vec2I pos, TileCallback const& callback) const {
    forEachMap([pos, callback](TMXMapConstPtr const& map) {
        return map->forEachTileAt(pos, callback);
      });
  }

  void TMXPartReader::forEachMap(function<bool(TMXMapConstPtr const&)> func) const {
    for (auto const& map : m_maps) {
      func(map.second);
    }
  }
}

}
