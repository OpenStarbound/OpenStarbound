#include "StarStagehandDatabase.hpp"
#include "StarStagehand.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

StagehandDatabase::StagehandDatabase() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("stagehand");
  assets->queueJsons(files);
  for (auto file : files) {
    try {
      auto config = assets->json(file);

      String typeName = config.getString("type");

      if (m_stagehandTypes.contains(typeName))
        throw StagehandDatabaseException(strf("Repeat stagehand type name '{}'", typeName));

      m_stagehandTypes[typeName] = config;

    } catch (StarException const& e) {
      throw StagehandDatabaseException(strf("Error loading stagehand type '{}'", file), e);
    }
  }
}

StagehandPtr StagehandDatabase::createStagehand(String const& stagehandType, Json const& extraConfig) const {
  auto finalConfig = jsonMerge(m_stagehandTypes.get(stagehandType), extraConfig);
  return make_shared<Stagehand>(finalConfig);
}

}
