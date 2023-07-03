#include "StarPlant.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorld.hpp"
#include "StarRoot.hpp"
#include "StarObjectDatabase.hpp"
#include "StarPlantDrop.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarAssets.hpp"
#include "StarImage.hpp"
#include "StarEntityRendering.hpp"
#include "StarParticleDatabase.hpp"

namespace Star {

float const Plant::PlantScanThreshold = 0.1f;

EnumMap<Plant::RotationType> const Plant::RotationTypeNames{
  {Plant::RotationType::DontRotate, "dontRotate"},
  {Plant::RotationType::RotateBranch, "rotateBranch"},
  {Plant::RotationType::RotateLeaves, "rotateLeaves"},
  {Plant::RotationType::RotateCrownBranch, "rotateCrownBranch"},
  {Plant::RotationType::RotateCrownLeaves, "rotateCrownLeaves"},
};

Plant::PlantPiece::PlantPiece() {
  image = "";
  imagePath = AssetPath();
  offset = {};
  segmentIdx = 0;
  structuralSegment = 0;
  kind = PlantPieceKind::None;
  zLevel = 0;
  rotationType = RotationType::DontRotate;
  rotationOffset = 0;
  spaces = {};
  flip = false;
}

Plant::Plant(TreeVariant const& config, uint64_t seed) : Plant() {
  m_broken = false;
  m_tilePosition = Vec2I();
  m_windTime = 0.0f;
  m_windLevel = 0.0f;
  m_ceiling = config.ceiling;
  m_piecesScanned = false;
  m_fallsWhenDead = true;
  m_piecesUpdated = true;
  m_tileDamageEvent = false;

  m_stemDropConfig = config.stemDropConfig;
  m_foliageDropConfig = config.foliageDropConfig;
  if (m_stemDropConfig.isNull())
    m_stemDropConfig = JsonObject();
  if (m_foliageDropConfig.isNull())
    m_foliageDropConfig = JsonObject();

  m_stemDropConfig = m_stemDropConfig.set("hueshift", config.stemHueShift);
  m_foliageDropConfig = m_foliageDropConfig.set("hueshift", config.foliageHueShift);

  JsonObject saplingDropConfig;
  saplingDropConfig["stemName"] = config.stemName;
  saplingDropConfig["stemHueShift"] = config.stemHueShift;
  if (!m_foliageDropConfig.isNull()) {
    saplingDropConfig["foliageName"] = config.foliageName;
    saplingDropConfig["foliageHueShift"] = config.foliageHueShift;
  }
  m_saplingDropConfig = saplingDropConfig;

  RandomSource rnd(seed);

  float xOffset = 0;
  float yOffset = 0;

  float roffset = Random::randf() * 0.5f;

  m_descriptions = config.descriptions;
  m_ephemeral = config.ephemeral;
  m_tileDamageParameters = config.tileDamageParameters;

  int segment = 0;

  auto assets = Root::singleton().assets();

  // base
  {
    JsonObject bases = config.stemSettings.get("base").toObject();
    String baseKey = bases.keys()[rnd.randInt(bases.size() - 1)];
    JsonObject baseSettings = bases[baseKey].toObject();

    JsonObject attachmentSettings = baseSettings["attachment"].toObject();

    xOffset += attachmentSettings.get("bx").toDouble() / TilePixels;
    yOffset += attachmentSettings.get("by").toDouble() / TilePixels;

    String baseFile = AssetPath::relativeTo(config.stemDirectory, baseSettings.get("image").toString());
    float baseImageHeight = assets->image(baseFile)->height();
    if (config.ceiling)
      yOffset = 1.0 - baseImageHeight / TilePixels;

    {
      PlantPiece piece;
      piece.image = strf("{}?hueshift={}", baseFile, config.stemHueShift);
      piece.offset = Vec2F(xOffset, yOffset);
      piece.segmentIdx = segment;
      piece.structuralSegment = true;
      piece.kind = PlantPieceKind::Stem;
      piece.zLevel = 0.0f;
      piece.rotationType = DontRotate;
      piece.rotationOffset = Random::randf() + roffset;
      m_pieces.append(piece);
    }

    // base leaves
    JsonObject baseLeaves = config.foliageSettings.getObject("baseLeaves", {});
    if (baseLeaves.contains(baseKey)) {
      JsonObject baseLeavesSettings = baseLeaves.get(baseKey).toObject();
      JsonObject attachmentSettings = baseLeavesSettings["attachment"].toObject();

      float xOf = xOffset + attachmentSettings.get("bx").toDouble() / TilePixels;
      float yOf = yOffset + attachmentSettings.get("by").toDouble() / TilePixels;

      if (baseLeavesSettings.contains("image") && !baseLeavesSettings.get("image").toString().empty()) {
        String baseLeavesFile =
            AssetPath::relativeTo(config.foliageDirectory, baseLeavesSettings.get("image").toString());

        PlantPiece piece;
        piece.image = strf("{}?hueshift={}", baseLeavesFile, config.foliageHueShift);
        piece.offset = Vec2F{xOf, yOf};
        piece.segmentIdx = segment;
        piece.structuralSegment = false;
        piece.kind = PlantPieceKind::Foliage;
        piece.zLevel = 3.0f;
        piece.rotationType = m_ceiling ? DontRotate : RotateLeaves;
        piece.rotationOffset = Random::randf() + roffset;
        m_pieces.append(piece);
      }

      if (baseLeavesSettings.contains("backimage") && !baseLeavesSettings.get("backimage").toString().empty()) {
        String baseLeavesBackFile =
            AssetPath::relativeTo(config.foliageDirectory, baseLeavesSettings.get("backimage").toString());
        PlantPiece piece;
        piece.image = strf("{}?hueshift={}", baseLeavesBackFile, config.foliageHueShift);
        piece.offset = Vec2F{xOf, yOf};
        piece.segmentIdx = segment;
        piece.structuralSegment = false;
        piece.kind = PlantPieceKind::Foliage;
        piece.zLevel = -1.0f;
        piece.rotationType = m_ceiling ? DontRotate : RotateLeaves;
        piece.rotationOffset = Random::randf() + roffset;
        m_pieces.append(piece);
      }
    }

    xOffset += attachmentSettings.get("x").toDouble() / TilePixels;
    yOffset += attachmentSettings.get("y").toDouble() / TilePixels; // trunk height

    segment++;
  }

  float branchYOffset = yOffset;

  // trunk
  {
    JsonObject middles = config.stemSettings.get("middle").toObject();

    int middleHeight = config.stemSettings.getInt("middleMinSize", 1) + rnd.randInt(config.stemSettings.getInt("middleMaxSize", 6) - config.stemSettings.getInt("middleMinSize", 1));

    bool hasBranches = config.stemSettings.contains("branch");
    JsonObject branches;
    if (hasBranches) {
      branches = config.stemSettings.get("branch").toObject();
      if (branches.size() == 0)
        hasBranches = false;
    }

    for (int i = 0; i < middleHeight; i++) {
      String middleKey = middles.keys()[rnd.randInt(middles.size() - 1)];
      JsonObject middleSettings = middles[middleKey].toObject();
      JsonObject attachmentSettings = middleSettings["attachment"].toObject();

      xOffset += attachmentSettings.get("bx").toDouble() / TilePixels;
      yOffset += attachmentSettings.get("by").toDouble() / TilePixels;

      String middleFile = AssetPath::relativeTo(config.stemDirectory, middleSettings.get("image").toString());

      {
        PlantPiece piece;
        piece.image = strf("{}?hueshift={}", middleFile, config.stemHueShift);
        piece.offset = Vec2F(xOffset, yOffset);
        piece.segmentIdx = segment;
        piece.structuralSegment = true;
        piece.kind = PlantPieceKind::Stem;
        piece.zLevel = 1.0f;
        piece.rotationType = DontRotate;
        piece.rotationOffset = Random::randf() + roffset;
        m_pieces.append(piece);
      }

      // trunk leaves
      JsonObject trunkLeaves = config.foliageSettings.getObject("trunkLeaves", {});
      if (trunkLeaves.contains(middleKey)) {
        JsonObject trunkLeavesSettings = trunkLeaves.get(middleKey).toObject();
        JsonObject attachmentSettings = trunkLeavesSettings["attachment"].toObject();

        float xOf = xOffset + attachmentSettings.get("bx").toDouble() / TilePixels;
        float yOf = yOffset + attachmentSettings.get("by").toDouble() / TilePixels;

        if (trunkLeavesSettings.contains("image") && !trunkLeavesSettings.get("image").toString().empty()) {
          String trunkLeavesFile =
              AssetPath::relativeTo(config.foliageDirectory, trunkLeavesSettings.get("image").toString());
          PlantPiece piece;
          piece.image = strf("{}?hueshift={}", trunkLeavesFile, config.foliageHueShift);
          piece.offset = Vec2F{xOf, yOf};
          piece.segmentIdx = segment;
          piece.structuralSegment = false;
          piece.kind = PlantPieceKind::Foliage;
          piece.zLevel = 3.0f;
          piece.rotationType = m_ceiling ? DontRotate : RotateLeaves;
          piece.rotationOffset = Random::randf() + roffset;
          m_pieces.append(piece);
        }

        if (trunkLeavesSettings.contains("backimage") && !trunkLeavesSettings.get("backimage").toString().empty()) {
          String trunkLeavesBackFile =
              AssetPath::relativeTo(config.foliageDirectory, trunkLeavesSettings.get("backimage").toString());
          PlantPiece piece;
          piece.image = strf("{}?hueshift={}", trunkLeavesBackFile, config.foliageHueShift);
          piece.offset = Vec2F{xOf, yOf};
          piece.segmentIdx = segment;
          piece.structuralSegment = false;
          piece.kind = PlantPieceKind::Foliage;
          piece.zLevel = -1.0f;
          piece.rotationType = m_ceiling ? DontRotate : RotateLeaves;
          piece.rotationOffset = Random::randf() + roffset;
          m_pieces.append(piece);
        }
      }

      xOffset += attachmentSettings.get("x").toDouble() / TilePixels;
      yOffset += attachmentSettings.get("y").toDouble() / TilePixels;

      // branch
      while (hasBranches && (yOffset >= branchYOffset) && ((middleHeight - i) > 0)) {
        String branchKey = branches.keys()[rnd.randInt(branches.size() - 1)];
        JsonObject branchSettings = branches[branchKey].toObject();
        JsonObject attachmentSettings = branchSettings["attachment"].toObject();

        float h = attachmentSettings.get("h").toDouble() / TilePixels;
        if (yOffset < branchYOffset + (h / 2.0f))
          break;

        float xO = xOffset + attachmentSettings.get("bx").toDouble() / TilePixels;
        float yO = branchYOffset + attachmentSettings.get("by").toDouble() / TilePixels;

        if (config.stemSettings.getBool("alwaysBranch", false) || rnd.randInt(2 + i) != 0) {
          float boffset = Random::randf() + roffset;
          String branchFile = AssetPath::relativeTo(config.stemDirectory, branchSettings.get("image").toString());
          {
            PlantPiece piece;
            piece.image = strf("{}?hueshift={}", branchFile, config.stemHueShift);
            piece.offset = Vec2F{xO, yO};
            piece.segmentIdx = segment;
            piece.structuralSegment = false;
            piece.kind = PlantPieceKind::Stem;
            piece.zLevel = 0.0f;
            piece.rotationType = m_ceiling ? DontRotate : RotateBranch;
            piece.rotationOffset = boffset;
            m_pieces.append(piece);
          }
          branchYOffset += h;

          // branch leaves
          JsonObject branchLeaves = config.foliageSettings.getObject("branchLeaves", {});
          if (branchLeaves.contains(branchKey)) {
            JsonObject branchLeavesSettings = branchLeaves.get(branchKey).toObject();
            JsonObject attachmentSettings = branchLeavesSettings["attachment"].toObject();

            float xOf = xO + attachmentSettings.get("bx").toDouble() / TilePixels;
            float yOf = yO + attachmentSettings.get("by").toDouble() / TilePixels;

            if (branchLeavesSettings.contains("image") && !branchLeavesSettings.get("image").toString().empty()) {
              String branchLeavesFile =
                  AssetPath::relativeTo(config.foliageDirectory, branchLeavesSettings.get("image").toString());
              PlantPiece piece;
              piece.image = strf("{}?hueshift={}", branchLeavesFile, config.foliageHueShift);
              piece.offset = Vec2F{xOf, yOf};
              piece.segmentIdx = segment;
              piece.structuralSegment = false;
              piece.kind = PlantPieceKind::Foliage;
              piece.zLevel = 3.0f;
              piece.rotationType = m_ceiling ? DontRotate : RotateLeaves;
              piece.rotationOffset = boffset;
              m_pieces.append(piece);
            }

            if (branchLeavesSettings.contains("backimage")
                && !branchLeavesSettings.get("backimage").toString().empty()) {
              String branchLeavesBackFile =
                  AssetPath::relativeTo(config.foliageDirectory, branchLeavesSettings.get("backimage").toString());
              PlantPiece piece;
              piece.image = strf("{}?hueshift={}", branchLeavesBackFile, config.foliageHueShift);
              piece.offset = Vec2F{xOf, yOf};
              piece.segmentIdx = segment;
              piece.structuralSegment = false;
              piece.kind = PlantPieceKind::Foliage;
              piece.zLevel = -1.0f;
              piece.rotationType = m_ceiling ? DontRotate : RotateLeaves;
              piece.rotationOffset = boffset;
              m_pieces.append(piece);
            }
          }

        } else {
          branchYOffset += (attachmentSettings.get("h").toDouble() / TilePixels) / (float)(1 + rnd.randInt(4));
        }
      }
      segment++;
    }
  }

  // crown
  {
    JsonObject crowns = config.stemSettings.getObject("crown", {});
    bool hasCrown = crowns.size() > 0;
    if (hasCrown) {
      String crownKey = crowns.keys()[rnd.randInt(crowns.size() - 1)];
      JsonObject crownSettings = crowns[crownKey].toObject();

      JsonObject attachmentSettings = crownSettings["attachment"].toObject();

      xOffset += attachmentSettings.get("bx").toDouble() / TilePixels;
      yOffset += attachmentSettings.get("by").toDouble() / TilePixels;

      float coffset = roffset + Random::randf();

      String crownFile = AssetPath::relativeTo(config.stemDirectory, crownSettings.get("image").toString());
      {
        PlantPiece piece;
        piece.image = strf("{}?hueshift={}", crownFile, config.stemHueShift);
        piece.offset = Vec2F{xOffset, yOffset};
        piece.segmentIdx = segment;
        piece.structuralSegment = false;
        piece.kind = PlantPieceKind::Stem;
        piece.zLevel = 0.0f;
        piece.rotationType = m_ceiling ? DontRotate : RotateCrownBranch;
        piece.rotationOffset = coffset;
        m_pieces.append(piece);
      }
      // crown leaves
      JsonObject crownLeaves = config.foliageSettings.getObject("crownLeaves", {});
      if (crownLeaves.contains(crownKey)) {
        JsonObject crownLeavesSettings = crownLeaves.get(crownKey).toObject();

        JsonObject attachmentSettings = crownLeavesSettings["attachment"].toObject();

        float xO = xOffset + attachmentSettings.get("bx").toDouble() / TilePixels;
        float yO = yOffset + attachmentSettings.get("by").toDouble() / TilePixels;

        if (crownLeavesSettings.contains("image") && !crownLeavesSettings.get("image").toString().empty()) {
          String crownLeavesFile =
              AssetPath::relativeTo(config.foliageDirectory, crownLeavesSettings.get("image").toString());

          PlantPiece piece;
          piece.image = strf("{}?hueshift={}", crownLeavesFile, config.foliageHueShift);
          piece.offset = Vec2F{xO, yO};
          piece.segmentIdx = segment;
          piece.structuralSegment = false;
          piece.kind = PlantPieceKind::Foliage;
          piece.zLevel = 3.0f;
          piece.rotationType = m_ceiling ? DontRotate : RotateCrownLeaves;
          piece.rotationOffset = coffset;
          m_pieces.append(piece);
        }

        if (crownLeavesSettings.contains("backimage") && !crownLeavesSettings.get("backimage").toString().empty()) {
          String crownLeavesBackFile =
              AssetPath::relativeTo(config.foliageDirectory, crownLeavesSettings.get("backimage").toString());

          PlantPiece piece;
          piece.image = strf("{}?hueshift={}", crownLeavesBackFile, config.foliageHueShift);
          piece.offset = Vec2F{xO, yO};
          piece.segmentIdx = segment;
          piece.structuralSegment = false;
          piece.kind = PlantPieceKind::Foliage;
          piece.zLevel = -1.0f;
          piece.rotationType = m_ceiling ? DontRotate : RotateCrownLeaves;
          piece.rotationOffset = coffset;
          m_pieces.append(piece);
        }
      }
    }
  }

  sort(m_pieces, [](PlantPiece const& a, PlantPiece const& b) { return a.zLevel < b.zLevel; });
  validatePieces();
  setupNetStates();
}

Json Plant::diskStore() const {
  return JsonObject{
    {"tilePosition", jsonFromVec2I(m_tilePosition)},
    {"ceiling", m_ceiling},
    {"stemDropConfig", m_stemDropConfig},
    {"foliageDropConfig", m_foliageDropConfig},
    {"saplingDropConfig", m_saplingDropConfig},
    {"descriptions", m_descriptions},
    {"ephemeral", m_ephemeral},
    {"tileDamageParameters", m_tileDamageParameters.toJson()},
    {"fallsWhenDead", m_fallsWhenDead},
    {"pieces", writePiecesToJson()},
  };
}

ByteArray Plant::netStore() const {
  DataStreamBuffer ds;
  ds.viwrite(m_tilePosition[0]);
  ds.viwrite(m_tilePosition[1]);
  ds.write(m_ceiling);
  ds.write(m_stemDropConfig);
  ds.write(m_foliageDropConfig);
  ds.write(m_saplingDropConfig);
  ds.write(m_descriptions);
  ds.write(m_ephemeral);
  ds.write(m_tileDamageParameters);
  ds.write(m_fallsWhenDead);
  m_tileDamageStatus.netStore(ds);
  ds.write(writePieces());

  return ds.takeData();
}

Plant::Plant(GrassVariant const& config, uint64_t seed) : Plant() {
  m_broken = false;
  m_tilePosition = Vec2I();
  m_ceiling = false;
  m_windTime = 0.0f;
  m_windLevel = 0.0f;
  m_piecesScanned = false;
  m_fallsWhenDead = false;
  m_descriptions = config.descriptions;
  m_ephemeral = config.ephemeral;
  m_tileDamageParameters = config.tileDamageParameters;
  m_piecesUpdated = true;

  RandomSource rand(seed);

  String imageName = AssetPath::relativeTo(config.directory, rand.randValueFrom(config.images));

  Vec2F offset = Vec2F();
  // If this is a ceiling plant, offset the image so that the [0, 0] space is
  // at the top
  if (config.ceiling) {
    auto imgMetadata = Root::singleton().imageMetadataDatabase();
    float imageHeight = imgMetadata->imageSize(imageName)[1];
    offset = Vec2F(0.0f, 1.0f - imageHeight / TilePixels);
  }

  PlantPiece piece;
  piece.image = strf("{}?hueshift={}", imageName, config.hueShift);
  piece.offset = offset;
  piece.segmentIdx = 0;
  piece.structuralSegment = true;
  piece.kind = PlantPieceKind::None;
  m_pieces = {piece};

  m_ceiling = config.ceiling;
  validatePieces();
  setupNetStates();
}

Plant::Plant(BushVariant const& config, uint64_t seed) : Plant() {
  m_broken = false;
  m_tilePosition = Vec2I();
  m_ceiling = false;
  m_windTime = 0.0f;
  m_windLevel = 0.0f;
  m_piecesScanned = false;
  m_fallsWhenDead = false;
  m_descriptions = config.descriptions;
  m_ephemeral = config.ephemeral;
  m_tileDamageParameters = config.tileDamageParameters;
  m_piecesUpdated = true;

  RandomSource rand(seed);
  auto assets = Root::singleton().assets();

  auto shape = rand.randValueFrom(config.shapes);
  String shapeImageName = AssetPath::relativeTo(config.directory, shape.image);
  float shapeImageHeight = assets->image(shapeImageName)->height();
  Vec2F offset = Vec2F();
  // If this is a ceiling plant, offset the image so that the [0, 0] space is
  // at the top
  if (config.ceiling)
    offset = Vec2F(0.0f, 1.0f - shapeImageHeight / TilePixels);

  {
    PlantPiece piece;
    piece.image = strf("{}?hueshift={}", shapeImageName, config.baseHueShift);
    piece.offset = offset;
    piece.segmentIdx = 0;
    piece.structuralSegment = true;
    piece.kind = PlantPieceKind::None;
    m_pieces.append(piece);
  }

  auto mod = rand.randValueFrom(shape.mods);
  if (!mod.empty()) {
    PlantPiece piece;
    piece.image = strf("{}?hueshift={}", AssetPath::relativeTo(config.directory, mod), config.modHueShift);
    piece.offset = offset;
    piece.segmentIdx = 0;
    piece.structuralSegment = false;
    piece.kind = PlantPieceKind::None;
    m_pieces.append(piece);
  }

  m_ceiling = config.ceiling;
  validatePieces();
  setupNetStates();
}

Plant::Plant(Json const& diskStore) : Plant() {
  m_tilePosition = jsonToVec2I(diskStore.get("tilePosition"));
  m_ceiling = diskStore.getBool("ceiling");
  m_stemDropConfig = diskStore.get("stemDropConfig");
  m_foliageDropConfig = diskStore.get("foliageDropConfig");
  m_saplingDropConfig = diskStore.get("saplingDropConfig");
  m_descriptions = diskStore.get("descriptions");
  m_ephemeral = diskStore.getBool("ephemeral");
  m_tileDamageParameters = TileDamageParameters(diskStore.get("tileDamageParameters"));
  m_fallsWhenDead = diskStore.getBool("fallsWhenDead");
  readPiecesFromJson(diskStore.get("pieces"));

  setupNetStates();
}

Plant::Plant(ByteArray const& netStore) : Plant() {
  m_broken = false;
  m_tilePosition = Vec2I();
  m_ceiling = false;
  m_windTime = 0.0f;
  m_windLevel = 0.0f;
  m_piecesScanned = false;
  m_fallsWhenDead = false;
  m_piecesUpdated = true;

  DataStreamBuffer ds(netStore);
  ds.viread(m_tilePosition[0]);
  ds.viread(m_tilePosition[1]);
  ds.read(m_ceiling);
  ds.read(m_stemDropConfig);
  ds.read(m_foliageDropConfig);
  ds.read(m_saplingDropConfig);
  ds.read(m_descriptions);
  ds.read(m_ephemeral);
  ds.read(m_tileDamageParameters);
  ds.read(m_fallsWhenDead);
  m_tileDamageStatus.netLoad(ds);
  readPieces(ds.read<ByteArray>());

  setupNetStates();
}

Plant::Plant() {
  m_ephemeral = false;
  m_piecesUpdated = true;
  m_ceiling = false;
  m_broken = false;
  m_fallsWhenDead = false;
  m_windTime = 0.0f;
  m_windLevel = 0.0f;
  m_piecesScanned = false;
  m_tileDamageX = 0.0f;
  m_tileDamageY = 0.0f;
  m_tileDamageEventTrigger = false;
  m_tileDamageEvent = false;
}

EntityType Plant::entityType() const {
  return EntityType::Plant;
}

void Plant::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);
  validatePieces();
  m_tilePosition = world->geometry().xwrap(m_tilePosition);
}

