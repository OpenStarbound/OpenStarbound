#ifndef STAR_TECH_CONTROLLER_HPP
#define STAR_TECH_CONTROLLER_HPP

#include "StarNetElementSystem.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaActorMovementComponent.hpp"
#include "StarTechDatabase.hpp"
#include "StarDirectives.hpp"

namespace Star {

STAR_CLASS(TechController);
STAR_CLASS(StatusController);

// Class that acts as a movement controller for the parent entity that supports
// a variety scriptable "Tech" that the entity can use that affect movement,
// physics, sounds, particles, damage regions, etc.  Network capable, and all
// flags are sensibly set on both the client and server.
class TechController : public NetElementGroup {
public:
  enum class ParentState {
    Stand,
    Fly,
    Fall,
    Sit,
    Lay,
    Duck,
    Walk,
    Run,
    Swim,
    SwimIdle
  };
  static EnumMap<ParentState> const ParentStateNames;

  TechController();

  Json diskStore();
  void diskLoad(Json const& store);

  void init(Entity* parentEntity, ActorMovementController* movementController, StatusController* statusController);
  void uninit();

  void setLoadedTech(StringList const& techModules, bool forceLoad = false);
  StringList loadedTech() const;
  void reloadTech();

  bool techOverridden() const;
  void setOverrideTech(StringList const& techModules);
  void clearOverrideTech();

  void setShouldRun(bool shouldRun);

  void beginPrimaryFire();
  void beginAltFire();
  void endPrimaryFire();
  void endAltFire();

  void moveUp();
  void moveDown();
  void moveLeft();
  void moveRight();
  void jump();
  void special(int specialKey);

  void setAimPosition(Vec2F const& aimPosition);

  void tickMaster(float dt);
  void tickSlave(float dt);

  Maybe<ParentState> parentState() const;
  DirectivesGroup const& parentDirectives() const;
  Vec2F parentOffset() const;
  bool toolUsageSuppressed() const;

  bool parentHidden() const;

  List<Drawable> backDrawables();
  List<Drawable> frontDrawables();

  List<LightSource> lightSources() const;

  List<AudioInstancePtr> pullNewAudios();
  List<Particle> pullNewParticles();

  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args = {});

private:
  struct TechAnimator : public NetElement {
    TechAnimator(Maybe<String> animationConfig = {});

    void initNetVersion(NetElementVersion const* version = nullptr) override;

    void netStore(DataStream& ds) const override;
    void netLoad(DataStream& ds) override;

    void enableNetInterpolation(float extrapolationHint = 0.0f) override;
    void disableNetInterpolation() override;
    void tickNetInterpolation(float dt) override;

    bool writeNetDelta(DataStream& ds, uint64_t fromVersion) const override;
    void readNetDelta(DataStream& ds, float interpolationTime = 0.0) override;
    void blankNetDelta(float interpolationTime) override;

    // If setting invisible, stops all playing audio
    void setVisible(bool visible);
    bool isVisible() const;

    Maybe<String> animationConfig;
    NetworkedAnimator animator;
    NetworkedAnimator::DynamicTarget dynamicTarget;
    NetElementBool visible;
    NetElementGroup netGroup;
  };

  typedef NetElementDynamicGroup<TechAnimator> TechAnimatorGroup;

  struct TechModule {
    TechConfig config;

    LuaMessageHandlingComponent<LuaStorableComponent<LuaActorMovementComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>>>
        scriptComponent;
    bool visible;
    bool toolUsageSuppressed;
    Directives parentDirectives;
    TechAnimatorGroup::ElementId animatorId;
  };

  // Name of module, any existing module script data.
  void setupTechModules(List<tuple<String, JsonObject>> const& moduleInits);

  void unloadModule(TechModule& techModule);

  void initializeModules();

  void resetMoves();
  void updateAnimators(float dt);

  LuaCallbacks makeTechCallbacks(TechModule& techModule);

  Maybe<StringList> m_overriddenTech;
  LinkedList<TechModule> m_techModules;
  TechAnimatorGroup m_techAnimators;

  Entity* m_parentEntity;
  ActorMovementController* m_movementController;
  StatusController* m_statusController;

  bool m_moveRun;
  bool m_movePrimaryFire;
  bool m_moveAltFire;
  bool m_moveUp;
  bool m_moveDown;
  bool m_moveLeft;
  bool m_moveRight;
  bool m_moveJump;
  bool m_moveSpecial1;
  bool m_moveSpecial2;
  bool m_moveSpecial3;

  Vec2F m_aimPosition;

  NetElementData<Maybe<ParentState>> m_parentState;
  NetElementData<DirectivesGroup> m_parentDirectives;
  NetElementFloat m_xParentOffset;
  NetElementFloat m_yParentOffset;
  NetElementBool m_parentHidden;
  NetElementBool m_toolUsageSuppressed;
};

}

#endif
