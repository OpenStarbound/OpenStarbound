#include "StarPlantDatabase.hpp"
#include "StarPlant.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

TreeVariant::TreeVariant()
  : stemHueShift(), foliageHueShift(), ceiling(), ephemeral() {}

TreeVariant::TreeVariant(Json const& variant) {
  stemName = variant.getString("stemName");
  foliageName = variant.getString("foliageName");
  stemDirectory = variant.getString("stemDirectory");
  stemSettings = variant.get("stemSettings");
  stemHueShift = variant.getFloat("stemHueShift");
  foliageDirectory = variant.getString("foliageDirectory");
  foliageSettings = variant.get("foliageSettings");
  foliageHueShift = variant.getFloat("foliageHueShift");
  descriptions = variant.get("descriptions");
  ceiling = variant.getBool("ceiling");
  ephemeral = variant.getBool("ephemeral");
  stemDropConfig = variant.get("stemDropConfig");
  foliageDropConfig = variant.get("foliageDropConfig");
  tileDamageParameters = TileDamageParameters(variant.get("tileDamageParameters"));
}

Json TreeVariant::toJson() const {
  return JsonObject{
      {"stemName", stemName},
      {"foliageName", foliageName},
      {"stemDirectory", stemDirectory},
      {"stemSettings", stemSettings},
      {"stemHueShift", stemHueShift},
      {"foliageDirectory", foliageDirectory},
      {"foliageSettings", foliageSettings},
      {"foliageHueShift", foliageHueShift},
      {"descriptions", descriptions},
      {"ceiling", ceiling},
      {"ephemeral", ephemeral},
      {"stemDropConfig", stemDropConfig},
      {"foliageDropConfig", foliageDropConfig},
      {"tileDamageParameters", tileDamageParameters.toJson()},
  };
}

GrassVariant::GrassVariant() : hueShift(), ceiling(), ephemeral() {}

GrassVariant::GrassVariant(Json const& variant) {
  name = variant.getString("name");
  directory = variant.getString("directory");
  images = jsonToStringList(variant.get("images"));
  hueShift = variant.getFloat("hueShift");
  descriptions = variant.get("descriptions");
  ceiling = variant.getBool("ceiling");
  ephemeral = variant.getBool("ephemeral");
  tileDamageParameters = TileDamageParameters(variant.get("tileDamageParameters"));
}

Json GrassVariant::toJson() const {
  return JsonObject{{"name", name},
      {"directory", directory},
      {"images", jsonFromStringList(images)},
      {"hueShift", hueShift},
      {"descriptions", descriptions},
      {"ceiling", ceiling},
      {"ephemeral", ephemeral},
      {"tileDamageParameters", tileDamageParameters.toJson()}};
}

BushVariant::BushVariant() : baseHueShift(), modHueShift(), ceiling(), ephemeral() {}

BushVariant::BushVariant(Json const& variant) {
  bushName = variant.getString("bushName");
  modName = variant.getString("modName");
  directory = variant.getString("directory");
  shapes = variant.getArray("shapes").transformed([](Json const& v) {
      return BushShape{v.getString(0), jsonToStringList(v.get(1))};
    });
  baseHueShift = variant.getFloat("baseHueShift");
  modHueShift = variant.getFloat("modHueShift");
  descriptions = variant.get("descriptions");
  ceiling = variant.getBool("ceiling");
  ephemeral = variant.getBool("ephemeral");
  tileDamageParameters = TileDamageParameters(variant.get("tileDamageParameters"));
}

Json BushVariant::toJson() const {
  return JsonObject{{"bushName", bushName},
      {"modName", modName},
      {"directory", directory},
      {"shapes", shapes.transformed([](BushShape const& shape) -> Json {
          return JsonArray{shape.image, jsonFromStringList(shape.mods)};
        })},
      {"baseHueShift", baseHueShift},
      {"modHueShift", modHueShift},
      {"descriptions", descriptions},
      {"ceiling", ceiling},
      {"ephemeral", ephemeral},
      {"tileDamageParameters", tileDamageParameters.toJson()}};
}

PlantDatabase::PlantDatabase() {
  auto assets = Root::singleton().assets();

  auto stems = assets->scanExtension("modularstem");
  auto foliages = assets->scanExtension("modularfoliage");
  auto grasses = assets->scanExtension("grass");
  auto bushes = assets->scanExtension("bush");

  assets->queueJsons(stems);
  assets->queueJsons(foliages);
  assets->queueJsons(grasses);
  assets->queueJsons(bushes);

  try {
    for (auto file : stems) {
      auto config = assets->json(file);
      m_treeStemConfigs.insert(config.getString("name"), Config{AssetPath::directory(file), config.toObject()});
    }

    for (auto file : foliages) {
      auto config = assets->json(file);
      m_treeFoliageConfigs.insert(config.getString("name"), Config{AssetPath::directory(file), config.toObject()});
    }

    for (auto file : grasses) {
      auto config = assets->json(file);
      m_grassConfigs.insert(config.getString("name"), Config{AssetPath::directory(file), config.toObject()});
    }

    for (auto file : bushes) {
      auto config = assets->json(file);
      m_bushConfigs.insert(config.getString("name"), Config{AssetPath::directory(file), config.toObject()});
    }
  } catch (StarException const& e) {
    throw PlantDatabaseException("Error loading plant database", e);
  }
}

