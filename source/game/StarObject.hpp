#ifndef STAR_OBJECT_HPP
#define STAR_OBJECT_HPP

#include "StarPeriodic.hpp"
#include "StarPeriodicFunction.hpp"
#include "StarNetElementSystem.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaAnimationComponent.hpp"
#include "StarTileEntity.hpp"
#include "StarStatusEffectEntity.hpp"
#include "StarSet.hpp"
#include "StarColor.hpp"
#include "StarScriptedEntity.hpp"
#include "StarChattyEntity.hpp"
#include "StarWireEntity.hpp"
#include "StarInspectableEntity.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarDamageTypes.hpp"
#include "StarEntityRendering.hpp"

namespace Star {

STAR_CLASS(AudioInstance);
STAR_CLASS(ObjectDatabase);
STAR_STRUCT(ObjectConfig);
STAR_STRUCT(ObjectOrientation);
STAR_CLASS(Object);

class Object
  : public virtual TileEntity,
    public virtual StatusEffectEntity,
    public virtual ScriptedEntity,
    public virtual ChattyEntity,
    public virtual InspectableEntity,
    public virtual WireEntity {
public:

  Object(ObjectConfigConstPtr config, Json const& parameters = JsonObject());

  Json diskStore() const;
  ByteArray netStore();

  virtual EntityType entityType() const override;

  virtual void init(World* world, EntityId entityId, EntityMode mode) override;
  virtual void uninit() override;

  virtual Vec2F position() const override;
  virtual RectF metaBoundBox() const override;

  virtual pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0) override;
  virtual void readNetState(ByteArray data, float interpolationTime = 0.0f) override;

  virtual String description() const override;

  virtual bool inspectable() const override;
  virtual Maybe<String> inspectionLogName() const override;
  virtual Maybe<String> inspectionDescription(String const& species) const override;

  virtual List<LightSource> lightSources() const override;

  virtual bool shouldDestroy() const override;
  virtual void destroy(RenderCallback* renderCallback) override;

  virtual void update(float dt, uint64_t currentStep) override;

  virtual void render(RenderCallback* renderCallback) override;

  virtual void renderLightSources(RenderCallback* renderCallback) override;

  virtual bool checkBroken() override;

  virtual Vec2I tilePosition() const override;

  virtual List<Vec2I> spaces() const override;
  virtual List<MaterialSpace> materialSpaces() const override;
  virtual List<Vec2I> roots() const override;

  Direction direction() const;
  void setDirection(Direction direction);

  // Updates tile position and calls updateOrientation
  void setTilePosition(Vec2I const& pos) override;

  // Find a new valid orientation for the object
  void updateOrientation();
  List<Vec2I> anchorPositions() const;

  virtual List<Drawable> cursorHintDrawables() const;

  String name() const;
  String shortDescription() const;
  String category() const;

  virtual ObjectOrientationPtr currentOrientation() const;

  virtual List<PersistentStatusEffect> statusEffects() const override;
  virtual PolyF statusEffectArea() const override;

  virtual List<DamageSource> damageSources() const override;

  virtual Maybe<HitType> queryHit(DamageSource const& source) const override;
  Maybe<PolyF> hitPoly() const override;

  virtual List<DamageNotification> applyDamage(DamageRequest const& damage) override;

  virtual bool damageTiles(List<Vec2I> const& position, Vec2F const& sourcePosition, TileDamage const& tileDamage) override;

  RectF interactiveBoundBox() const override;

  bool isInteractive() const override;
  virtual InteractAction interact(InteractRequest const& request) override;
  List<Vec2I> interactiveSpaces() const override;

  Maybe<LuaValue> callScript(String const& func, LuaVariadic<LuaValue> const& args) override;
  Maybe<LuaValue> evalScript(String const& code) override;

  virtual Vec2F mouthPosition() const override;
  virtual Vec2F mouthPosition(bool ignoreAdjustments) const override;
  virtual List<ChatAction> pullPendingChatActions() override;

  void breakObject(bool smash = true);

  virtual size_t nodeCount(WireDirection direction) const override;
  virtual Vec2I nodePosition(WireNode wireNode) const override;
  virtual List<WireConnection> connectionsForNode(WireNode wireNode) const override;
  virtual bool nodeState(WireNode wireNode) const override;

  virtual void addNodeConnection(WireNode wireNode, WireConnection nodeConnection) override;
  virtual void removeNodeConnection(WireNode wireNode, WireConnection nodeConnection) override;

