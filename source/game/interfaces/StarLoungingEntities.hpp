#pragma once

#include "StarDrawable.hpp"
#include "StarAnchorableEntity.hpp"
#include "StarStatusTypes.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarGameTimers.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarLua.hpp"
#include "StarInteractionTypes.hpp"

namespace Star {

STAR_CLASS(World);

STAR_STRUCT(LoungeAnchor);
STAR_CLASS(LoungeableEntity);
STAR_CLASS(LoungingEntity);

enum class LoungeOrientation { None, Sit, Lay, Stand };
extern EnumMap<LoungeOrientation> const LoungeOrientationNames;

enum class LoungeControl { Left, Right, Down, Up, Jump, PrimaryFire, AltFire, Special1, Special2, Special3, Walk };
extern EnumMap<LoungeControl> const LoungeControlNames;

struct LoungeAnchor : EntityAnchor {
  LoungeOrientation orientation;
  EntityRenderLayer loungeRenderLayer;
  bool controllable;
  List<PersistentStatusEffect> statusEffects;
  StringSet effectEmitters;
  Maybe<String> emote;
  Maybe<String> dance;
  Maybe<Directives> directives;
  JsonObject armorCosmeticOverrides;
  Maybe<String> cursorOverride;
  Maybe<bool> suppressTools;
  bool cameraFocus;
  bool usePartZLevel = false;
  bool hidden = false;
};

// Extends an AnchorableEntity to have more specific effects when anchoring,
// such as status effects and lounge controls.  All LoungeableEntity methods
// may be called on both the master and slave.
class LoungeableEntity : public AnchorableEntity {
public:
  virtual size_t anchorCount() const override;
  EntityAnchorConstPtr anchor(size_t anchorPositionIndex) const override;
  virtual LoungeAnchorConstPtr loungeAnchor(size_t anchorPositionIndex) const;

  virtual void loungeControl(size_t anchorPositionIndex, LoungeControl loungeControl);
  virtual void loungeAim(size_t anchorPositionIndex, Vec2F const& aimPosition);

  // Queries around this entity's metaBoundBox for any LoungingEntities
  // reporting that they are lounging in this entity, and returns ones that are
  // lounging in the given position.
  Set<EntityId> entitiesLoungingIn(size_t anchorPositionIndex) const;
  // Returns pairs of entity ids, and the position they are lounging in.
  Set<pair<EntityId, size_t>> entitiesLounging() const;

  void setupLoungePositions(float timeout, float heartbeat, JsonObject positions, bool extraControls);
  void setupLoungeNetStates(NetElementTopGroup* netGroup, uint8_t minimumVersion);
  void loungeInit();
  void loungeTickMaster(float dt);
  void loungeTickSlave(float dt);

  Maybe<Json> receiveLoungeMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args);
  Maybe<size_t> loungeInteract(InteractRequest const& request);
  LuaCallbacks addLoungeableCallbacks(LuaCallbacks callbacks);

  virtual void setupLoungingDrawables();

  virtual EntityRenderLayer loungeRenderLayer(size_t anchorPositionIndex) const = 0;
  virtual NetworkedAnimator const* networkedAnimator() const = 0;
  virtual NetworkedAnimator * networkedAnimator() = 0;

  struct MasterControlState {
    Set<ConnectionId> slavesHeld;
    bool masterHeld;
  };

  struct LoungePositionConfig {
    LoungePositionConfig(Json config);
    void setupNetStates(NetElementTopGroup* netGroup, uint8_t minimumVersion);

    // The NetworkedAnimator part and part property which should control the
    // lounge position.
    String part;
    String partAnchor;
    Maybe<Vec2F> exitBottomOffset;
    JsonObject armorCosmeticOverrides;
    Maybe<String> cursorOverride;
    Maybe<bool> suppressTools;
    bool cameraFocus;
    bool usePartZLevel;

    NetElementBool enabled;
    NetElementEnum<LoungeOrientation> orientation;
    NetElementData<Maybe<String>> emote;
    NetElementData<Maybe<String>> dance;
    NetElementData<Maybe<String>> directives;
    NetElementData<List<PersistentStatusEffect>> statusEffects;
    NetElementBool hidden;

    Map<LoungeControl, MasterControlState> masterControlState;
    Vec2F masterAimPosition;

    Set<LoungeControl> slaveOldControls;
    Vec2F slaveOldAimPosition;
    Set<LoungeControl> slaveNewControls;
    Vec2F slaveNewAimPosition;
  };
  typedef OrderedHashMap<String, LoungePositionConfig> LoungePositions;
  virtual LoungeableEntity::LoungePositions* loungePositions() = 0;
  virtual LoungeableEntity::LoungePositions const* loungePositions() const = 0;

private:
  float m_slaveControlTimeout = 0.0f;
  bool m_receiveExtraControls;

  Map<ConnectionId, GameTimer> m_aliveMasterConnections;
  GameTimer m_slaveHeartbeatTimer;

};

// Any lounging entity should report the entity it is lounging in on both
// master and slave, so that lounging entities can cooperate and avoid lounging
// in the same spot.
class LoungingEntity : public virtual Entity {
public:
  virtual List<Drawable> drawables(Vec2F position = Vec2F()) = 0;

  virtual Maybe<EntityAnchorState> loungingIn() const = 0;
  // Returns true if the entity is in a lounge achor, but other entities are
  // also reporting being in that lounge anchor.
  bool inConflictingLoungeAnchor() const;
};

}