StringList PlantDatabase::treeStemNames(bool ceiling) const {
  StringList names;
  for (auto const& pair : m_treeStemConfigs) {
    if (pair.second.settings.getBool("ceiling", false) == ceiling)
      names.append(pair.first);
  }
  return names;
}

StringList PlantDatabase::treeFoliageNames() const {
  return m_treeFoliageConfigs.keys();
}

String PlantDatabase::treeStemShape(String const& stemName) const {
  return m_treeStemConfigs.get(stemName).settings.get("shape").toString();
}

String PlantDatabase::treeFoliageShape(String const& foliageName) const {
  return m_treeFoliageConfigs.get(foliageName).settings.get("shape").toString();
}

Maybe<String> PlantDatabase::treeStemDirectory(String const& stemName) const {
  if (auto stem = m_treeStemConfigs.maybe(stemName))
    return stem->directory;
  return {};
}

Maybe<String> PlantDatabase::treeFoliageDirectory(String const& foliageName) const {
  if (auto foliage = m_treeFoliageConfigs.maybe(foliageName))
    return foliage->directory;
  return {};
}

TreeVariant PlantDatabase::buildTreeVariant(
    String const& stemName, float stemHueShift, String const& foliageName, float foliageHueShift) const {
  if (!m_treeStemConfigs.contains(stemName) || !m_treeFoliageConfigs.contains(foliageName))
    throw PlantDatabaseException::format("stemName '{}' or foliageName '{}' not found in plant database", stemName, foliageName);

  TreeVariant treeVariant;

  treeVariant.stemName = stemName;
  treeVariant.foliageName = foliageName;

  auto stemConfig = m_treeStemConfigs.get(stemName);
  treeVariant.stemDirectory = stemConfig.directory;
  treeVariant.stemSettings = stemConfig.settings;
  treeVariant.stemHueShift = stemHueShift;

  auto foliageConfig = m_treeFoliageConfigs.get(foliageName);
  treeVariant.foliageDirectory = foliageConfig.directory;
  treeVariant.foliageSettings = foliageConfig.settings;
  treeVariant.foliageHueShift = foliageHueShift;

  treeVariant.ceiling = stemConfig.settings.getBool("ceiling", false);

  treeVariant.stemDropConfig = stemConfig.settings.get("dropConfig", JsonObject());
  treeVariant.foliageDropConfig = foliageConfig.settings.get("dropConfig", JsonObject());

  JsonObject descriptions;
  for (auto const& entry : stemConfig.settings.iterateObject()) {
    if (entry.first.endsWith("Description"))
      descriptions[entry.first] = entry.second;
  }
  descriptions["description"] = stemConfig.settings.getString("description", stemName + " with " + foliageName);
  treeVariant.descriptions = descriptions;

  treeVariant.ephemeral = stemConfig.settings.getBool("allowsBlockPlacement", false);

  treeVariant.tileDamageParameters = TileDamageParameters(
      stemConfig.settings.get("damageTable", "/plants/treeDamage.config"),
      stemConfig.settings.getFloat("health", 1.0f));

  return treeVariant;
}

TreeVariant PlantDatabase::buildTreeVariant(String const& stemName, float stemHueShift) const {
  if (!m_treeStemConfigs.contains(stemName))
    throw PlantDatabaseException(strf("stemName '{}' not found in plant database", stemName));

  TreeVariant treeVariant;

  auto stemConfig = m_treeStemConfigs.get(stemName);
  treeVariant.stemDirectory = stemConfig.directory;
  treeVariant.stemSettings = stemConfig.settings;
  treeVariant.stemHueShift = stemHueShift;

  treeVariant.ceiling = stemConfig.settings.getBool("ceiling", false);

  treeVariant.stemDropConfig = stemConfig.settings.get("dropConfig", JsonObject());

  treeVariant.foliageSettings = JsonObject();
  treeVariant.foliageDropConfig = JsonObject();

  JsonObject descriptions;
  for (auto const& entry : stemConfig.settings.iterateObject()) {
    if (entry.first.endsWith("Description"))
      descriptions[entry.first] = entry.second;
  }
  descriptions["description"] = stemConfig.settings.getString("description", stemName);
  treeVariant.descriptions = descriptions;

  treeVariant.ephemeral = stemConfig.settings.getBool("ephemeral", false);

  treeVariant.tileDamageParameters = TileDamageParameters(
      stemConfig.settings.get("damageTable", "/plants/treeDamage.config"),
      stemConfig.settings.getFloat("health", 1.0f));

  return treeVariant;
}