  virtual void evaluate(WireCoordinator* coordinator) override;

  virtual List<QuestArcDescriptor> offeredQuests() const override;
  virtual StringSet turnInQuests() const override;
  virtual Vec2F questIndicatorPosition() const override;

  Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args = {}) override;

  // Check, in order, the passed in object parameters, the config parameters,
  // and then the orientation parameters for the given key.  Returns 'def' if
  // no value is found.
  Json configValue(String const& name, Json const& def = Json()) const;

  ObjectConfigConstPtr config() const;

  float liquidFillLevel() const;

  bool biomePlaced() const;

  using Entity::setUniqueId;

protected:
  friend class ObjectDatabase;

  // Will be automatically called at appropriate times.  Derived classes must
  // call base class versions.
  virtual void getNetStates(bool initial);
  virtual void setNetStates();

  virtual void readStoredData(Json const& diskStore);
  virtual Json writeStoredData() const;

  void setImageKey(String const& name, String const& value);

  size_t orientationIndex() const;
  virtual void setOrientationIndex(size_t orientationIndex);

  PolyF volume() const;

  LuaMessageHandlingComponent<LuaStorableComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>> m_scriptComponent;
  mutable LuaAnimationComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>> m_scriptedAnimator;

  NetElementTopGroup m_netGroup;
  NetElementBool m_interactive;
  NetElementData<List<MaterialSpace>> m_materialSpaces;

private:
  struct InputNode {
    Vec2I position;
    NetElementData<List<WireConnection>> connections;
    NetElementBool state;
  };

  struct OutputNode {
    Vec2I position;
    NetElementData<List<WireConnection>> connections;
    NetElementBool state;
  };

  LuaCallbacks makeObjectCallbacks();
  LuaCallbacks makeAnimatorObjectCallbacks();

  void ensureNetSetup();
  List<Drawable> orientationDrawables(size_t orientationIndex) const;

  void addChatMessage(String const& message, Json const& config, String const& portrait = "");

  void writeOutboundNode(Vec2I outboundNode, bool state);

  EntityRenderLayer renderLayer() const;

  // Base class render() simply calls all of these in turn.
  void renderLights(RenderCallback* renderCallback) const;
  void renderParticles(RenderCallback* renderCallback);
  void renderSounds(RenderCallback* renderCallback);

  Vec2F damageShake() const;

  void checkLiquidBroken();
  GameTimer m_liquidCheckTimer;

  ObjectConfigConstPtr m_config;
  NetElementHashMap<String, Json> m_parameters;

  NetElementData<Maybe<String>> m_uniqueIdNetState;

  NetElementInt m_xTilePosition;
  NetElementInt m_yTilePosition;
  NetElementEnum<Direction> m_direction;
  float m_animationTimer;
  int m_currentFrame;

  Directives m_directives;
  Directives m_colorDirectives;

  Maybe<PeriodicFunction<float>> m_lightFlickering;

  EntityTileDamageStatusPtr m_tileDamageStatus;

  bool m_broken;
  bool m_unbreakable;
  NetElementFloat m_health;

  size_t m_orientationIndex;
  NetElementSize m_orientationIndexNetState;
  NetElementHashMap<String, String> m_netImageKeys;
  mutable StringMap<String> m_imageKeys;

  void resetEmissionTimers();
  List<GameTimer> m_emissionTimers;

  NetElementBool m_soundEffectEnabled;
  AudioInstancePtr m_soundEffect;

  NetElementData<Color> m_lightSourceColor;

  Vec2F m_animationPosition;
  float m_animationCenterLine;
  NetworkedAnimatorPtr m_networkedAnimator;
  NetworkedAnimator::DynamicTarget m_networkedAnimatorDynamicTarget;

  List<ChatAction> m_pendingChatActions;
  NetElementEvent m_newChatMessageEvent;
  NetElementString m_chatMessage;
  NetElementString m_chatPortrait;
  NetElementData<Json> m_chatConfig;

  mutable Maybe<pair<size_t, List<Drawable>>> m_orientationDrawablesCache;

  List<InputNode> m_inputNodes;
  List<OutputNode> m_outputNodes;

  NetElementData<List<QuestArcDescriptor>> m_offeredQuests;
  NetElementData<StringSet> m_turnInQuests;

  NetElementHashMap<String, Json> m_scriptedAnimationParameters;

  NetElementData<List<DamageSource>> m_damageSources;
};

}

#endif
