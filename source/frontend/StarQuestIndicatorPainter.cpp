#include "StarQuestIndicatorPainter.hpp"
#include "StarAssets.hpp"
#include "StarGuiContext.hpp"
#include "StarQuestManager.hpp"
#include "StarWorldClient.hpp"
#include "StarUniverseClient.hpp"

namespace Star {

QuestIndicatorPainter::QuestIndicatorPainter(UniverseClientPtr const& client) {
  m_client = client;
}

AnimationPtr indicatorAnimation(String indicatorPath) {
  auto assets = Root::singleton().assets();
  return make_shared<Animation>(assets->json(indicatorPath), indicatorPath);
}

void QuestIndicatorPainter::update(float dt, WorldClientPtr const& world, WorldCamera const& camera) {
  m_camera = camera;

  Set<EntityId> foundIndicators;
  for (auto const& entity : world->query<Entity>(camera.worldScreenRect())) {
    auto indicator = m_client->questManager()->getQuestIndicator(entity);
    if (!indicator) continue;

    foundIndicators.insert(entity->entityId());
    Vec2F screenPos = camera.worldToScreen(indicator->worldPosition);

    if (auto currentIndicator = m_indicators.ptr(entity->entityId())) {
      currentIndicator->screenPos = screenPos;
      if (currentIndicator->indicatorName == indicator->indicatorImage) {
        currentIndicator->animation->update(dt);
      } else {
        currentIndicator->indicatorName = indicator->indicatorImage;
        currentIndicator->animation = indicatorAnimation(indicator->indicatorImage);
      }
    } else {
      m_indicators[entity->entityId()] = Indicator {
          entity->entityId(),
          screenPos,
          indicator->indicatorImage,
          indicatorAnimation(indicator->indicatorImage)
        };
    }
  }

  m_indicators = Map<EntityId, Indicator>::from(m_indicators.pairs().filtered([&foundIndicators](pair<EntityId, Indicator> indicator) {
      return foundIndicators.contains(indicator.first);
    }));
}

Drawable QuestIndicatorPainter::Indicator::render(float pixelRatio) const {
  return animation->drawable(pixelRatio);
}

void QuestIndicatorPainter::render() {
  auto& context = GuiContext::singleton();

  for (auto const& indicator : m_indicators.values()) {
    Drawable drawable = indicator.render(m_camera.pixelRatio());
    drawable.fullbright = true;
    context.drawDrawable(drawable, Vec2F(indicator.screenPos), 1, Vec4B(255, 255, 255, 255));
  }
}

}
