#pragma once

#include "StarNetworkedAnimator.hpp"
#include "StarActorMovementController.hpp"
#include "StarTtlCache.hpp"
#include "StarDamageTypes.hpp"
#include "StarStatusTypes.hpp"
#include "StarImageProcessing.hpp"
#include "StarEntityRenderingTypes.hpp"

namespace Star {

STAR_CLASS(LuaRoot);
STAR_CLASS(Rebuilder);
STAR_CLASS(RandomSource);
STAR_CLASS(Monster);
STAR_CLASS(MonsterDatabase);

STAR_EXCEPTION(MonsterException, StarException);

struct MonsterVariant {
  String type;
  uint64_t seed;
  Json uniqueParameters;

  Maybe<String> shortDescription;
  Maybe<String> description;

  Json animatorConfig;
  StringMap<String> animatorPartTags;
  float animatorZoom;
  // Is the animator specified Left facing?
  bool reversed;

  // Either is a String which specifies a dropPool, or a map which maps
  // damageSourceKind to the appropriate treasure pool for this monster, with a
  // "default" key as a catch-all.
  Json dropPoolConfig;

  // Every parameter specified in each section of the monster configuration is
  // stored here.  The base parameters, size parameters, variation parameters,
  // and part parameters are all merged together into one final configuration.
  Json parameters;

  // Parameters common to all Monsters

  StringList scripts;
  unsigned initialScriptDelta;
  StringList animationScripts;

  RectF metaBoundBox;
  EntityRenderLayer renderLayer;
  float scale;

  ActorMovementParameters movementSettings;
  float walkMultiplier;
  float runMultiplier;
  float jumpMultiplier;
  float weightMultiplier;
  float healthMultiplier;
  float touchDamageMultiplier;

  Json touchDamageConfig;
  StringMap<Json> animationDamageParts;
  Json statusSettings;
  Vec2F mouthOffset;
  Vec2F feetOffset;

  String powerLevelFunction;
  String healthLevelFunction;

  ClientEntityMode clientEntityMode;
  bool persistent;

  TeamType damageTeamType;
  uint8_t damageTeam;

  PolyF selfDamagePoly;

  Maybe<String> portraitIcon;

  float damageReceivedAggressiveDuration;
  float onDamagedOthersAggressiveDuration;
  float onFireAggressiveDuration;

  Vec3B nametagColor;
  Maybe<ColorReplaceMap> colorSwap;
};

class MonsterDatabase {
public:
  MonsterDatabase();

  void cleanup();

  StringList monsterTypes() const;

  MonsterVariant randomMonster(String const& typeName, Json const& uniqueParameters = JsonObject()) const;
  MonsterVariant monsterVariant(String const& typeName, uint64_t seed, Json const& uniqueParameters = JsonObject()) const;

  ByteArray writeMonsterVariant(MonsterVariant const& variant, NetCompatibilityRules rules = {}) const;
  MonsterVariant readMonsterVariant(ByteArray const& data, NetCompatibilityRules rules = {}) const;

  Json writeMonsterVariantToJson(MonsterVariant const& mVar) const;
  MonsterVariant readMonsterVariantFromJson(Json const& variant) const;

  // If level is 0, then the monster will start with the threat level of
  // whatever world they're spawned in.
  MonsterPtr createMonster(MonsterVariant monsterVariant, Maybe<float> level = {}, Json uniqueParameters = {}) const;
  MonsterPtr diskLoadMonster(Json const& diskStore) const;
  MonsterPtr netLoadMonster(ByteArray const& netStore, NetCompatibilityRules rules = {}) const;

  List<Drawable> monsterPortrait(MonsterVariant const& variant) const;

  pair<String, String> skillInfo(String const& skillName) const;
  Json skillConfigParameter(String const& skillName, String const& configParameterName) const;

  ColorReplaceMap colorSwap(String const& setName, uint64_t seed) const;

private:
  struct MonsterType {
    String typeName;
    Maybe<String> shortDescription;
    Maybe<String> description;

    StringList categories;
    StringList partTypes;

    String animationConfigPath;
    String colors;
    bool reversed;

    JsonArray dropPools;

    Json baseParameters;

    // Additional part-specific parameters which will override any part-specific
    // parameters (such as skills, sounds, etc.) defined in individual .monsterpart files
    Json partParameterOverrides;

    // Description of all part parameters, and how they are combined and with
    // what defaults.
    Json partParameterDescription;
  };

  struct MonsterPart {
    String name;
    String category;
    String type;

    String path;
    JsonObject frames;
    Json partParameters;
  };

  struct MonsterSkill {
    String name;
    String label;
    String image;

    Json config;
    Json parameters;
    Json animationParameters;
  };

  // Merges part configuration by the method specified in the part parameter
  // config.
  static Json mergePartParameters(Json const& partParameterDescription, JsonArray const& parameters);

  // Merges final monster variant parameters together according to the
  // hard-coded variant merge rules (handles things like scripts which are
  // combined rather than overwritten)
  static Json mergeFinalParameters(JsonArray const& parameters);

  // Reads common parameters out of parameters map
  static void readCommonParameters(MonsterVariant& monsterVariant);

  // Maps category name -> part type -> part name -> MonsterPart.  part name ->
  // MonsterPart needs to be be in a predictable order.
  typedef StringMap<StringMap<Map<String, MonsterPart>>> PartDirectory;

  MonsterVariant produceMonster(String const& typeName, uint64_t seed, Json const& uniqueParameters) const;

  // Given a variant including parameters for baseSkills and specialSkills,
  // returns a variant containing a final 'skills' list of chosen skills, also
  // merges animation configs from skills together.
  pair<Json, Json> chooseSkills(Json const& parameters, Json const& animatorConfig, RandomSource& rand) const;

  StringMap<MonsterType> m_monsterTypes;
  PartDirectory m_partDirectory;
  StringMap<MonsterSkill> m_skills;
  StringMap<List<ColorReplaceMap>> m_colorSwaps;

  mutable Mutex m_cacheMutex;

  RebuilderPtr m_rebuilder;

  // Key here is the type name, seed, and the serialized unique parameters JSON
  mutable HashTtlCache<tuple<String, uint64_t, Json>, MonsterVariant> m_monsterCache;
};

}