pair<ByteArray, uint64_t> Plant::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void Plant::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(move(data), interpolationTime);
}

void Plant::enableInterpolation(float extrapolationHint) {
  // Only enable plant interpolation when it actually matters, for things that
  // generate PlantDrops so that they match when the PlantDrops appear.
  if (m_fallsWhenDead)
    m_netGroup.enableNetInterpolation(extrapolationHint);
}

void Plant::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

String Plant::description() const {
  return m_descriptions.getString("description");
}

Vec2F Plant::position() const {
  return Vec2F(m_tilePosition);
}

RectF Plant::metaBoundBox() const {
  return m_metaBoundBox;
}

bool Plant::ephemeral() const {
  return m_ephemeral;
}

Vec2I Plant::tilePosition() const {
  return m_tilePosition;
}

void Plant::setTilePosition(Vec2I const& tilePosition) {
  m_tilePosition = tilePosition;
}

List<Vec2I> Plant::spaces() const {
  return m_spaces;
}

List<Vec2I> Plant::roots() const {
  return m_roots;
}

Vec2I Plant::primaryRoot() const {
  return m_ceiling ? Vec2I(0, 1) : Vec2I(0, -1);
}

bool Plant::ceiling() const {
  return m_ceiling;
}

bool Plant::shouldDestroy() const {
  return m_broken || m_pieces.size() == 0;
}

