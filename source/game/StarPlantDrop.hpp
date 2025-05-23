#pragma once

#include "StarNetElementSystem.hpp"
#include "StarMovementController.hpp"
#include "StarPlant.hpp"
#include "StarAssetPath.hpp"
#include "StarMobileEntity.hpp"

namespace Star {

STAR_CLASS(RenderCallback);
STAR_CLASS(PlantDrop);

class PlantDrop :
  public virtual MobileEntity,
  public virtual Entity {
public:
  PlantDrop(List<Plant::PlantPiece> pieces, Vec2F const& position, Vec2F const& strikeVector, String const& description,
      bool upsideDown, Json stemConfig, Json foliageConfig, Json saplingConfig,
      bool master, float random);
  PlantDrop(ByteArray const& netStore, NetCompatibilityRules rules = {});

  ByteArray netStore(NetCompatibilityRules rules = {});

  EntityType entityType() const override;

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  String description() const override;

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0, NetCompatibilityRules rules = {}) override;
  void readNetState(ByteArray data, float interpolationTime = 0.0f, NetCompatibilityRules rules = {}) override;

  void enableInterpolation(float extrapolationHint = 0.0f) override;
  void disableInterpolation() override;

  Vec2F position() const override;
  RectF metaBoundBox() const override;

  bool shouldDestroy() const override;
  void destroy(RenderCallback* renderCallback) override;

  void setPosition();
  void setVelocity(Vec2F const& position);

  RectF collisionRect() const;

  void update(float dt, uint64_t currentStep) override;

  void render(RenderCallback* renderCallback) override;

  MovementController* movementController() override;

private:
  struct PlantDropPiece {
    PlantDropPiece();
    AssetPath image;
    Vec2F offset;
    int segmentIdx;
    Plant::PlantPieceKind kind;
    bool flip;
  };

  void particleForPlantPart(PlantDropPiece const& piece, String const& mode, Json const& mainConfig, RenderCallback* renderCallback);

  NetElementTopGroup m_netGroup;
  String m_description;
  float m_time;
  MovementController m_movementController;
  RectF m_collisionRect;
  RectF m_boundingBox;
  float m_rotationRate;
  float m_rotationFallThreshold;
  float m_rotationCap;
  List<PlantDropPiece> m_pieces;
  Json m_stemConfig;
  Json m_foliageConfig;
  Json m_saplingConfig;
  bool m_master;
  bool m_firstTick;
  NetElementBool m_spawnedDrops;
  bool m_spawnedDropEffects;
};

}
