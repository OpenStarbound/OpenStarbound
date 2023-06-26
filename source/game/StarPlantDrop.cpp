#include "StarPlantDrop.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarPlayer.hpp"
#include "StarRoot.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarItemDrop.hpp"
#include "StarAssets.hpp"
#include "StarEntityRendering.hpp"
#include "StarWorld.hpp"
#include "StarRandom.hpp"
#include "StarParticleDatabase.hpp"

namespace Star {

PlantDrop::PlantDropPiece::PlantDropPiece() {
  image = "";
  offset = {};
  segmentIdx = 0;
  kind = Plant::PlantPieceKind::None;
  flip = false;
}

PlantDrop::PlantDrop(List<Plant::PlantPiece> pieces, Vec2F const& position, Vec2F const& strikeVector, String const& description,
    bool upsideDown, Json stemConfig, Json foliageConfig, Json saplingConfig, bool master, float random) {
  m_netGroup.addNetElement(&m_movementController);
  m_netGroup.addNetElement(&m_spawnedDrops);

  m_stemConfig = stemConfig;
  if (m_stemConfig.isNull())
    m_stemConfig = JsonObject();
  m_foliageConfig = foliageConfig;
  if (m_foliageConfig.isNull())
    m_foliageConfig = JsonObject();
  m_saplingConfig = saplingConfig;

  m_master = master;
  m_firstTick = true;
  m_spawnedDrops.set(false);
  m_spawnedDropEffects = false;
  m_time = 5000;
  m_description = description;
  m_movementController.setPosition(position);
  if (!upsideDown) {
    m_rotationRate = copysign(0.00001f, -strikeVector[0] + random);
    m_rotationFallThreshold = Constants::pi / (3 + random);
    m_rotationCap = Constants::pi - m_rotationFallThreshold;
  } else {
    m_rotationRate = 0;
    m_rotationFallThreshold = 0;
    m_rotationCap = 0;
  }

  bool structuralFound = false;
  RectF stemBounds = RectF::null();
  RectF fullBounds = RectF::null();

  // note structuralSegment is only available in the constructor
  for (auto& piece : pieces) {
    for (auto& pos : piece.spaces) {
      fullBounds.combine(Vec2F(pos));
      fullBounds.combine(Vec2F(pos) + Vec2F(1, 1));
      if (piece.structuralSegment) {
        structuralFound = true;
        stemBounds.combine(Vec2F(pos));
        stemBounds.combine(Vec2F(pos) + Vec2F(1, 1));
      }
    }
    PlantDropPiece pdp;
    pdp.image = piece.image;
    pdp.offset = piece.offset;
    pdp.segmentIdx = piece.segmentIdx;
    pdp.kind = piece.kind;
    pdp.flip = piece.flip;
    m_pieces.append(pdp);
  }

  if (fullBounds.isNull())
    fullBounds = RectF(position, position);
  if (stemBounds.isNull())
    stemBounds = RectF(position, position);

  m_boundingBox = fullBounds;
  if (structuralFound)
    m_collisionRect = stemBounds;
  else
    m_collisionRect = fullBounds;
}

PlantDrop::PlantDrop(ByteArray const& netStore) {
  m_netGroup.addNetElement(&m_movementController);
  m_netGroup.addNetElement(&m_spawnedDrops);

  DataStreamBuffer ds(netStore);
  ds >> m_time;
  ds >> m_master;
  ds >> m_description;
  ds >> m_boundingBox;
  ds >> m_collisionRect;
  ds >> m_rotationRate;
  ds.readContainer(m_pieces,
      [](DataStream& ds, PlantDropPiece& piece) {
        ds.read(piece.image);
        ds.read(piece.offset[0]);
        ds.read(piece.offset[1]);
        ds.read(piece.flip);
        ds.read(piece.kind);
      });
  ds >> m_stemConfig;
  ds >> m_foliageConfig;
  ds >> m_saplingConfig;

  m_firstTick = true;
  m_spawnedDropEffects = true;
}

ByteArray PlantDrop::netStore() {
  DataStreamBuffer ds;
  ds << m_time;
  ds << m_master;
  ds << m_description;
  ds << m_boundingBox;
  ds << m_collisionRect;
  ds << m_rotationRate;
  ds.writeContainer(m_pieces,
      [](DataStream& ds, PlantDropPiece const& piece) {
        ds.write(piece.image);
        ds.write(piece.offset[0]);
        ds.write(piece.offset[1]);
        ds.write(piece.flip);
        ds.write(piece.kind);
      });
  ds << m_stemConfig;
  ds << m_foliageConfig;
  ds << m_saplingConfig;

  return ds.data();
}

EntityType PlantDrop::entityType() const {
  return EntityType::PlantDrop;
}

void PlantDrop::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);
  m_movementController.init(world);

  PolyF collisionPoly = PolyF(RectF::withCenter(m_collisionRect.center(), m_collisionRect.size() / 2.0f));
  MovementParameters parameters;
  parameters.collisionPoly = collisionPoly;
  parameters.ignorePlatformCollision = true;
  parameters.gravityMultiplier = 0.2f;
  parameters.physicsEffectCategories = StringSet({"plantdrop"});
  m_movementController.applyParameters(parameters);
}