bool Plant::checkBroken() {
  if (!m_broken) {
    if (!allSpacesOccupied(m_roots)) {
      if (m_fallsWhenDead) {
        breakAtPosition(m_tilePosition, Vec2F(m_tilePosition));
        return false;
      } else
        m_broken = true;
    } else if (anySpacesOccupied(m_spaces))
      m_broken = true;
  }

  return m_broken;
}

List<Plant::PlantPiece> Plant::pieces() const {
  return m_pieces;
}

RectF Plant::interactiveBoundBox() const {
  return RectF(m_boundBox);
}

void Plant::scanSpacesAndRoots() {
  auto imageMetadataDatabase = Root::singleton().imageMetadataDatabase();

  // build spaces
  Set<Vec2I> spaces;

  // always include the base position in spaces, it causes all kinds of problems if you don't
  spaces.add({0, 0});

  for (auto& piece : m_pieces) {
    piece.imageSize = imageMetadataDatabase->imageSize(piece.image);
    piece.spaces = Set<Vec2I>::from(
        imageMetadataDatabase->imageSpaces(piece.image, piece.offset * TilePixels, PlantScanThreshold, piece.flip));
    spaces.addAll(piece.spaces);
  }

  m_spaces = spaces.values();

  m_boundBox = RectI::boundBoxOfPoints(m_spaces);

  for (auto space : m_spaces) {
    if (space[1] == 0) {
      if (m_ceiling)
        m_roots.push_back({space[0], 1});
      else
        m_roots.push_back({space[0], -1});
    }
  }
}

