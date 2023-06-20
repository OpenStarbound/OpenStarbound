#ifndef STAR_TOOL_USER_HPP
#define STAR_TOOL_USER_HPP

#include "StarHumanoid.hpp"
#include "StarNetElementSystem.hpp"
#include "StarItemDescriptor.hpp"
#include "StarStatusTypes.hpp"
#include "StarLightSource.hpp"
#include "StarDamage.hpp"
#include "StarEffectEmitter.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarPhysicsEntity.hpp"

namespace Star {

STAR_CLASS(ObjectItem);
STAR_CLASS(ToolUserEntity);
STAR_CLASS(Item);
STAR_CLASS(World);

STAR_CLASS(ToolUser);

class ToolUser : public NetElementSyncGroup {
public:
  ToolUser();

  Json diskStore() const;
  void diskLoad(Json const& diskStore);

  void init(ToolUserEntity* user);
  void uninit();

  ItemPtr primaryHandItem() const;
  ItemPtr altHandItem() const;
  ItemDescriptor primaryHandItemDescriptor() const;
  ItemDescriptor altHandItemDescriptor() const;

  List<LightSource> lightSources() const;
  void effects(EffectEmitter& emitter) const;
  List<PersistentStatusEffect> statusEffects() const;

  Maybe<float> toolRadius() const;
  // FIXME: There is a render method in ToolUser, why can't this be rendered
  // with the rest of everything else, there are TILE previews and OBJECT
  // previews, but of course one has to go through the render method and the
  // other has to be rendered separately.
  List<Drawable> renderObjectPreviews(Vec2F aimPosition, Direction walkingDirection, bool inToolRange, Vec4B favoriteColor) const;
  // Returns the facing override direciton if there is one
  Maybe<Direction> setupHumanoidHandItems(Humanoid& humanoid, Vec2F position, Vec2F aimPosition) const;
  void setupHumanoidHandItemDrawables(Humanoid& humanoid) const;

  Vec2F armPosition(Humanoid const& humanoid, ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset) const;
  Vec2F handOffset(Humanoid const& humanoid, ToolHand hand, Direction facingDirection) const;
  Vec2F handPosition(ToolHand hand, Humanoid const& humanoid, Vec2F const& handOffset) const;
  bool queryShieldHit(DamageSource const& source) const;

  void tick(bool shifting, HashSet<MoveControlType> const& moves);

  void beginPrimaryFire();
  void beginAltFire();
  void endPrimaryFire();
  void endAltFire();

  bool firingPrimary() const;
  bool firingAlt() const;

  List<DamageSource> damageSources() const;
  List<PhysicsForceRegion> forceRegions() const;

  void render(RenderCallback* renderCallback, bool inToolRange, bool shifting, EntityRenderLayer renderLayer);

  void setItems(ItemPtr primaryHandItem, ItemPtr altHandItem);

  void suppressItems(bool suppress);

  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args = {});

  float beamGunRadius() const;

private:
  class NetItem : public NetElement {
  public:
    void initNetVersion(NetElementVersion const* version = nullptr) override;

    void netStore(DataStream& ds) const override;
    void netLoad(DataStream& ds) override;

    void enableNetInterpolation(float extrapolationHint = 0.0f) override;
    void disableNetInterpolation() override;
    void tickNetInterpolation(float dt) override;

    bool writeNetDelta(DataStream& ds, uint64_t fromVersion) const override;
    void readNetDelta(DataStream& ds, float interpolationTime = 0.0) override;
    void blankNetDelta(float interpolationTime) override;

    ItemPtr const& get() const;
    void set(ItemPtr item);

    bool pullNewItem();

  private:
    void updateItemDescriptor();

    NetElementData<ItemDescriptor> m_itemDescriptor;
    ItemPtr m_item;
    NetElementVersion const* m_netVersion = nullptr;
    bool m_netInterpolationEnabled = false;
    float m_netExtrapolationHint = 0;
    bool m_newItem = false;
    mutable DataStreamBuffer m_buffer;
  };

  void initPrimaryHandItem();
  void initAltHandItem();
  void uninitItem(ItemPtr const& item);

  void netElementsNeedLoad(bool full) override;
  void netElementsNeedStore() override;

  float m_beamGunRadius;
  unsigned m_beamGunGlowBorder;
  float m_objectPreviewInnerAlpha;
  float m_objectPreviewOuterAlpha;

  ToolUserEntity* m_user;

  NetItem m_primaryHandItem;
  NetItem m_altHandItem;

  bool m_fireMain;
  bool m_fireAlt;
  bool m_edgeTriggeredMain;
  bool m_edgeTriggeredAlt;
  bool m_edgeSuppressedMain;
  bool m_edgeSuppressedAlt;

  NetElementBool m_suppress;

  NetElementFloat m_primaryFireTimerNetState;
  NetElementFloat m_altFireTimerNetState;
  NetElementFloat m_primaryTimeFiringNetState;
  NetElementFloat m_altTimeFiringNetState;
  NetElementBool m_primaryItemActiveNetState;
  NetElementBool m_altItemActiveNetState;
};

}

#endif