StringList PlantDatabase::grassNames(bool ceiling) const {
  StringList names;
  for (auto const& pair : m_grassConfigs) {
    if (pair.second.settings.getBool("ceiling", false) == ceiling)
      names.append(pair.first);
  }
  return names;
}

GrassVariant PlantDatabase::buildGrassVariant(String const& name, float hueShift) const {
  if (!m_grassConfigs.contains(name))
    throw PlantDatabaseException(strf("grass '{}' not found in plant database", name));

  GrassVariant grassVariant;
  auto config = m_grassConfigs.get(name);

  grassVariant.name = name;
  grassVariant.directory = config.directory;
  grassVariant.images = jsonToStringList(config.settings.get("images"));
  grassVariant.hueShift = hueShift;
  grassVariant.ceiling = config.settings.getBool("ceiling", false);

  JsonObject descriptions;
  for (auto const& entry : config.settings.iterateObject()) {
    if (entry.first.endsWith("Description"))
      descriptions[entry.first] = entry.second;
  }
  descriptions["description"] = config.settings.getString("description", name);
  grassVariant.descriptions = descriptions;

  grassVariant.ephemeral = config.settings.getBool("ephemeral", true);
  grassVariant.tileDamageParameters = TileDamageParameters(
      config.settings.get("damageTable", "/plants/grassDamage.config"),
      config.settings.getFloat("health", 1.0f));

  return grassVariant;
}

StringList PlantDatabase::bushNames(bool ceiling) const {
  StringList names;
  for (auto const& pair : m_bushConfigs) {
    if (pair.second.settings.getBool("ceiling") == ceiling)
      names.append(pair.first);
  }
  return names;
}

StringList PlantDatabase::bushMods(String const& bushName) const {
  return m_bushConfigs.get(bushName).settings.opt("mods").apply(jsonToStringList).value();
}

BushVariant PlantDatabase::buildBushVariant(String const& bushName, float baseHueShift, String const& modName, float modHueShift) const {
  if (!m_bushConfigs.contains(bushName))
    throw PlantDatabaseException(strf("bush '{}' not found in plant database", bushName));

  BushVariant bushVariant;
  auto config = m_bushConfigs.get(bushName);

  bushVariant.bushName = bushName;
  bushVariant.modName = modName;
  bushVariant.directory = config.directory;
  auto shapes = config.settings.get("shapes").toArray();
  for (auto shapeVar : shapes) {
    auto shapeMap = shapeVar.toObject();
    auto base = shapeMap.get("base").toString();
    StringList mods;
    if (!modName.empty())
      mods = jsonToStringList(shapeMap.get("mods").get(modName, JsonArray()));
    bushVariant.shapes.push_back({base, mods});
  }
  bushVariant.baseHueShift = baseHueShift;
  bushVariant.modHueShift = modHueShift;
  bushVariant.ceiling = config.settings.getBool("ceiling", false);

  JsonObject descriptions;
  for (auto const& entry : config.settings.iterateObject()) {
    if (entry.first.endsWith("Description"))
      descriptions[entry.first] = entry.second;
  }
  descriptions["description"] = config.settings.getString("description", bushName + " with " + modName);
  bushVariant.descriptions = descriptions;

  bushVariant.ephemeral = config.settings.getBool("ephemeral", true);
  bushVariant.tileDamageParameters = TileDamageParameters(
      config.settings.get("damageTable", "/plants/bushDamage.config"),
      config.settings.getFloat("health", 1.0f));
  return bushVariant;
}

PlantPtr PlantDatabase::createPlant(TreeVariant const& treeVariant, uint64_t seed) const {
  try {
    return make_shared<Plant>(treeVariant, seed);
  } catch (std::exception const& e) {
    throw PlantDatabaseException(strf("Error constructing plant from tree variant stem: {} foliage: {}", treeVariant.stemName, treeVariant.foliageName), e);
  }
}

PlantPtr PlantDatabase::createPlant(GrassVariant const& grassVariant, uint64_t seed) const {
  try {
    return make_shared<Plant>(grassVariant, seed);
  } catch (std::exception const& e) {
    throw PlantDatabaseException(strf("Error constructing plant from grass variant name: {}", grassVariant.name), e);
  }
}

PlantPtr PlantDatabase::createPlant(BushVariant const& bushVariant, uint64_t seed) const {
  try {
    return make_shared<Plant>(bushVariant, seed);
  } catch (std::exception const& e) {
    throw PlantDatabaseException(
        strf("Error constructing plant from bush variant name: {} mod: {}", bushVariant.bushName, bushVariant.modName),
        e);
  }
}

}