void Plant::calcBoundBox() {
  RectF boundBox = RectF::boundBoxOfPoints(m_spaces);
  // Plants are allowed to visibly occupy one outside space from the spaces
  // they take up.
  m_metaBoundBox = RectF(boundBox.min() - Vec2F(1, 1), boundBox.max() + Vec2F(2, 2));
}

float Plant::branchRotation(float xPos, float rotoffset) const {
  if (!inWorld() || m_windLevel == 0.0f)
    return 0.0f;

  float intensity = fabs(m_windLevel);

  return copysign(0.00117f, m_windLevel) * (std::sin(m_windTime + rotoffset + xPos / 10.0f) * intensity - intensity / 300.0f);
}

void Plant::update(uint64_t) {
  m_windTime += WorldTimestep;
  m_windTime = std::fmod(m_windTime, 628.32f);
  m_windLevel = world()->windLevel(Vec2F(m_tilePosition));

  if (isMaster()) {
    if (m_tileDamageStatus.damaged())
      m_tileDamageStatus.recover(m_tileDamageParameters, WorldTimestep);
  } else {
    if (m_tileDamageStatus.damaged() && !m_tileDamageStatus.damageProtected()) {
      float damageEffectPercentage = m_tileDamageStatus.damageEffectPercentage();
      m_windTime += damageEffectPercentage * 10 * WorldTimestep;
      m_windLevel += damageEffectPercentage * 20;
    }

    m_netGroup.tickNetInterpolation(WorldTimestep);
  }
}

