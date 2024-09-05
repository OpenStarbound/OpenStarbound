#pragma once

#include "StarThread.hpp"
#include "StarHumanoid.hpp"
#include "StarDamageTypes.hpp"
#include "StarStatusTypes.hpp"
#include "StarEntitySplash.hpp"
#include "StarItemDescriptor.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(Npc);
STAR_CLASS(NpcDatabase);

STAR_EXCEPTION(NpcException, StarException);

struct NpcVariant {
  String species;
  String typeName;
  float level;
  uint64_t seed;

  Json overrides;

  StringList scripts;
  unsigned initialScriptDelta;
  Json scriptConfig;

  HumanoidIdentity humanoidIdentity;
  Json humanoidConfig;

  Json movementParameters;
  Json statusControllerSettings;
  List<PersistentStatusEffect> innateStatusEffects;
  Json touchDamageConfig;

  StringMap<ItemDescriptor> items;

  StringList dropPools;
  bool disableWornArmor;

  bool persistent;
  bool keepAlive;

  TeamType damageTeamType;
  uint8_t damageTeam;

  Vec3B nametagColor;

  EntitySplashConfig splashConfig;
};

class NpcDatabase {
public:
  NpcDatabase();

  NpcVariant generateNpcVariant(String const& species, String const& typeName, float level) const;
  NpcVariant generateNpcVariant(String const& species, String const& typeName, float level, uint64_t seed, Json const& overrides) const;

  ByteArray writeNpcVariant(NpcVariant const& variant, NetCompatibilityRules rules = {}) const;
  NpcVariant readNpcVariant(ByteArray const& data, NetCompatibilityRules rules = {}) const;

  Json writeNpcVariantToJson(NpcVariant const& variant) const;
  NpcVariant readNpcVariantFromJson(Json const& data) const;

  NpcPtr createNpc(NpcVariant const& npcVariant) const;
  NpcPtr diskLoadNpc(Json const& diskStore) const;
  NpcPtr netLoadNpc(ByteArray const& netStore, NetCompatibilityRules rules = {}) const;

  List<Drawable> npcPortrait(NpcVariant const& npcVariant, PortraitMode mode) const;

  Json buildConfig(String const& typeName, Json const& overrides = Json()) const;

private:
  // Recursively merges maps and lets any non-null merger (including lists)
  // override any base value
  Json mergeConfigValues(Json const& base, Json const& merger) const;

  StringMap<Json> m_npcTypes;
};

}
