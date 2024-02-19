#include "StarNameplatePainter.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarNametagEntity.hpp"
#include "StarPlayer.hpp"
#include "StarGuiContext.hpp"

namespace Star {

NameplatePainter::NameplatePainter() {
  auto assets = Root::singleton().assets();

  Json nametagConfig = assets->json("/interface.config:nametag");
  m_showMasterNames = nametagConfig.getBool("showMasterNames");
  m_opacityRate = nametagConfig.getFloat("opacityRate");
  m_inspectOpacityRate = nametagConfig.queryFloat("inspectOpacityRate", m_opacityRate);
  m_offset = jsonToVec2F(nametagConfig.get("offset"));
  m_font = nametagConfig.queryString("font", "");
  m_fontDirectives = nametagConfig.queryString("fontDirectives", "");
  m_fontSize = nametagConfig.getFloat("fontSize");
  m_statusFont = nametagConfig.queryString("font", m_font);
  m_statusFontDirectives = nametagConfig.queryString("fontDirectives", m_fontDirectives);
  m_statusFontSize = nametagConfig.queryFloat("statusFontSize", m_fontSize);
  m_statusOffset = jsonToVec2F(nametagConfig.get("statusOffset"));
  m_statusColor = jsonToColor(nametagConfig.get("statusColor"));
  m_opacityBoost = nametagConfig.getFloat("opacityBoost");
  m_nametags.setTweenFactor(nametagConfig.getFloat("tweenFactor"));
  m_nametags.setMovementThreshold(nametagConfig.getFloat("movementThreshold"));
}

void NameplatePainter::update(float dt, WorldClientPtr const& world, WorldCamera const& camera, bool inspectionMode) {
  m_camera = camera;

  Set<EntityId> foundEntities;
  for (auto const& entity : world->query<NametagEntity>(camera.worldScreenRect())) {
    if ((entity->isMaster() && !m_showMasterNames) || !entity->displayNametag())
      continue;
    if (auto player = as<Player>(entity)) {
      if (player->isTeleporting())
        continue;
    }
    foundEntities.insert(entity->entityId());

    if (!m_entitiesWithNametags.contains(entity->entityId())) {
      Nametag nametag = {entity->name(), entity->statusText(), entity->nametagColor(), 1.0f, entity->entityId()};
      RectF boundBox = determineBoundBox(Vec2F(), nametag);
      m_nametags.addBubble(Vec2F(), boundBox, std::move(nametag));
    }
  }

  m_nametags.filter([&foundEntities](
      BubbleState<Nametag> const&, Nametag const& nametag) { return foundEntities.contains(nametag.entityId); });

  m_nametags.forEach([&world, &camera, this, inspectionMode](BubbleState<Nametag>& bubbleState, Nametag& nametag) {
    if (auto entity = as<NametagEntity>(world->entity(nametag.entityId))) {
      bubbleState.idealDestination = camera.worldToScreen(entity->nametagOrigin()) + m_offset * camera.pixelRatio();
      bubbleState.boundBox = determineBoundBox(bubbleState.idealDestination, nametag);

      nametag.statusText = entity->statusText();
      nametag.name = entity->name();
      nametag.color = entity->nametagColor();
      bool fullyOnScreen = world->geometry().rectContains(camera.worldScreenRect(), entity->position());
      if (inspectionMode)
        nametag.opacity = approach(1.0f, nametag.opacity, m_inspectOpacityRate);
      else if (fullyOnScreen)
        nametag.opacity = approach(0.0f, nametag.opacity, m_opacityRate);
      else
        nametag.opacity = approach(m_opacityBoost, nametag.opacity, m_opacityRate);
    }
  });

  m_entitiesWithNametags = std::move(foundEntities);
  m_nametags.update(dt);
}

void NameplatePainter::render() {
  auto& context = GuiContext::singleton();

  m_nametags.forEach([&context, this](BubbleState<Nametag> const& bubble, Nametag const& nametag) {
    if (nametag.opacity == 0.0f)
      return;

    context.setFont(m_font);
    context.setFontProcessingDirectives(m_fontDirectives);
    context.setFontSize(m_fontSize);

    auto color = Color::rgb(nametag.color);
    color.setAlphaF(nametag.opacity);
    context.setFontColor(color.toRgba());
    context.setFontMode(FontMode::Normal);

    context.renderText(nametag.name, namePosition(bubble.currentPosition));

    if (nametag.statusText) {
      auto statusColor = m_statusColor;
      statusColor.setAlphaF(nametag.opacity);
      context.setFontColor(statusColor.toRgba());

      context.setFontSize(m_statusFontSize);
      context.setFontProcessingDirectives(m_statusFontDirectives);
      context.setFont(m_statusFont);

      context.renderText(*nametag.statusText, statusPosition(bubble.currentPosition));
    }
    
    context.setDefaultFont();
    context.setFontProcessingDirectives("");
  });
}

TextPositioning NameplatePainter::namePosition(Vec2F bubblePosition) const {
  return TextPositioning(bubblePosition, HorizontalAnchor::HMidAnchor, VerticalAnchor::BottomAnchor);
}

TextPositioning NameplatePainter::statusPosition(Vec2F bubblePosition) const {
  auto& context = GuiContext::singleton();
  return TextPositioning(
      bubblePosition + m_statusOffset * context.interfaceScale(),
      HorizontalAnchor::HMidAnchor, VerticalAnchor::BottomAnchor);
}

RectF NameplatePainter::determineBoundBox(Vec2F bubblePosition, Nametag const& nametag) const {
  auto& context = GuiContext::singleton();
  context.setFontSize(m_fontSize);
  context.setFontProcessingDirectives(m_fontDirectives);
  context.setFont(m_font);
  RectF nametagBox = context.determineTextSize(nametag.name, namePosition(bubblePosition));
  if (nametag.statusText) {
    context.setFontSize(m_statusFontSize);
    context.setFontProcessingDirectives(m_statusFontDirectives);
    context.setFont(m_statusFont);
    nametagBox.combine(context.determineTextSize(*nametag.statusText, statusPosition(bubblePosition)));
  }
  return nametagBox;
}

}