void Plant::render(RenderCallback* renderCallback) {
  float damageXOffset = Random::randf(-0.1f, 0.1f) * m_tileDamageStatus.damageEffectPercentage();

  for (auto const& plantPiece : m_pieces) {
    auto size = Vec2F(plantPiece.imageSize) / TilePixels;

    Vec2F offset = plantPiece.offset;
    if ((m_ceiling && offset[1] <= m_tileDamageY) || (!m_ceiling && offset[1] + size[1] >= m_tileDamageY))
      offset[0] += damageXOffset;

    auto drawable = Drawable::makeImage(plantPiece.imagePath, 1.0f / TilePixels, false, offset);
    if (plantPiece.flip)
      drawable.scale(Vec2F(-1, 1));

    if (plantPiece.rotationType == RotateCrownBranch || plantPiece.rotationType == RotateCrownLeaves) {
      drawable.rotate(branchRotation(m_tilePosition[0], plantPiece.rotationOffset * 1.4f) * 0.7f, plantPiece.offset + Vec2F(size[0] / 2.0f, 0));
      drawable.translate(Vec2F(0, -0.40f));
    } else if (plantPiece.rotationType == RotateBranch || plantPiece.rotationType == RotateLeaves) {
      drawable.rotate(branchRotation(m_tilePosition[0], plantPiece.rotationOffset * 1.4f), plantPiece.offset + Vec2F(size) / 2.0f);
    }
    drawable.translate(position());
    renderCallback->addDrawable(move(drawable), RenderLayerPlant);
  }

  if (m_tileDamageEvent) {
    m_tileDamageEvent = false;
    if (m_stemDropConfig.type() == Json::Type::Object) {
      Json particleConfig = m_stemDropConfig.get("particles", JsonObject()).get("damageTree", JsonObject());
      JsonArray particleOptions = particleConfig.getArray("options", {});
      auto hueshift = m_stemDropConfig.getFloat("hueshift", 0) / 360.0f;
      auto density = particleConfig.getFloat("density", 1);
      while (density-- > 0) {
        auto config = Random::randValueFrom(particleOptions, {});
        if (config.isNull() || config.size() == 0)
          continue;
        auto particle = Root::singleton().particleDatabase()->particle(config);
        particle.color.hueShift(hueshift);
        if (!particle.string.empty()) {
          particle.string = strf("{}?hueshift={}", particle.string, hueshift);
          particle.image = particle.string;
        }
        particle.position = {m_tileDamageX + Random::randf(), m_tileDamageY + Random::randf()};
        particle.translate(position());
        renderCallback->addParticle(move(particle));
      }
      JsonArray damageTreeSoundOptions = m_stemDropConfig.get("sounds", JsonObject()).getArray("damageTree", JsonArray());
      if (damageTreeSoundOptions.size()) {
        auto sound = Random::randFrom(damageTreeSoundOptions);
        Vec2F pos = position() + Vec2F(m_tileDamageX + Random::randf(), m_tileDamageY + Random::randf());
        auto assets = Root::singleton().assets();
        auto audioInstance = make_shared<AudioInstance>(*assets->audio(sound.getString("file")));
        audioInstance->setPosition(pos);
        audioInstance->setVolume(sound.getFloat("volume", 1.0f));
        renderCallback->addAudio(move(audioInstance));
      }
    }
  }
}

