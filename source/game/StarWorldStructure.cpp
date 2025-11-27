#include "StarWorldStructure.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarImage.hpp"
#include "StarAssets.hpp"

namespace Star {

WorldStructure::WorldStructure() {}

WorldStructure::WorldStructure(String const& configPath) {
  auto assets = Root::singleton().assets();
  auto imgMetadata = Root::singleton().imageMetadataDatabase();
  auto settings = assets->json(configPath);

  m_region = RectI::null();
  m_config = settings.getObject("config", JsonObject());

  // Read all the background / foreground overlays, and combine the image size
  // in tiles with the full structure range

  for (auto overlaySettings : settings.getArray("backgroundOverlays", JsonArray())) {
    Overlay overlay = Overlay{jsonToVec2F(overlaySettings.get("position")),
        AssetPath::relativeTo(configPath, overlaySettings.getString("image")),
        overlaySettings.getBool("fullbright", false)};
    m_backgroundOverlays.append(overlay);
    m_region.combine(RectI::withSize(
        Vec2I::floor(overlay.min), Vec2I::ceil(Vec2F(imgMetadata->imageSize(overlay.image)) / TilePixels)));
  }

  for (auto overlaySettings : settings.getArray("foregroundOverlays", JsonArray())) {
    Overlay overlay = Overlay{jsonToVec2F(overlaySettings.get("position")),
        AssetPath::relativeTo(configPath, overlaySettings.getString("image")),
        overlaySettings.getBool("fullbright", false)};
    m_foregroundOverlays.append(overlay);
    m_region.combine(RectI::withSize(
        Vec2I::floor(overlay.min), Vec2I::ceil(Vec2F(imgMetadata->imageSize(overlay.image)) / TilePixels)));
  }

  // Read block position, keys, and then use that to interpret the block image,
  // if given.

  auto blockPosition = jsonToVec2I(settings.getArray("blocksPosition", JsonArray{0, 0}));
  HashMap<Vec4B, BlockKey> blockKeys;
  auto matDb = Root::singleton().materialDatabase();
  for (auto const& blockKeyConfig :
      assets->fetchJson(settings.get("blockKey", JsonArray()), configPath).iterateArray()) {
    auto foregroundMat = blockKeyConfig.getString("foregroundMat", "");
    MaterialId foregroundMatId;
    if (foregroundMat == "")
      foregroundMatId = StructureMaterialId;
    else
      foregroundMatId = matDb->materialId(foregroundMat);

    auto backgroundMat = blockKeyConfig.getString("backgroundMat", "");
    MaterialId backgroundMatId;
    if (backgroundMat == "")
      backgroundMatId = StructureMaterialId;
    else
      backgroundMatId = matDb->materialId(backgroundMat);

    auto foregroundMod = blockKeyConfig.getString("foregroundMod", "");
    ModId foregroundModId;
    if (foregroundMod == "")
      foregroundModId = NoModId;
    else
      foregroundModId = matDb->modId(foregroundMod);

    auto backgroundMod = blockKeyConfig.getString("backgroundMod", "");
    ModId backgroundModId;
    if (backgroundMod == "")
      backgroundModId = NoModId;
    else
      backgroundModId = matDb->modId(backgroundMod);

    BlockKey blockKey{blockKeyConfig.getBool("anchor", false),
        blockKeyConfig.getBool("foregroundBlock", false),
        foregroundMatId,
        blockKeyConfig.getBool("foregroundResidual", false),
        blockKeyConfig.getBool("backgroundBlock", false),
        backgroundMatId,
        blockKeyConfig.getBool("backgroundResidual", false),
        blockKeyConfig.getString("object", ""),
        DirectionNames.getLeft(blockKeyConfig.getString("objectDirection", "left")),
        blockKeyConfig.getObject("objectParameters", JsonObject()),
        blockKeyConfig.getBool("objectResidual", false),
        jsonToStringList(blockKeyConfig.get("flags", JsonArray())),
        MaterialColorVariant(blockKeyConfig.getUInt("foregroundMatColor", 0)),
        MaterialColorVariant(blockKeyConfig.getUInt("backgroundMatColor", 0)),
        MaterialHue(blockKeyConfig.getUInt("foregroundMatHue", 0)),
        MaterialHue(blockKeyConfig.getUInt("backgroundMatHue", 0)),
        foregroundModId,
        backgroundModId,
      };
    blockKeys[jsonToColor(blockKeyConfig.get("value")).toRgba()] = std::move(blockKey);
  }

  Maybe<Vec2I> anchorPosition;
  if (settings.contains("blockImage")) {
    auto blocksImage = assets->image(AssetPath::relativeTo(configPath, settings.getString("blockImage")));
    const BlockKey defaultBlockKey = {false,
        false,
        StructureMaterialId,
        false,
        false,
        StructureMaterialId,
        false,
        String(),
        Direction::Left,
        Json(),
        false,
        StringList(),
        MaterialColorVariant(0),
        MaterialColorVariant(0),
        MaterialHue(0),
        MaterialHue(0),
        NoModId,
        NoModId
      };

    for (size_t y = 0; y < blocksImage->height(); ++y) {
      for (size_t x = 0; x < blocksImage->width(); ++x) {
        auto blockKey = blockKeys.value(blocksImage->getrgb(x, y), defaultBlockKey);
        auto pos = blockPosition + Vec2I(x, y);

        if (blockKey.anchor) {
          if (anchorPosition)
            throw WorldStructureException(
                strf("Multiple anchor points defined in blockImage, first point is at {}, second at {}",
                    *anchorPosition,
                    pos));
          anchorPosition = pos;
        }

        if (blockKey.foregroundBlock)
          m_foregroundBlocks.append({pos, blockKey.foregroundMat, blockKey.foregroundResidual, blockKey.foregroundMatColor, blockKey.foregroundMatHue, blockKey.foregroundMatMod});
        if (blockKey.backgroundBlock)
          m_backgroundBlocks.append({pos, blockKey.backgroundMat, blockKey.backgroundResidual, blockKey.backgroundMatColor, blockKey.foregroundMatHue, blockKey.foregroundMatMod});

        if (!blockKey.object.empty())
          m_objects.append(Object{
              pos, blockKey.object, blockKey.objectDirection, blockKey.objectParameters, blockKey.objectResidual});

        for (auto const& flag : blockKey.flags)
          m_flaggedBlocks[flag].append(pos);

        m_region.combine(pos);
      }
    }

    if (anchorPosition)
      m_anchorPosition = *anchorPosition;
    else
      m_anchorPosition = m_region.center();

    // Objects put into the list are from top to bottom, need to place them
    // from bottom to top for objects on top of other objects.
    reverse(m_objects);
  }
}

WorldStructure::WorldStructure(Json const& store) {
  auto overlayFromJson = [](Json const& v) -> Overlay {
    return Overlay{jsonToVec2F(v.get("min")), v.getString("image"), v.getBool("fullbright")};
  };

  auto blockFromJson = [](Json const& v) -> Block {
    return Block{jsonToVec2I(v.get("position")),
      MaterialId(v.getUInt("materialId")),
      v.getBool("residual"),
      MaterialColorVariant(v.getUInt("materialColor", 0)),
      MaterialHue(v.getUInt("materialHue", 0)),
      ModId(v.getUInt("modId", NoModId))
    };
  };

  auto objectFromJson = [](Json const& v) -> Object {
    return Object{jsonToVec2I(v.get("position")),
        v.getString("name"),
        DirectionNames.getLeft(v.getString("direction")),
        v.get("parameters", {}),
        v.getBool("residual", false)};
  };

  m_region = jsonToRectI(store.get("region"));
  m_anchorPosition = jsonToVec2I(store.get("anchorPosition"));
  m_config = store.get("config");
  m_backgroundOverlays = store.getArray("backgroundOverlays").transformed(overlayFromJson);
  m_foregroundOverlays = store.getArray("foregroundOverlays").transformed(overlayFromJson);
  m_backgroundBlocks = store.getArray("backgroundBlocks").transformed(blockFromJson);
  m_foregroundBlocks = store.getArray("foregroundBlocks").transformed(blockFromJson);
  m_objects = store.getArray("objects").transformed(objectFromJson);
  m_flaggedBlocks = transform<StringMap<List<Vec2I>>>(store.getObject("flaggedBlocks"),
      [](pair<String, Json> const& p) { return make_pair(p.first, p.second.toArray().transformed(jsonToVec2I)); });
}

Json WorldStructure::configValue(String const& name) const {
  return m_config.get(name, Json());
}

auto WorldStructure::backgroundOverlays() const -> List<Overlay> const & {
  return m_backgroundOverlays;
}

auto WorldStructure::foregroundOverlays() const -> List<Overlay> const & {
  return m_foregroundOverlays;
}

auto WorldStructure::backgroundBlocks() const -> List<Block> const & {
  return m_backgroundBlocks;
}

auto WorldStructure::foregroundBlocks() const -> List<Block> const & {
  return m_foregroundBlocks;
}

auto WorldStructure::objects() const -> List<Object> const & {
  return m_objects;
}

auto WorldStructure::flaggedBlocks(String const& flag) const -> List<Vec2I> {
  return m_flaggedBlocks.value(flag);
}

RectI WorldStructure::region() const {
  return m_region;
}

Vec2I WorldStructure::anchorPosition() const {
  return m_anchorPosition;
}

void WorldStructure::setAnchorPosition(Vec2I const& anchorPosition) {
  translate(anchorPosition - m_anchorPosition);
}

void WorldStructure::translate(Vec2I const& distance) {
  if (!m_region.isNull())
    m_region.translate(distance);

  m_anchorPosition += distance;

  for (auto& bg : m_backgroundOverlays)
    bg.min += Vec2F(distance);

  for (auto& fg : m_foregroundOverlays)
    fg.min += Vec2F(distance);

  for (auto& b : m_backgroundBlocks)
    b.position += distance;

  for (auto& b : m_foregroundBlocks)
    b.position += distance;

  for (auto& object : m_objects)
    object.position += distance;

  for (auto& flagPair : m_flaggedBlocks) {
    for (auto& pos : flagPair.second)
      pos += distance;
  }
}

Json WorldStructure::store() const {
  auto overlayToJson = [](Overlay const& o) -> Json {
    return JsonObject{{"min", jsonFromVec2F(o.min)}, {"image", o.image}, {"fullbright", o.fullbright}};
  };

  auto blockToJson = [](Block const& b) -> Json {
    return JsonObject{
      {"position", jsonFromVec2I(b.position)},
      {"materialId", b.materialId},
      {"residual", b.residual},
      {"materialColor", b.materialColor},
      {"materialHue", b.materialHue},
      {"modId", b.materialMod}

    };
  };

  auto objectToJson = [](Object const& o) -> Json {
    return JsonObject{
        {"position", jsonFromVec2I(o.position)},
        {"name", o.name},
        {"direction", DirectionNames.getRight(o.direction)},
        {"parameters", o.parameters},
        {"residual", o.residual},
    };
  };

  return JsonObject{{"region", jsonFromRectI(m_region)},
      {"anchorPosition", jsonFromVec2I(m_anchorPosition)},
      {"config", m_config},
      {"backgroundOverlays", m_backgroundOverlays.transformed(overlayToJson)},
      {"foregroundOverlays", m_foregroundOverlays.transformed(overlayToJson)},
      {"backgroundBlocks", m_backgroundBlocks.transformed(blockToJson)},
      {"foregroundBlocks", m_foregroundBlocks.transformed(blockToJson)},
      {"objects", m_objects.transformed(objectToJson)},
      {"flaggedBlocks",
          transform<JsonObject>(m_flaggedBlocks,
              [](pair<String, List<Vec2I>> const& p) {
                return pair<String, Json>(p.first, p.second.transformed(jsonFromVec2I));
              })}};
}

}
