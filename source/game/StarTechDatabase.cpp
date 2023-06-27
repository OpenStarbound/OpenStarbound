#include "StarTechDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

EnumMap<TechType> const TechTypeNames{
  {TechType::Head, "Head"},
  {TechType::Body, "Body"},
  {TechType::Legs, "Legs"}
};

TechDatabase::TechDatabase() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("tech");
  assets->queueJsons(files);
  for (auto file : files) {
    auto tech = parseTech(assets->json(file), file);

    if (m_tech.contains(tech.name))
      throw TechDatabaseException::format("Duplicate tech named '{}', config file '{}'", tech.name, file);
    m_tech[tech.name] = tech;
  }
}

bool TechDatabase::contains(String const& techName) const {
  return m_tech.contains(techName);
}

TechConfig TechDatabase::tech(String const& techName) const {
  if (auto p = m_tech.ptr(techName))
    return *p;
  throw TechDatabaseException::format("No such tech '{}'", techName);
}

TechConfig TechDatabase::parseTech(Json const& config, String const& path) const {
  try {
    auto assets = Root::singleton().assets();

    TechConfig tech;
    tech.name = config.getString("name");
    tech.path = path;
    tech.parameters = config;

    tech.type = TechTypeNames.getLeft(config.getString("type"));

    tech.scripts = jsonToStringList(config.get("scripts")).transformed(bind(AssetPath::relativeTo, path, _1));
    tech.animationConfig = config.optString("animator").apply(bind(&AssetPath::relativeTo, path, _1));

    tech.description = config.getString("description");
    tech.shortDescription = config.getString("shortDescription");
    tech.rarity = RarityNames.getLeft(config.getString("rarity"));
    tech.icon = AssetPath::relativeTo(path, config.getString("icon"));

    return tech;
  } catch (std::exception const& e) {
    throw TechDatabaseException(strf("Error reading tech config {}", path), e);
  }
}

}