void Plant::readPieces(ByteArray pieces) {
  if (!pieces.empty()) {
    DataStreamBuffer ds(move(pieces));
    ds.readContainer(m_pieces, [](DataStream& ds, PlantPiece& piece) {
        ds.read(piece.image);
        ds.read(piece.offset[0]);
        ds.read(piece.offset[1]);
        ds.read(piece.rotationType);
        ds.read(piece.rotationOffset);
        ds.read(piece.structuralSegment);
        ds.read(piece.kind);
        ds.read(piece.segmentIdx);
        ds.read(piece.flip);
      });
    m_piecesScanned = false;
    if (inWorld())
      validatePieces();
  }
}

ByteArray Plant::writePieces() const {
  return DataStreamBuffer::serializeContainer(m_pieces, [](DataStream& ds, PlantPiece const& piece) {
      ds.write(piece.image);
      ds.write(piece.offset[0]);
      ds.write(piece.offset[1]);
      ds.write(piece.rotationType);
      ds.write(piece.rotationOffset);
      ds.write(piece.structuralSegment);
      ds.write(piece.kind);
      ds.write(piece.segmentIdx);
      ds.write(piece.flip);
    });
}

void Plant::readPiecesFromJson(Json const& pieces) {
  m_pieces = jsonToList<PlantPiece>(pieces, [](Json const& v) -> PlantPiece {
      PlantPiece res;
      res.image = v.getString("image");
      res.offset = jsonToVec2F(v.get("offset"));
      res.rotationType = RotationTypeNames.getLeft(v.getString("rotationType"));
      res.rotationOffset = v.getFloat("rotationOffset");
      res.structuralSegment = v.getBool("structuralSegment");
      res.kind = (PlantPieceKind)v.getInt("kind");
      res.segmentIdx = v.getInt("segmentIdx");
      res.flip = v.getBool("flip");

      return res;
    });
  m_piecesScanned = false;
  if (inWorld())
    validatePieces();
}

