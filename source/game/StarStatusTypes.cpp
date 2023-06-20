#include "StarStatusTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

bool StatBaseMultiplier::operator==(StatBaseMultiplier const& rhs) const {
  return tie(statName, baseMultiplier) == tie(rhs.statName, rhs.baseMultiplier);
}

DataStream& operator>>(DataStream& ds, StatBaseMultiplier& baseMultiplier) {
  ds >> baseMultiplier.statName;
  ds >> baseMultiplier.baseMultiplier;
  return ds;
}

DataStream& operator<<(DataStream& ds, StatBaseMultiplier const& baseMultiplier) {
  ds << baseMultiplier.statName;
  ds << baseMultiplier.baseMultiplier;
  return ds;
}

bool StatValueModifier::operator==(StatValueModifier const& rhs) const {
  return tie(statName, value) == tie(rhs.statName, rhs.value);
}

DataStream& operator>>(DataStream& ds, StatValueModifier& valueModifier) {
  ds >> valueModifier.statName;
  ds >> valueModifier.value;
  return ds;
}

DataStream& operator<<(DataStream& ds, StatValueModifier const& valueModifier) {
  ds << valueModifier.statName;
  ds << valueModifier.value;
  return ds;
}

bool StatEffectiveMultiplier::operator==(StatEffectiveMultiplier const& rhs) const {
  return tie(statName, effectiveMultiplier) == tie(rhs.statName, rhs.effectiveMultiplier);
}

DataStream& operator>>(DataStream& ds, StatEffectiveMultiplier& effectiveMultiplier) {
  ds >> effectiveMultiplier.statName;
  ds >> effectiveMultiplier.effectiveMultiplier;
  return ds;
}

DataStream& operator<<(DataStream& ds, StatEffectiveMultiplier const& effectiveMultiplier) {
  ds << effectiveMultiplier.statName;
  ds << effectiveMultiplier.effectiveMultiplier;
  return ds;
}

StatModifier jsonToStatModifier(Json const& config) {
  String statName = config.getString("stat");
  if (auto baseMultiplier = config.optFloat("baseMultiplier")) {
    return StatModifier(StatBaseMultiplier{statName, *baseMultiplier});
  } else if (auto amount = config.optFloat("amount")) {
    return StatModifier(StatValueModifier{statName, *amount});
  } else if (auto effectiveMultiplier = config.optFloat("effectiveMultiplier")) {
    return StatModifier(StatEffectiveMultiplier{statName, *effectiveMultiplier});
  } else {
    throw JsonException("Could not find 'baseMultiplier' or 'effectiveMultiplier' or 'amount' element in stat effect config");
  }
}

Json jsonFromStatModifier(StatModifier const& modifier) {
  if (auto baseMultiplier = modifier.ptr<StatBaseMultiplier>()) {
    return JsonObject{{"stat", baseMultiplier->statName}, {"baseMultiplier", baseMultiplier->baseMultiplier}};
  } else if (auto valueModifier = modifier.ptr<StatValueModifier>()) {
    return JsonObject{{"stat", valueModifier->statName}, {"amount", valueModifier->value}};
  } else if (auto effectiveMultiplier = modifier.ptr<StatEffectiveMultiplier>()) {
    return JsonObject{{"stat", effectiveMultiplier->statName}, {"effectiveMultiplier", effectiveMultiplier->effectiveMultiplier}};
  } else {
    throw JsonException("No 'baseMultiplier', 'amount', or 'effectiveMultiplier' member found in json");
  }
}

PersistentStatusEffect jsonToPersistentStatusEffect(Json const& config) {
  if (config.isType(Json::Type::String)) {
    return UniqueStatusEffect(config.toString());
  } else if (config.isType(Json::Type::Object)) {
    return jsonToStatModifier(config);
  } else {
    throw JsonException("Json is wrong type for persistent stat effect config");
  }
}

Json jsonFromPersistentStatusEffect(PersistentStatusEffect const& effect) {
  if (auto uniqueStatusEffect = effect.ptr<UniqueStatusEffect>())
    return Json(*uniqueStatusEffect);
  else if (auto statModifier = effect.ptr<StatModifier>())
    return jsonFromStatModifier(*statModifier);

  return Json();
}

bool EphemeralStatusEffect::operator==(EphemeralStatusEffect const& rhs) const {
  return tie(uniqueEffect, duration) == tie(rhs.uniqueEffect, rhs.duration);
}

DataStream& operator>>(DataStream& ds, EphemeralStatusEffect& ephemeralStatusEffect) {
  ds >> ephemeralStatusEffect.uniqueEffect;
  ds >> ephemeralStatusEffect.duration;
  return ds;
}

DataStream& operator<<(DataStream& ds, EphemeralStatusEffect const& ephemeralStatusEffect) {
  ds << ephemeralStatusEffect.uniqueEffect;
  ds << ephemeralStatusEffect.duration;
  return ds;
}

EphemeralStatusEffect jsonToEphemeralStatusEffect(Json const& config) {
  if (config.isType(Json::Type::String)) {
    return EphemeralStatusEffect{UniqueStatusEffect(config.toString()), {}};
  } else if (config.isType(Json::Type::Object)) {
    String effectName = config.getString("effect");
    Maybe<float> duration = config.optFloat("duration");
    return EphemeralStatusEffect{UniqueStatusEffect(effectName), duration};
  } else {
    throw JsonException("Json is wrong type for ephemeral stat effect config");
  }
}

Json jsonFromEphemeralStatusEffect(EphemeralStatusEffect const& effect) {
  return JsonObject{{"effect", effect.uniqueEffect}, {"duration", jsonFromMaybe(effect.duration)}};
}

}
