#ifndef STAR_PLANT_DATABASE_HPP
#define STAR_PLANT_DATABASE_HPP

#include "StarJson.hpp"
#include "StarThread.hpp"
#include "StarTileDamage.hpp"

namespace Star {

STAR_CLASS(Plant);
STAR_CLASS(PlantDatabase);

STAR_EXCEPTION(PlantDatabaseException, StarException);

// Configuration for a specific tree variant
struct TreeVariant {
  TreeVariant();
  TreeVariant(Json const& json);

  Json toJson() const;

  String stemName;
  String foliageName;

  String stemDirectory;
  Json stemSettings;
  float stemHueShift;

  String foliageDirectory;
  Json foliageSettings;
  float foliageHueShift;

  Json descriptions;
  bool ceiling;

  bool ephemeral;

  Json stemDropConfig;
  Json foliageDropConfig;

  TileDamageParameters tileDamageParameters;
};

// Configuration for a specific grass variant
struct GrassVariant {
  GrassVariant();
  GrassVariant(Json const& json);

  Json toJson() const;

  String name;

  String directory;
  StringList images;
  float hueShift;

  Json descriptions;
  bool ceiling;

  bool ephemeral;

  TileDamageParameters tileDamageParameters;
};

// Configuration for a specific bush variant
struct BushVariant {
  struct BushShape {
    String image;
    StringList mods;
  };

  BushVariant();
  BushVariant(Json const& json);

  Json toJson() const;

  String bushName;
  String modName;

  String directory;
  List<BushShape> shapes;

  float baseHueShift;
  float modHueShift;

  Json descriptions;
  bool ceiling;

  bool ephemeral;

  TileDamageParameters tileDamageParameters;
};

class PlantDatabase {
public:
  PlantDatabase();

  StringList treeStemNames(bool ceiling = false) const;
  StringList treeFoliageNames() const;
  // Each stem / foliage set has its own patterns of shapes that must match up
  String treeStemShape(String const& stemName) const;
  String treeFoliageShape(String const& foliageName) const;
  Maybe<String> treeStemDirectory(String const& stemName) const;
  Maybe<String> treeFoliageDirectory(String const& foliageName) const;
  // Throws an exception if stem shape and foliage shape do not match
  TreeVariant buildTreeVariant(String const& stemName, float stemHueShift, String const& foliageName, float foliageHueShift) const;
  // Build a foliage-less tree
  TreeVariant buildTreeVariant(String const& stemName, float stemHueShift) const;

  StringList grassNames(bool ceiling = false) const;
  GrassVariant buildGrassVariant(String const& grassName, float hueShift) const;

  StringList bushNames(bool ceiling = false) const;
  StringList bushMods(String const& bushName) const;
  BushVariant buildBushVariant(String const& bushName, float baseHueShift, String const& modName, float modHueShift) const;

  PlantPtr createPlant(TreeVariant const& treeVariant, uint64_t seed) const;
  PlantPtr createPlant(GrassVariant const& grassVariant, uint64_t seed) const;
  PlantPtr createPlant(BushVariant const& bushVariant, uint64_t seed) const;

private:
  struct Config {
    String directory;
    Json settings;
  };

  StringMap<Config> m_treeStemConfigs;
  StringMap<Config> m_treeFoliageConfigs;

  StringMap<Config> m_grassConfigs;

  StringMap<Config> m_bushConfigs;
};

}

#endif