Json Plant::writePiecesToJson() const {
  return jsonFromList<PlantPiece>(m_pieces, [](PlantPiece const& piece) -> Json {
      return JsonObject{
        {"image", piece.image},
        {"offset", jsonFromVec2F(piece.offset)},
        {"rotationType", RotationTypeNames.getRight(piece.rotationType)},
        {"rotationOffset", piece.rotationOffset},
        {"structuralSegment", piece.structuralSegment},
        {"kind", piece.kind},
        {"segmentIdx", piece.segmentIdx},
        {"flip", piece.flip},
      };
    });
}

void Plant::validatePieces() {
  for (auto& piece : m_pieces)
    piece.imagePath = piece.image;
  if (!m_piecesScanned) {
    scanSpacesAndRoots();
    calcBoundBox();
    m_piecesScanned = true;
  }
}

void Plant::setupNetStates() {
  m_netGroup.addNetElement(&m_tileDamageStatus);
  m_netGroup.addNetElement(&m_piecesNetState);
  m_netGroup.addNetElement(&m_tileDamageXNetState);
  m_netGroup.addNetElement(&m_tileDamageYNetState);
  m_netGroup.addNetElement(&m_tileDamageEventNetState);

  m_netGroup.setNeedsStoreCallback(bind(&Plant::setNetStates, this));
  m_netGroup.setNeedsLoadCallback(bind(&Plant::getNetStates, this));
}

void Plant::getNetStates() {
  if (m_piecesNetState.pullUpdated()) {
    readPieces(m_piecesNetState.get());
    m_piecesUpdated = true;
  }

  m_tileDamageX = m_tileDamageXNetState.get();
  m_tileDamageY = m_tileDamageYNetState.get();
  if (m_tileDamageEventNetState.pullOccurred()) {
    m_tileDamageEvent = true;
    m_tileDamageEventTrigger = true;
  }
}

void Plant::setNetStates() {
  if (m_piecesUpdated) {
    m_piecesNetState.set(writePieces());
    m_piecesUpdated = false;
  }
  m_tileDamageXNetState.set(m_tileDamageX);
  m_tileDamageYNetState.set(m_tileDamageY);
  if (m_tileDamageEventTrigger) {
    m_tileDamageEventTrigger = false;
    m_tileDamageEventNetState.trigger();
  }
}

