#include "StarStatusPane.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarGuiReader.hpp"
#include "StarImageWidget.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"
#include "StarStatusEffectDatabase.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarImageProcessing.hpp"
#include "StarSimpleTooltip.hpp"

namespace Star {

StatusPane::StatusPane(MainInterfacePaneManager* paneManager, UniverseClientPtr client) {
  m_paneManager = paneManager;
  m_client = client;
  m_player = m_client->mainPlayer();

  m_guiContext = GuiContext::singletonPtr();
  auto assets = Root::singleton().assets();

  GuiReader reader;
  reader.construct(assets->json("/interface/windowconfig/statuspane.config:paneLayout"), this);
  disableScissoring();
}

PanePtr StatusPane::createTooltip(Vec2I const& screenPosition) {
  auto interfaceScale = m_guiContext->interfaceScale();
  for (auto const& indicator : m_statusIndicators) {
    if (indicator.screenRect.contains(Vec2F(screenPosition * interfaceScale))) {
      if (!indicator.label.empty())
        return SimpleTooltipBuilder::buildTooltip(indicator.label);
    }
  }
  return {};
}

void StatusPane::renderImpl() {
  Pane::renderImpl();

  auto assets = Root::singleton().assets();
  auto interfaceScale = m_guiContext->interfaceScale();
  auto imageMetadataDatabase = Root::singleton().imageMetadataDatabase();

  String statusIconDarkenImage = assets->json("/interface.config:statusIconDarkenImage").toString();

  for (auto const& entry : m_statusIndicators) {
    String image = entry.icon;
    if (entry.durationPercentage) {
      int imageHeight = imageMetadataDatabase->imageSize(image)[1];
      int yOffset = -(int)(*entry.durationPercentage * imageHeight);
      image += "?" + imageOperationToString(BlendImageOperation{
                         BlendImageOperation::Multiply, {statusIconDarkenImage}, Vec2I(0, yOffset)});
    }
    m_guiContext->drawQuad(image, entry.screenRect.min(), interfaceScale);
  }
}

void StatusPane::update() {
  Pane::update();

  auto assets = Root::singleton().assets();
  auto interfaceScale = m_guiContext->interfaceScale();
  int roundWindowHeight = ceil(windowHeight() / interfaceScale) * interfaceScale;

  auto imageMetadataDatabase = Root::singleton().imageMetadataDatabase();
  auto statusEffectDatabase = Root::singleton().statusEffectDatabase();

  Vec2I statusIconOffset = jsonToVec2I(assets->json("/interface.config:statusIconPos"));
  Vec2I statusIconPos = Vec2I(statusIconOffset[0] * interfaceScale, roundWindowHeight - statusIconOffset[1] * interfaceScale);
  Vec2I statusIconShift = jsonToVec2I(assets->json("/interface.config:statusIconShift")) * interfaceScale;

  RectF boundRect = RectF::null();

  m_statusIndicators.clear();
  for (auto const& pair : m_player->activeUniqueStatusEffectSummary()) {
    auto effectConfig = statusEffectDatabase->uniqueEffectConfig(pair.first);
    if (effectConfig.icon) {
      RectF rect = RectF::withSize(Vec2F(statusIconPos), Vec2F(imageMetadataDatabase->imageSize(*effectConfig.icon)) * interfaceScale);
      boundRect.combine(rect);
      m_statusIndicators.append(StatusEffectIndicator{*effectConfig.icon, pair.second, effectConfig.label, rect});
      statusIconPos += statusIconShift;
    }
  }

  setPosition(Vec2I::round(boundRect.min() / interfaceScale));
  setSize(Vec2I::round(boundRect.size() / interfaceScale));
}

}