void PlantDrop::uninit() {
  Entity::uninit();
  m_movementController.uninit();
}

String PlantDrop::description() const {
  return m_description;
}

Vec2F PlantDrop::position() const {
  return m_movementController.position();
}

RectF PlantDrop::metaBoundBox() const {
  return m_boundingBox;
}

RectF PlantDrop::collisionRect() const {
  PolyF shape = PolyF(m_collisionRect);
  shape.rotate(m_movementController.rotation());
  return shape.boundBox();
}

void PlantDrop::update(uint64_t) {
  m_time -= WorldTimestep;

  if (isMaster()) {
    if (m_spawnedDropEffects && !m_spawnedDrops.get())
      m_spawnedDropEffects = false; // false positive assumption over already having done the effect
    // to avoid effects for newly joining players.
    if (m_spawnedDrops.get())
      m_firstTick = false;

    // think up a better curve then sin
    auto rotationAcceleration = 0.01f * world()->gravity(position()) * copysign(1.0f, m_rotationRate) * WorldTimestep;
    if (abs(m_movementController.rotation()) > m_rotationCap)
      m_rotationRate -= rotationAcceleration;
    else if (std::fabs(m_movementController.rotation()) < m_rotationFallThreshold)
      m_rotationRate += rotationAcceleration;

    m_movementController.rotate(m_rotationRate);

    PolyF collisionPoly = PolyF(RectF::withCenter(m_collisionRect.center(), m_collisionRect.size() / 2.0f));

    if (m_time > 0) {
      MovementParameters parameters;
      parameters.collisionPoly = collisionPoly;
      parameters.gravityEnabled = std::fabs(m_movementController.rotation()) >= m_rotationFallThreshold;
      m_movementController.applyParameters(parameters);

      m_movementController.tickMaster();
      if (m_movementController.onGround())
        m_time = 0;
    }

    auto imgMetadata = Root::singleton().imageMetadataDatabase();

    if ((m_time <= 0 || world()->gravity(position()) == 0) && !m_spawnedDrops.get()) {
      m_spawnedDrops.set(true);
      for (auto& plantPiece : m_pieces) {
        JsonArray dropOptions;
        if (plantPiece.kind == Plant::PlantPieceKind::Stem)
          dropOptions = m_stemConfig.getArray("drops", {});
        if (plantPiece.kind == Plant::PlantPieceKind::Foliage)
          dropOptions = m_foliageConfig.getArray("drops", {});
        if (dropOptions.size()) {
          auto option = Random::randFrom(dropOptions).toArray();
          for (auto drop : option) {
            auto size = imgMetadata->imageSize(plantPiece.image);
            Vec2F pos = Vec2F(plantPiece.offset + Vec2F(size) * .5f / TilePixels).rotate(m_movementController.rotation())
                + Vec2F(Random::randf(-0.2f, 0.2f), Random::randf(-0.2f, 0.2f));
            if (drop.getString("item") == "sapling")
              world()->addEntity(ItemDrop::createRandomizedDrop(
                  ItemDescriptor("sapling", (size_t)drop.getInt("count", 1), m_saplingConfig), position() + pos));
            else
              world()->addEntity(ItemDrop::createRandomizedDrop(
                  {drop.getString("item"), (size_t)drop.getInt("count", 1)}, position() + pos));
          }
        }
      }
    }
  } else {
    m_netGroup.tickNetInterpolation(WorldTimestep);

    if (m_spawnedDropEffects && !m_spawnedDrops.get())
      m_spawnedDropEffects = false; // false positive assumption over already having done the effect
    // to avoid effects for newly joining players.
    if (m_spawnedDrops.get())
      m_firstTick = false;

    m_movementController.tickSlave();
  }
}

void PlantDrop::destroy(RenderCallback* renderCallback) {
  if (renderCallback)
    render(renderCallback);
}

