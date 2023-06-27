#include "StarPortraitWidget.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarAssets.hpp"

namespace Star {

PortraitWidget::PortraitWidget(PortraitEntityPtr entity, PortraitMode mode) : m_entity(entity), m_portraitMode(mode) {
  m_scale = 1;
  m_iconMode = false;

  init();
}

PortraitWidget::PortraitWidget() {
  m_entity = {};
  m_portraitMode = PortraitMode::Full;
  m_scale = 1;
  m_renderHumanoid = false;
  m_iconMode = false;

  init();
}

RectI PortraitWidget::getScissorRect() const {
  return noScissor();
}

void PortraitWidget::renderImpl() {
  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  Vec2I offset = Vec2I();
  if (m_iconMode) {
    auto imgSize = Vec2F(imgMetadata->imageSize(m_iconImage));
    offset = Vec2I(m_scale * imgSize / 2);
    offset += m_iconOffset;
    context()->drawInterfaceQuad(m_iconImage, Vec2F(screenPosition()), m_scale);
  }
  if (m_entity) {
    HumanoidPtr humanoid = nullptr;
    if (m_renderHumanoid) {
      if (auto player = as<Player>(m_entity))
        humanoid = player->humanoid();
    }

    List<Drawable> portrait = humanoid ? humanoid->render() : m_entity->portrait(m_portraitMode);
    for (auto& i : portrait) {
      i.scale(humanoid ? m_scale * 8.0f : m_scale);
      context()->drawInterfaceDrawable(i, Vec2F(screenPosition() + offset));
    }
  } else {
    if (m_portraitMode == PortraitMode::Bust || m_portraitMode == PortraitMode::Head) {
      Vec2I pos = offset;
      auto imgSize = Vec2F(imgMetadata->imageSize(m_noEntityImagePart));
      pos -= Vec2I(m_scale * imgSize * 0.5);
      context()->drawInterfaceQuad(m_noEntityImagePart, Vec2F(screenPosition() + pos), m_scale);
    } else {
      Vec2I pos = offset;
      auto imgSize = Vec2F(imgMetadata->imageSize(m_noEntityImageFull));
      pos -= Vec2I(m_scale * imgSize * 0.5);
      context()->drawInterfaceQuad(m_noEntityImageFull, Vec2F(screenPosition() + pos), m_scale);
    }
  }
}

void PortraitWidget::init() {
  auto assets = Root::singleton().assets();

  m_noEntityImageFull = assets->json("/interface.config:portraitNullPlayerImageFull").toString();
  m_noEntityImagePart = assets->json("/interface.config:portraitNullPlayerImagePart").toString();
  m_iconImage = assets->json("/interface.config:portraitIconImage").toString();
  m_iconOffset = jsonToVec2I(assets->json("/interface.config:portraitIconOffset"));

  updateSize();
}

void PortraitWidget::setEntity(PortraitEntityPtr entity) {
  m_entity = entity;
  updateSize();
}

void PortraitWidget::setMode(PortraitMode mode) {
  m_portraitMode = mode;
  updateSize();
}

void PortraitWidget::setScale(float scale) {
  m_scale = scale;
  updateSize();
}

void PortraitWidget::setIconMode() {
  m_iconMode = true;
  updateSize();
}

void PortraitWidget::setRenderHumanoid(bool renderHumanoid) {
  m_renderHumanoid = renderHumanoid;
}

bool PortraitWidget::sendEvent(InputEvent const&) {
  return false;
}

void PortraitWidget::updateSize() {
  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  if (m_iconMode) {
    setSize(Vec2I(imgMetadata->imageSize(m_iconImage) * m_scale));
  } else {
    if (m_entity) {
      setSize(Vec2I(
          (Drawable::boundBoxAll(m_entity->portrait(m_portraitMode), false)
                  .size()
              * TilePixels
              * m_scale)
              .ceil()));
    } else {
      if (m_portraitMode == PortraitMode::Bust || m_portraitMode == PortraitMode::Head)
        setSize(Vec2I(imgMetadata->imageSize(m_iconImage) * m_scale));
      else
        setSize(Vec2I(imgMetadata->imageSize(m_noEntityImageFull) * m_scale));
    }
  }
}

}
