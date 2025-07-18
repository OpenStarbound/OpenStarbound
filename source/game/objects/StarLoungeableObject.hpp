#pragma once

#include "StarObject.hpp"
#include "StarLoungingEntities.hpp"

namespace Star {

class LoungeableObject : public Object, public virtual LoungeableEntity {
public:
  LoungeableObject(ObjectConfigConstPtr config, Json const& parameters = Json());

  void render(RenderCallback* renderCallback) override;

  InteractAction interact(InteractRequest const& request) override;

  size_t anchorCount() const override;
  LoungeAnchorConstPtr loungeAnchor(size_t positionIndex) const override;

  virtual void init(World* world, EntityId entityId, EntityMode mode) override;
  virtual void uninit() override;
  virtual void update(float dt, uint64_t currentStep) override;

  virtual void loungeControl(size_t anchorPositionIndex, LoungeControl loungeControl) override;
  virtual void loungeAim(size_t anchorPositionIndex, Vec2F const& aimPosition) override;

  virtual EntityRenderLayer loungeRenderLayer(size_t anchorPositionIndex) const override;
  virtual NetworkedAnimator const* networkedAnimator() const override;

  virtual Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args = {}) override;

  virtual LoungeableEntity::LoungePositions* loungePositions() override;
  virtual LoungeableEntity::LoungePositions const* loungePositions() const override;

protected:
  virtual LuaCallbacks makeObjectCallbacks() override;

  void setOrientationIndex(size_t orientationIndex) override;

private:
  List<Vec2F> m_sitPositions;
  bool m_sitFlipDirection;
  LoungeOrientation m_sitOrientation;
  float m_sitAngle;
  String m_sitCoverImage;
  bool m_flipImages;
  List<PersistentStatusEffect> m_sitStatusEffects;
  StringSet m_sitEffectEmitters;
  Maybe<String> m_sitEmote;
  Maybe<String> m_sitDance;
  JsonObject m_sitArmorCosmeticOverrides;
  Maybe<String> m_sitCursorOverride;

  bool m_useLoungePositions = false;
  LoungePositions m_loungePositions;
};

}