void PlantDrop::particleForPlantPart(PlantDropPiece const& piece, String const& mode, Json const& mainConfig, RenderCallback* renderCallback) {
  Json particleConfig = mainConfig.get("particles", JsonObject()).get(mode, JsonObject());
  JsonArray particleOptions = particleConfig.getArray("options", {});
  if (!particleOptions.size())
    return;

  Particle particle;

  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  Vec2F imageSize = Vec2F(imgMetadata->imageSize(piece.image)) / TilePixels;
  float density = (imageSize.x() * imageSize.y()) / particleConfig.getFloat("density", 1);

  auto spaces = Set<Vec2I>::from(imgMetadata->imageSpaces(piece.image, piece.offset * TilePixels, Plant::PlantScanThreshold, piece.flip));
  if (spaces.empty())
    return;

  while (density > 0) {
    Vec2F particlePos = piece.offset + Vec2F(imageSize) / 2.0f + Vec2F(Random::nrandf(imageSize.x() / 8.0f, 0), Random::nrandf(imageSize.y() / 8.0f, 0));

    if (!spaces.contains(Vec2I(particlePos.floor())))
      continue;

    auto config = Random::randValueFrom(particleOptions, {});

    particle = Root::singleton().particleDatabase()->particle(config);
    particle.color.hueShift(mainConfig.getFloat("hueshift", 0) / 360.0f);
    for (Directives const& directives : piece.image.directives.list())
      particle.directives.append(directives);

    density--;

    particle.position = position() + particlePos.rotate(m_movementController.rotation());

    renderCallback->addParticle(particle);
  }
}

void PlantDrop::render(RenderCallback* renderCallback) {
  auto assets = Root::singleton().assets();

  if (m_firstTick) {
    m_firstTick = false;
    // smoke, particles

    if (m_master) {
      auto playBreakSound = [&](Json const& config) {
        JsonArray breakTreeOptions = config.get("sounds", JsonObject()).getArray("breakTree", JsonArray());
        if (breakTreeOptions.size()) {
          auto sound = Random::randFrom(breakTreeOptions);
          auto audioInstance = make_shared<AudioInstance>(*assets->audio(sound.getString("file")));
          audioInstance->setPosition(collisionRect().center() + position());
          audioInstance->setVolume(sound.getFloat("volume", 1.0f));
          renderCallback->addAudio(move(audioInstance));
        }
      };
      playBreakSound(m_stemConfig);
      playBreakSound(m_foliageConfig);
    }

    for (auto const& plantPiece : m_pieces) {
      if (plantPiece.kind == Plant::PlantPieceKind::Stem)
        particleForPlantPart(plantPiece, "breakTree", m_stemConfig, renderCallback);
      if (plantPiece.kind == Plant::PlantPieceKind::Foliage)
        particleForPlantPart(plantPiece, "breakTree", m_foliageConfig, renderCallback);
    }
  }

  if (m_spawnedDrops.get() && !m_spawnedDropEffects) {
    m_spawnedDropEffects = true;
    // smoke, particles

    auto playHitSound = [&](Json const& config) {
      JsonArray hitGroundOptions = config.get("sounds", JsonObject()).getArray("hitGround", JsonArray());
      if (hitGroundOptions.size()) {
        auto sound = Random::randFrom(hitGroundOptions);
        auto audioInstance = make_shared<AudioInstance>(*assets->audio(sound.getString("file")));
        audioInstance->setPosition(collisionRect().center() + position());
        audioInstance->setVolume(sound.getFloat("volume", 1.0f));
        renderCallback->addAudio(move(audioInstance));
      }
    };
    playHitSound(m_stemConfig);
    playHitSound(m_foliageConfig);

    for (auto const& plantPiece : m_pieces) {
      if (plantPiece.kind == Plant::PlantPieceKind::Stem)
        particleForPlantPart(plantPiece, "hitGround", m_stemConfig, renderCallback);
      if (plantPiece.kind == Plant::PlantPieceKind::Foliage)
        particleForPlantPart(plantPiece, "hitGround", m_foliageConfig, renderCallback);
    }
  }

  if (m_time > 0 && !m_spawnedDrops.get()) {
    for (auto const& plantPiece : m_pieces) {
      auto drawable = Drawable::makeImage(plantPiece.image, 1.0f / TilePixels, false, plantPiece.offset);
      if (plantPiece.flip)
        drawable.scale(Vec2F(-1, 1));
      drawable.rotate(m_movementController.rotation());
      drawable.translate(position());
      renderCallback->addDrawable(move(drawable), RenderLayerPlantDrop);
    }
  }
}

pair<ByteArray, uint64_t> PlantDrop::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void PlantDrop::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(move(data), interpolationTime);
}

void PlantDrop::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void PlantDrop::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

bool PlantDrop::shouldDestroy() const {
  return m_time <= 0.0f;
}

}
