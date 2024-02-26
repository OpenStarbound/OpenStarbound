#pragma once

#include "StarPeriodicFunction.hpp"
#include "StarTtlCache.hpp"
#include "StarGameTypes.hpp"
#include "StarItemDescriptor.hpp"
#include "StarParticle.hpp"
#include "StarSet.hpp"
#include "StarTileDamage.hpp"
#include "StarDamageTypes.hpp"
#include "StarStatusTypes.hpp"
#include "StarEntityRendering.hpp"
#include "StarTileEntity.hpp"

namespace Star {

STAR_CLASS(World);
STAR_CLASS(Image);
STAR_CLASS(ItemDatabase);
STAR_CLASS(RecipeDatabase);
STAR_CLASS(Object);
STAR_STRUCT(ObjectOrientation);
STAR_STRUCT(ObjectConfig);
STAR_CLASS(ObjectDatabase);

STAR_EXCEPTION(ObjectException, StarException);

struct ObjectOrientation {
  struct Anchor {
    TileLayer layer;
    Vec2I position;
    bool tilled;
    bool soil;
    Maybe<MaterialId> material;
  };

  struct ParticleEmissionEntry {
    float particleEmissionRate;
    float particleEmissionRateVariance;
    // Particle positions are considered relative to image pixels, and are
    // flipped with image flipping
    Particle particle;
    Particle particleVariance;
    bool placeInSpaces;
  };

  // The JSON values that were used to configure this orientation.
  Json config;

  EntityRenderLayer renderLayer;
  List<Drawable> imageLayers;
  bool flipImages;

  // Offset of image from (0, 0) object position, in tile coordinates
  Vec2F imagePosition;

  // If an object has frames > 1, then the image name will have the marker
  // "{frame}" replaced with an integer in [0, frames)
  unsigned frames;
  float animationCycle;

  // Spaces the object occupies.  By default, this is simply the single space
  // at the object position, but can be specified in config as either a list of
  // Vec2I, or by setting a threshold value using "spaceScanning", which will
  // scan the image (frame 1) for non-transparent pixels.
  List<Vec2I> spaces;
  RectI boundBox;

  // Allow an orientation to override the metaboundbox in case you don't want to
  // specify spaces
  Maybe<RectF> metaBoundBox;

  // Anchors of the object to place it in the world
  // For background tiles set in order for the object to
  // remain placed.  Must be within 1 space of the bounding box of spaces.
  // For foreground tiles this cannot logically contain any position
  // also in spaces, as objects cannot overlap with foreground tiles.
  List<Anchor> anchors;

  // if true, only one anchor needs to be valid for the orientation to be valid,
  // otherwise all anchors must be valid
  bool anchorAny;

  Maybe<Direction> directionAffinity;

  // Optional list of material spaces
  List<MaterialSpace> materialSpaces;

  // optionally override the default spaces used for interaction
  Maybe<List<Vec2I>> interactiveSpaces;

  Vec2F lightPosition;
  float beamAngle;

  List<ParticleEmissionEntry> particleEmitters;

  Maybe<PolyF> statusEffectArea;
  Json touchDamageConfig;

  static ParticleEmissionEntry parseParticleEmitter(String const& path, Json const& config);
  bool placementValid(World const* world, Vec2I const& position) const;
  bool anchorsValid(World const* world, Vec2I const& position) const;
};

// TODO: This is used very strangely and inconsistently. We go to all the trouble of populating
// this ObjectConfig structure from the JSON, but then keep around the JSON anyway. In some
// places we access the objectConfig, but in many more we use the object's configValue method
// to access the raw config JSON which means it's inconsistent which parameters can be overridden
// by instance values at various levels. This whole system needs reevaluation.
struct ObjectConfig {
  // Returns the index of the best valid orientation.  If no orientations are
  // valid, returns NPos
  size_t findValidOrientation(World const* world, Vec2I const& position, Maybe<Direction> directionAffinity = Maybe<Direction>()) const;

  String path;
  // The JSON values that were used to configure this Object
  Json config;

  String name;
  String type;
  String race;
  String category;
  StringList colonyTags;
  StringList scripts;
  StringList animationScripts;

  unsigned price;
  bool printable;
  bool scannable;

  bool interactive;

  StringMap<Color> lightColors;
  bool pointLight;
  float pointBeam;
  float beamAmbience;
  Maybe<PeriodicFunction<float>> lightFlickering;

  String soundEffect;
  float soundEffectRangeMultiplier;

  List<PersistentStatusEffect> statusEffects;
  Json touchDamageConfig;

  bool hasObjectItem;
  bool retainObjectParametersInItem;

  bool smashable;
  bool smashOnBreak;
  bool unbreakable;
  String smashDropPool;
  List<List<ItemDescriptor>> smashDropOptions;
  StringList smashSoundOptions;
  JsonArray smashParticles;

  String breakDropPool;
  List<List<ItemDescriptor>> breakDropOptions;

  TileDamageParameters tileDamageParameters;
  float damageShakeMagnitude;
  String damageMaterialKind;

  EntityDamageTeam damageTeam;

  Maybe<float> minimumLiquidLevel;
  Maybe<float> maximumLiquidLevel;
  float liquidCheckInterval;

  float health;

  Json animationConfig;

  List<ObjectOrientationPtr> orientations;

  // If true, the object will root - it will prevent the blocks it is
  // anchored to from being destroyed directly, and damage from those
  // blocks will be redirected to the object
  bool rooting;

  bool biomePlaced;
};

class ObjectDatabase {
public:
  static List<Vec2I> scanImageSpaces(ImageConstPtr const& image, Vec2F const& position, float fillLimit, bool flip = false);
  static Json parseTouchDamage(String const& path, Json const& touchDamage);
  static List<ObjectOrientationPtr> parseOrientations(String const& path, Json const& configList);

  ObjectDatabase();

  void cleanup();

  StringList allObjects() const;
  bool isObject(String const& name) const;

  ObjectConfigPtr getConfig(String const& objectName) const;
  List<ObjectOrientationPtr> const& getOrientations(String const& objectName) const;

  ObjectPtr createObject(String const& objectName, Json const& objectParameters = JsonObject()) const;
  ObjectPtr diskLoadObject(Json const& diskStore) const;
  ObjectPtr netLoadObject(ByteArray const& netStore) const;

  bool canPlaceObject(World const* world, Vec2I const& position, String const& objectName) const;
  // If the object is placeable in the given position, creates the given object
  // and sets its position and direction and returns it, otherwise returns
  // null.
  ObjectPtr createForPlacement(World const* world, String const& objectName, Vec2I const& position,
      Direction direction, Json const& parameters = JsonObject()) const;

  List<Drawable> cursorHintDrawables(World const* world, String const& objectName, Vec2I const& position,
      Direction direction, Json parameters = {}) const;

private:
  static ObjectConfigPtr readConfig(String const& path);

  StringMap<String> m_paths;
  mutable Mutex m_cacheMutex;
  mutable HashTtlCache<String, ObjectConfigPtr> m_configCache;
};

}
