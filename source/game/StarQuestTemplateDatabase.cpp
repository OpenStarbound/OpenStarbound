#include "StarQuestTemplateDatabase.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

QuestTemplate::QuestTemplate(Json const& config) {
  this->config = config;
  templateId = config.getString("id");
  title = config.getString("title", "");
  text = config.getString("text", "");
  completionText = config.getString("completionText", "");
  failureText = config.getString("failureText", "");

  parameterTypes = {};
  parameterExamples = {};
  for (auto entry : config.getObject("parameters", JsonObject{})) {
    parameterTypes[entry.first] = entry.second.getString("type");
    if (entry.second.contains("example"))
      parameterExamples[entry.first] = entry.second.get("example");
  }

  moneyRange = jsonToVec2U(config.get("moneyRange", JsonArray{0, 0}));
  for (auto options : config.getArray("rewards", {})) {
    List<ItemDescriptor> rewardOption;
    for (auto entry : options.iterateArray()) {
      rewardOption.append(ItemDescriptor(entry));
    }
    rewards.append(rewardOption);
  }
  rewardParameters = config.getArray("rewardParameters", {}).transformed(mem_fn(&Json::toString));
  completionCinema = config.optString("completionCinema");
  canBeAbandoned = config.getBool("canBeAbandoned", true);
  ephemeral = config.getBool("ephemeral", false);
  showInLog = config.getBool("showInLog", true);
  showAcceptDialog = config.getBool("showAcceptDialog", true);
  showCompleteDialog = config.getBool("showCompleteDialog", true);
  showFailDialog = config.getBool("showFailDialog", true);
  mainQuest = config.getBool("mainQuest", false);
  hideCrossServer = config.getBool("hideCrossServer", false);
  questGiverIndicator = config.getString("questGiverIndicator", "questgiver");
  questReceiverIndicator = config.getString("questReceiverIndicator", "questreceiver");
  prerequisiteQuests = jsonToStringList(config.get("prerequisites", JsonArray{}));
  requiredShipLevel = config.optUInt("requiredShipLevel");
  requiredItems = config.getArray("requiredItems", {}).transformed([](Json item) { return ItemDescriptor(item); });

  updateDelta = config.getUInt("updateDelta", 10);
  script = config.optString("script");
  scriptConfig = config.getObject("scriptConfig", JsonObject{});

  if (auto guiConfigs = config.opt("guiConfigs")) {
    newQuestGuiConfig = guiConfigs->optString("newQuest");
    questCompleteGuiConfig = guiConfigs->optString("questComplete");
    questFailedGuiConfig = guiConfigs->optString("questFailed");
  }
}

QuestTemplateDatabase::QuestTemplateDatabase() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("questtemplate");
  assets->queueJsons(files);
  for (auto qt : files) {
    auto questTemplate = make_shared<QuestTemplate>(assets->json(qt));
    if (!m_templates.insert(questTemplate->templateId, questTemplate).second)
      throw StarException(strf("Duplicate quest template '{}'", questTemplate->templateId));
  }
}

List<String> QuestTemplateDatabase::allQuestTemplateIds() const {
  return m_templates.keys();
}

QuestTemplatePtr QuestTemplateDatabase::questTemplate(String const& templateId) const {
  if (!m_templates.contains(templateId)) {
    Logger::error("No quest template found for id '{}'", templateId);
    return {};
  }
  return m_templates.get(templateId);
}

}
