#pragma once

#include "StarJson.hpp"
#include "StarBiMap.hpp"
#include "StarGameTypes.hpp"

namespace Star {

STAR_EXCEPTION(TechDatabaseException, StarException);

STAR_CLASS(TechDatabase);

enum class TechType {
  Head,
  Body,
  Legs
};
extern EnumMap<TechType> const TechTypeNames;

struct TechConfig {
  String name;
  String path;
  Json parameters;

  TechType type;

  StringList scripts;
  Maybe<String> animationConfig;

  String description;
  String shortDescription;
  Rarity rarity;
  String icon;
};

class TechDatabase {
public:
  TechDatabase();

  bool contains(String const& techName) const;
  TechConfig tech(String const& techName) const;

private:
  TechConfig parseTech(Json const& config, String const& path) const;

  StringMap<TechConfig> m_tech;
};

}
