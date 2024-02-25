#pragma once

#include "StarJson.hpp"
#include "StarEither.hpp"
#include "StarGameTypes.hpp"
#include "StarList.hpp"
#include "StarCellularLiquid.hpp"
#include "StarItemDescriptor.hpp"

namespace Star {

STAR_STRUCT(LiquidSettings);
STAR_CLASS(LiquidsDatabase);

STAR_EXCEPTION(LiquidException, StarException);

typedef Either<MaterialId, LiquidId> LiquidInteractionResult;

struct LiquidSettings {
  LiquidSettings();

  String name;
  LiquidId id;
  String path;
  Json config;
  Json descriptions;

  unsigned tickDelta;
  Vec4B liquidColor;
  Vec3F radiantLightLevel;
  ItemDescriptor itemDrop;
  JsonArray statusEffects;

  HashMap<LiquidId, Maybe<LiquidInteractionResult>> interactions;
};

class LiquidsDatabase {
public:
  LiquidsDatabase();

  LiquidCellEngineParameters liquidEngineParameters() const;
  float backgroundDrain() const;

  StringList liquidNames() const;

  // Returns settings object for all liquids except "empty"
  List<LiquidSettingsConstPtr> allLiquidSettings() const;

  bool isLiquidName(String const& name) const;
  bool isValidLiquidId(LiquidId liquidId) const;

  LiquidId liquidId(String const& str) const;
  String liquidName(LiquidId liquidId) const;
  String liquidDescription(LiquidId liquidId, String const& species) const;
  String liquidDescription(LiquidId liquidId) const;
  Maybe<String> liquidPath(LiquidId liquidId) const;
  Maybe<Json> liquidConfig(LiquidId liquidId) const;

  // Returns null on EmptyLiquidId or invalid liquid id
  LiquidSettingsConstPtr liquidSettings(LiquidId liquidId) const;

  Vec3F radiantLight(LiquidLevel level) const;

  Maybe<LiquidInteractionResult> interact(LiquidId target, LiquidId other) const;

private:
  LiquidCellEngineParameters m_liquidEngineParameters;
  float m_backgroundDrain;
  List<LiquidSettingsConstPtr> m_settings;
  StringMap<LiquidId> m_liquidNames;
};

inline float LiquidsDatabase::backgroundDrain() const {
  return m_backgroundDrain;
}

inline bool LiquidsDatabase::isLiquidName(String const& name) const {
  return m_liquidNames.contains(name);
}

inline bool LiquidsDatabase::isValidLiquidId(LiquidId liquidId) const {
  return liquidId == EmptyLiquidId || (liquidId < m_settings.size() && m_settings[liquidId]);
}

inline LiquidSettingsConstPtr LiquidsDatabase::liquidSettings(LiquidId liquidId) const {
  if (liquidId >= m_settings.size())
    return {};
  return m_settings[liquidId];
}

inline Vec3F LiquidsDatabase::radiantLight(LiquidLevel level) const {
  if (level.liquid >= m_settings.size())
    return Vec3F();

  auto const& settings = m_settings[level.liquid];
  if (!settings)
    return Vec3F();

  return settings->radiantLightLevel * level.level;
}

}