bool Plant::damageTiles(List<Vec2I> const& positions, Vec2F const& sourcePosition, TileDamage const& tileDamage) {
  auto position = baseDamagePosition(positions);

  auto geometry = world()->geometry();

  m_tileDamageStatus.damage(m_tileDamageParameters, tileDamage);
  m_tileDamageX = geometry.diff(position[0], tilePosition()[0]);
  m_tileDamageY = position[1] - tilePosition()[1];
  m_tileDamageEvent = true;
  m_tileDamageEventTrigger = true;

  bool breaking = false;
  if (m_tileDamageStatus.dead()) {
    breaking = true;
    if (m_fallsWhenDead) {
      m_tileDamageStatus.reset();
      breakAtPosition(position, sourcePosition);
    } else {
      m_broken = true;
    }
  }

  return breaking;
}

void Plant::breakAtPosition(Vec2I const& position, Vec2F const& sourcePosition) {
  auto geometry = world()->geometry();
  Vec2I internalPos = geometry.diff(position, tilePosition());
  size_t idx = highest<size_t>();
  int segmentIdx = highest<int>();
  for (size_t i = 0; i < m_pieces.size(); ++i) {
    auto& piece = m_pieces[i];
    if (piece.structuralSegment && piece.spaces.contains(internalPos)) {
      if (piece.segmentIdx < segmentIdx) {
        segmentIdx = piece.segmentIdx;
        idx = i;
      }
    }
  }

  // default to highest structural piece
  if (idx >= m_pieces.size()) {
    for (size_t i = m_pieces.size(); i > 0; --i) {
      auto& piece = m_pieces[i - 1];
      if (piece.structuralSegment) {
        segmentIdx = piece.segmentIdx;
        idx = i - 1;
        break;
      }
    }
  }

  // plant has no structural segments? this is a terrible fallback because it
  // prevents destruction
  if (idx >= m_pieces.size())
    return;

  PlantPiece breakPiece = m_pieces[idx];
  Vec2F breakPoint = Vec2F(position) - Vec2F(tilePosition());
  if (breakPiece.spaces.size()) {
    RectF bounds = RectF::null();
    for (auto space : breakPiece.spaces) {
      bounds.combine(Vec2F(space));
      bounds.combine(Vec2F(space) + Vec2F(1, 1));
    }
    breakPoint[0] = (bounds.max()[0] + bounds.min()[0]) / 2.0f;
    if (!m_ceiling)
      breakPoint[1] = bounds.min()[1];
    else
      breakPoint[1] = bounds.max()[1];
  }

  List<PlantPiece> droppedPieces;
  if (m_pieces[idx].structuralSegment) {
    idx = 0;
    while (idx < m_pieces.size()) {
      if (m_pieces[idx].segmentIdx >= segmentIdx) {
        droppedPieces.append(m_pieces.takeAt(idx));
        continue;
      }
      idx++;
    }
  } else {
    droppedPieces.append(m_pieces.takeAt(idx));
  }
  m_piecesUpdated = true;

  Vec2I breakPointI = Vec2I(round(breakPoint[0]), round(breakPoint[1]));

  // Calculate a new origin for the droppedPieces
  for (auto& piece : droppedPieces) {
    piece.offset -= breakPoint;
    Set<Vec2I> spaces = piece.spaces;
    piece.spaces.clear();
    for (auto const& space : spaces) {
      piece.spaces.add(space - breakPointI);
    }
  }

  Vec2F worldSpaceBreakPoint = breakPoint + Vec2F(tilePosition());

  List<int> segmentOrder;
  Map<int, List<PlantPiece>> segments;
  for (auto& piece : droppedPieces) {
    if (!segments.contains(piece.segmentIdx))
      segmentOrder.append(piece.segmentIdx);
    segments[piece.segmentIdx].append(piece);
  }
  reverse(segmentOrder);
  float random = Random::randf(-0.3f, 0.3f);
  auto fallVector = (worldSpaceBreakPoint - sourcePosition).normalized();
  bool first = true;
  for (auto segmentIdx : segmentOrder) {
    auto segment = segments[segmentIdx];
    world()->addEntity(make_shared<PlantDrop>(segment,
        worldSpaceBreakPoint,
        fallVector,
        description(),
        m_ceiling,
        m_stemDropConfig,
        m_foliageDropConfig,
        m_saplingDropConfig,
        first,
        random));
    first = false;
  }

  m_piecesScanned = false;

  validatePieces();
}

Vec2I Plant::baseDamagePosition(List<Vec2I> const& positions) const {
  starAssert(positions.size());
  auto res = positions.at(0);

  for (auto const& piece : m_pieces) {
    if (piece.structuralSegment) {
      for (auto space : piece.spaces) {
        for (auto position : positions) {
          if (world()->geometry().equal(m_tilePosition + space, position)) {
            // if this space is a "better match" for the root of the plant
            if ((res[1] < position[1]) == m_ceiling) {
              res = position;
            }
          }
        }
      }
    }
  }

  return res;
}

bool Plant::damagable() const {
  if (m_stemDropConfig.type() != Json::Type::Object)
    return true;
  if (!m_stemDropConfig.getBool("destructable", true))
    return false;
  return true;
}

}
