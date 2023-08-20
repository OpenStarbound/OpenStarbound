#ifndef STAR_ITEM_DROP_HPP
#define STAR_ITEM_DROP_HPP

#include "StarNetElementSystem.hpp"
#include "StarMovementController.hpp"
#include "StarItemDescriptor.hpp"
#include "StarGameTimers.hpp"
#include "StarEntity.hpp"
#include "StarDrawable.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(ItemDrop);

class ItemDrop : public virtual Entity {
public:
  // Creates a drop at the given position and adds a hard-coded amount of
  // randomness to the drop position / velocity.
  static ItemDropPtr createRandomizedDrop(ItemPtr const& item, Vec2F const& position, bool eternal = false);
  static ItemDropPtr createRandomizedDrop(ItemDescriptor const& itemDescriptor, Vec2F const& position, bool eternal = false);

  // Create a drop and throw in the given direction with a hard-coded initial
  // throw velocity (unrelated to magnitude of direction, direction is
  // normalized first).  Initially intangible for 1 second.
  static ItemDropPtr throwDrop(ItemPtr const& item, Vec2F const& position, Vec2F const& velocity, Vec2F const& direction, bool eternal = false);
  static ItemDropPtr throwDrop(ItemDescriptor const& itemDescriptor, Vec2F const& position, Vec2F const& velocity, Vec2F const& direction, bool eternal = false);

  ItemDrop(ItemPtr item);
  ItemDrop(Json const& diskStore);
  ItemDrop(ByteArray netStore);

  Json diskStore() const;
  ByteArray netStore() const;

  EntityType entityType() const override;

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  String description() const override;

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0) override;
  void readNetState(ByteArray data, float interpolationTime = 0.0f) override;

  void enableInterpolation(float extrapolationHint = 0.0f) override;
  void disableInterpolation() override;

  Vec2F position() const override;
  RectF metaBoundBox() const override;

  bool ephemeral() const override;

  RectF collisionArea() const override;

  void update(float dt, uint64_t currentStep) override;

  bool shouldDestroy() const override;

  virtual void render(RenderCallback* renderCallback) override;
  virtual void renderLightSources(RenderCallback* renderCallback) override;
  // The item that this drop contains
  ItemPtr item() const;

  void setEternal(bool eternal);

  // If intangibleTime is set, will be intangible and unable to be picked up
  // until that amount of time has passed.
  void setIntangibleTime(float intangibleTime);

  // Mark this drop as taken by the given entity.  The drop will animate
  // towards them for a while and then disappear.
  ItemPtr takeBy(EntityId entityId, float timeOffset = 0.0f);

  // Mark this drop as taken, but do not animate it towards a player simply
  // disappear next step.
  ItemPtr take();

  // Item is not taken and is not intangible
  bool canTake() const;

  void setPosition(Vec2F const& position);

  Vec2F velocity() const;
  void setVelocity(Vec2F const& position);

private:
  enum class Mode { Intangible, Available, Taken, Dead };
  static EnumMap<Mode> const ModeNames;

  ItemDrop();

  // Set the movement controller's collision poly to match the
  // item drop drawables
  void updateCollisionPoly();

  void updateTaken(bool master);

  Json m_config;
  ItemPtr m_item;
  RectF m_boundBox;
  float m_afterTakenLife;
  float m_overheadTime;
  float m_pickupDistance;
  float m_velocity;
  float m_velocityApproach;
  float m_overheadApproach;
  Vec2F m_overheadOffset;

  float m_combineChance;
  float m_combineRadius;
  double m_ageItemsEvery;

  NetElementTopGroup m_netGroup;
  NetElementEnum<Mode> m_mode;
  NetElementIntegral<EntityId> m_owningEntity;
  NetElementData<ItemDescriptor> m_itemDescriptor;
  MovementController m_movementController;

  // Only updated on master
  bool m_eternal;
  EpochTimer m_dropAge;
  GameTimer m_intangibleTimer;
  EpochTimer m_ageItemsTimer;

  Maybe<List<Drawable>> m_drawables;
};

}

#endif
