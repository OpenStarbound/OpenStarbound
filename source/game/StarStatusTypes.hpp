#pragma once

#include "StarJson.hpp"
#include "StarStrongTypedef.hpp"
#include "StarDataStream.hpp"
#include "StarIdMap.hpp"

namespace Star {

STAR_EXCEPTION(StatusException, StarException);

// Multipliers act exactly the way you'd expect: 0.0 is a 100% reduction of the
// base stat, while 2.0 is a 100% increase. Since these are *base* multipliers
// they do not interact with each other, thus stacking a 0.0 and a 2.0 leaves
// the stat unmodified
struct StatBaseMultiplier {
  String statName;
  float baseMultiplier;

  bool operator==(StatBaseMultiplier const& rhs) const;
};

DataStream& operator>>(DataStream& ds, StatBaseMultiplier& baseMultiplier);
DataStream& operator<<(DataStream& ds, StatBaseMultiplier const& baseMultiplier);

struct StatValueModifier {
  String statName;
  float value;

  bool operator==(StatValueModifier const& rhs) const;
};

DataStream& operator>>(DataStream& ds, StatValueModifier& valueModifier);
DataStream& operator<<(DataStream& ds, StatValueModifier const& valueModifier);

// Unlike base multipliers, these all stack multiplicatively with the final
// stat value (including all base and value modifiers) such that an effective
// multiplier of 0.0 will ALWAYS reduce the stat to 0 regardless of other
// effects
struct StatEffectiveMultiplier {
  String statName;
  float effectiveMultiplier;

  bool operator==(StatEffectiveMultiplier const& rhs) const;
};

DataStream& operator>>(DataStream& ds, StatEffectiveMultiplier& effectiveMultiplier);
DataStream& operator<<(DataStream& ds, StatEffectiveMultiplier const& effectiveMultiplier);

typedef MVariant<StatValueModifier, StatBaseMultiplier, StatEffectiveMultiplier> StatModifier;

StatModifier jsonToStatModifier(Json const& config);
Json jsonFromStatModifier(StatModifier const& modifier);

typedef uint32_t StatModifierGroupId;
typedef IdMap<StatModifierGroupId, List<StatModifier>> StatModifierGroupMap;

// Unique stat effects are identified uniquely by name.
typedef String UniqueStatusEffect;

// Second element here is *percentage* of duration remaining, based on the
// highest duration that the effect has had
typedef List<pair<UniqueStatusEffect, Maybe<float>>> ActiveUniqueStatusEffectSummary;

// Persistent status effects can either be a modifier effect or unique effect
typedef MVariant<StatModifier, UniqueStatusEffect> PersistentStatusEffect;

// Reads either a name of a unique stat effect or a stat modifier object
PersistentStatusEffect jsonToPersistentStatusEffect(Json const& config);
Json jsonFromPersistentStatusEffect(PersistentStatusEffect const& effect);

// Ephemeral effects are always unique effects and either use the default
// duration in their config or optionally the default
struct EphemeralStatusEffect {
  UniqueStatusEffect uniqueEffect;
  Maybe<float> duration;

  bool operator==(EphemeralStatusEffect const& rhs) const;
};

DataStream& operator>>(DataStream& ds, EphemeralStatusEffect& ephemeralStatusEffect);
DataStream& operator<<(DataStream& ds, EphemeralStatusEffect const& ephemeralStatusEffect);

// Reads either a name of a unique stat effect or an object containing the
// type name and optionally the duration.
EphemeralStatusEffect jsonToEphemeralStatusEffect(Json const& config);
Json jsonFromEphemeralStatusEffect(EphemeralStatusEffect const& effect);

}
