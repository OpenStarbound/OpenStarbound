#pragma once

#include "StarThread.hpp"
#include "StarItemDescriptor.hpp"
#include "StarHumanoid.hpp"
#include "StarStatusTypes.hpp"
#include "StarLuaRoot.hpp"
#include "StarTtlCache.hpp"

namespace Star {

STAR_CLASS(SpeciesDefinition);
STAR_CLASS(SpeciesDatabase);

struct SpeciesCharCreationTooltip {
  String title;
  String subTitle;
  String description;
};

struct SpeciesGenderOption {
  Gender gender;
  String name;
  String image;
  String characterImage;
  List<String> hairOptions;
  String hairGroup;
  List<String> shirtOptions;
  List<String> pantsOptions;
  String facialHairGroup;
  List<String> facialHairOptions;
  String facialMaskGroup;
  List<String> facialMaskOptions;
};

struct SpeciesOption {
  SpeciesOption();

  String species;
  bool headOptionAsHairColor;
  bool headOptionAsFacialhair;
  bool altOptionAsUndyColor;
  bool altOptionAsHairColor;
  bool altOptionAsFacialMask;
  bool hairColorAsBodySubColor;
  bool bodyColorAsFacialMaskSubColor;
  bool altColorAsFacialMaskSubColor;
  List<SpeciesGenderOption> genderOptions;
  List<String> bodyColorDirectives;
  List<String> undyColorDirectives;
  List<String> hairColorDirectives;
};

class SpeciesDefinition {
public:
  SpeciesDefinition(Json const& config);

  String kind() const;
  bool playerSelectable() const;
  SpeciesOption const& options() const;
  Json humanoidConfig() const;
  List<Personality> const& personalities() const;
  String nameGen(Gender gender) const;
  String ouchNoise(Gender gender) const;
  List<ItemDescriptor> defaultItems() const;
  List<ItemDescriptor> defaultBlueprints() const;
  StringList charGenTextLabels() const;
  String skull() const;
  List<PersistentStatusEffect> statusEffects() const;
  String effectDirectives() const;
  List<String> animationScripts() const;

  SpeciesCharCreationTooltip const& tooltip() const;

  void generateHumanoid(HumanoidIdentity& identity, int64_t seed);

private:
  String m_kind;
  SpeciesCharCreationTooltip m_tooltip;
  bool m_playerSelectable;
  Json m_config;
  String m_humanoidConfig;
  Json m_humanoidOverrides;
  List<Personality> m_personalities;
  List<String> m_nameGen;
  List<String> m_ouchNoises;
  SpeciesOption m_options;
  List<ItemDescriptor> m_defaultItems;
  List<ItemDescriptor> m_defaultBlueprints;
  StringList m_charGenTextLabels;
  String m_skull;
  List<PersistentStatusEffect> m_statusEffects;
  String m_effectDirectives;

  List<String> m_buildScripts;
  List<String> m_animationScripts;

  friend class SpeciesDatabase;
};

class SpeciesDatabase {
public:
  SpeciesDatabase();

  SpeciesDefinitionPtr species(String const& kind) const;
  StringMap<SpeciesDefinitionPtr> allSpecies() const;

  Json humanoidConfig(HumanoidIdentity identity, Json config = Json()) const;

private:
  StringMap<SpeciesDefinitionPtr> m_species;

  mutable RecursiveMutex m_luaMutex;
  LuaRootPtr m_luaRoot;
};

}
