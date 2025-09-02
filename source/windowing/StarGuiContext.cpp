#include "StarGuiContext.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarMixer.hpp"
#include "StarAssets.hpp"
#include "StarImageMetadataDatabase.hpp"
#include <iostream>

namespace Star {

GuiContext* GuiContext::s_singleton;

GuiContext* GuiContext::singletonPtr() {
  return s_singleton;
}

GuiContext& GuiContext::singleton() {
  if (!s_singleton)
    throw GuiContextException("GuiContext::singleton() called with no GuiContext instance available");
  else
    return *s_singleton;
}

GuiContext::GuiContext(MixerPtr mixer, ApplicationControllerPtr appController) {
  if (s_singleton)
    throw GuiContextException("Singleton GuiContext has been constructed twice");

  s_singleton = this;

  m_mixer = std::move(mixer);
  m_applicationController = std::move(appController);

  m_interfaceScale = 1;

  m_shiftHeld = false;

  refreshKeybindings();
}

GuiContext::~GuiContext() {
  s_singleton = nullptr;
}

void GuiContext::renderInit(RendererPtr renderer) {
  m_renderer = std::move(renderer);
  auto textureGroup = m_renderer->createTextureGroup();
  m_textureCollection = make_shared<AssetTextureGroup>(textureGroup);
  m_drawablePainter = make_shared<DrawablePainter>(m_renderer, m_textureCollection);
  m_textPainter = make_shared<TextPainter>(m_renderer, textureGroup);
}

MixerPtr const& GuiContext::mixer() const {
  return m_mixer;
}

ApplicationControllerPtr const& GuiContext::applicationController() const {
  return m_applicationController;
}

RendererPtr const& GuiContext::renderer() const {
  if (!m_renderer)
    throw GuiContextException("GuiContext::renderer() called before renderInit");
  return m_renderer;
}

AssetTextureGroupPtr const& GuiContext::assetTextureGroup() const {
  if (!m_textureCollection)
    throw GuiContextException("GuiContext::assetTextureGroup() called before renderInit");
  return m_textureCollection;
}

TextPainterPtr const& GuiContext::textPainter() const {
  if (!m_textPainter)
    throw GuiContextException("GuiContext::textPainter() called before renderInit");
  return m_textPainter;
}

unsigned GuiContext::windowWidth() const {
  return renderer()->screenSize()[0];
}

unsigned GuiContext::windowHeight() const {
  return renderer()->screenSize()[1];
}

Vec2U GuiContext::windowSize() const {
  return renderer()->screenSize();
}

Vec2U GuiContext::windowInterfaceSize() const {
  return Vec2U::ceil(Vec2F(windowSize()) / interfaceScale());
}

float GuiContext::interfaceScale() const {
  return m_interfaceScale;
}

void GuiContext::setInterfaceScale(float interfaceScale) {
  m_interfaceScale = interfaceScale;
}

Maybe<Vec2I> GuiContext::mousePosition(InputEvent const& event, float pixelRatio) const {
  auto getInterfacePosition = [pixelRatio](Vec2F pos) {
    return Vec2I(pos / pixelRatio);
  };

  if (auto mouseMoveEvent = event.ptr<MouseMoveEvent>())
    return getInterfacePosition(mouseMoveEvent->mousePosition);
  else if (auto mouseDownEvent = event.ptr<MouseButtonDownEvent>())
    return getInterfacePosition(mouseDownEvent->mousePosition);
  else if (auto mouseUpEvent = event.ptr<MouseButtonUpEvent>())
    return getInterfacePosition(mouseUpEvent->mousePosition);
  else if (auto mouseWheelEvent = event.ptr<MouseWheelEvent>())
    return getInterfacePosition(mouseWheelEvent->mousePosition);
  else
    return {};
}

Maybe<Vec2I> GuiContext::mousePosition(InputEvent const& event) const {
  return mousePosition(event, interfaceScale());
}

Set<InterfaceAction> GuiContext::actions(InputEvent const& event) const {
  return m_keyBindings.actions(event);
}

Set<InterfaceAction> GuiContext::actionsForKey(Key key) const {
  return m_keyBindings.actionsForKey(key);
}

void GuiContext::refreshKeybindings() {
  m_keyBindings = KeyBindings(Root::singleton().configuration()->get("bindings"));
}

void GuiContext::setInterfaceScissorRect(RectI const& scissor) {
  renderer()->setScissorRect(scissor.scaled(interfaceScale()));
}

void GuiContext::resetInterfaceScissorRect() {
  renderer()->setScissorRect({});
}

Vec2U GuiContext::textureSize(AssetPath const& texName) {
  return assetTextureGroup()->loadTexture(texName)->size();
}

void GuiContext::drawQuad(RectF const& screenCoords, Vec4B const& color) {
  renderer()->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), screenCoords, color, 0.0f);
}

void GuiContext::drawQuad(AssetPath const& texName, RectF const& screenCoords, Vec4B const& color) {
  renderer()->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), assetTextureGroup()->loadTexture(texName), screenCoords, color, 0.0f);
}

void GuiContext::drawQuad(AssetPath const& texName, Vec2F const& screenPos, float pixelRatio, Vec4B const& color) {
  renderer()->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), assetTextureGroup()->loadTexture(texName), screenPos, pixelRatio, color, 0.0f);
}

void GuiContext::drawQuad(AssetPath const& texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color) {
  renderer()->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), assetTextureGroup()->loadTexture(texName),
      screenCoords.min(), texCoords.min(),
      Vec2F(screenCoords.xMax(), screenCoords.yMin()), Vec2F(texCoords.xMax(), texCoords.yMin()),
      screenCoords.max(), texCoords.max(),
      Vec2F(screenCoords.xMin(), screenCoords.yMax()), Vec2F(texCoords.xMin(), texCoords.yMax()),
    color, 0.0f);
}

void GuiContext::drawDrawable(Drawable drawable, Vec2F const& screenPos, float pixelRatio, Vec4B const& color) {
  if (drawable.isLine())
    drawable.linePart().width *= pixelRatio;

  drawable.scale(pixelRatio);
  drawable.translate(screenPos);
  drawable.color *= Color::rgba(color);
  m_drawablePainter->drawDrawable(drawable);
}

void GuiContext::drawLine(Vec2F const& begin, Vec2F const end, Vec4B const& color, float lineWidth) {
  Vec2F left = vnorm(Vec2F(end) - Vec2F(begin)).rot90() * lineWidth / 2.0f;
  renderer()->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(),
    begin + left,
    begin - left,
    end - left,
    end + left,
    color, 0.0f);
}

void GuiContext::drawPolyLines(PolyF const& poly, Vec4B const& color, float lineWidth) {
  for (size_t i = 0; i < poly.sides(); ++i)
    drawLine(poly.vertex(i), poly.vertex(i + 1), color, lineWidth);
}

void GuiContext::drawTriangles(List<tuple<Vec2F, Vec2F, Vec2F>> const& triangles, Vec4B const& color) {
  for (auto& poly : triangles) {
    renderer()->immediatePrimitives().emplace_back(std::in_place_type_t<RenderTriangle>(),
      get<0>(poly), get<1>(poly), get<2>(poly), color, 0.0f);
  }
}

void GuiContext::drawInterfaceDrawable(Drawable drawable, Vec2F const& screenPos, Vec4B const& color) {
  drawDrawable(std::move(drawable), screenPos * interfaceScale(), interfaceScale(), color);
}

void GuiContext::drawInterfaceLine(Vec2F const& begin, Vec2F const end, Vec4B const& color, float lineWidth) {
  drawLine(begin * interfaceScale(), end * interfaceScale(), color, lineWidth * interfaceScale());
}

void GuiContext::drawInterfacePolyLines(PolyF poly, Vec4B const& color, float lineWidth) {
  poly.scale(interfaceScale());
  drawPolyLines(poly, color, lineWidth * interfaceScale());
}

void GuiContext::drawInterfaceQuad(RectF const& screenCoords, Vec4B const& color) {
  drawQuad(screenCoords.scaled(interfaceScale()), color);
}

void GuiContext::drawInterfaceQuad(AssetPath const& texName, Vec2F const& screenCoords, Vec4B const& color) {
  drawQuad(texName, screenCoords * interfaceScale(), interfaceScale(), color);
}

void GuiContext::drawInterfaceQuad(AssetPath const& texName, Vec2F const& screenCoords, float scale, Vec4B const& color) {
  drawQuad(texName, screenCoords * interfaceScale(), interfaceScale() * scale, color);
}

void GuiContext::drawInterfaceQuad(AssetPath const& texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color) {
  drawQuad(texName, texCoords, screenCoords.scaled(interfaceScale()), color);
}

void GuiContext::drawInterfaceTriangles(List<tuple<Vec2F, Vec2F, Vec2F>> const& triangles, Vec4B const& color) {
  drawTriangles(triangles.transformed([this](tuple<Vec2F, Vec2F, Vec2F> const& poly) {
    return tuple<Vec2F, Vec2F, Vec2F>(get<0>(poly) * interfaceScale(), get<1>(poly) * interfaceScale(), get<2>(poly) * interfaceScale());
  }), color);
}

void GuiContext::drawImageStretchSet(ImageStretchSet const& imageSet, RectF const& screenPos, GuiDirection direction, Vec4B const& color) {
  int innerSize;
  int innerOffset = 0;
  RectF begin, end, inner;
  if (direction == GuiDirection::Horizontal) {
    innerSize = screenPos.width();
    if (imageSet.begin.size())
      innerOffset = textureSize(imageSet.begin)[0];

    if (imageSet.end.size())
      innerSize = std::max(0, innerSize - innerOffset - Vec2I(textureSize(imageSet.end))[0]);
    else
      innerSize = std::max(0, innerSize - innerOffset);

    if (imageSet.begin.size())
      begin = RectF::withSize(screenPos.min(), Vec2F(textureSize(imageSet.begin)[0], screenPos.height()));
    else
      begin = RectF::withSize(screenPos.min(), Vec2F(0, screenPos.height()));

    inner = RectF::withSize(screenPos.min() + Vec2F(innerOffset, 0), Vec2F(innerSize, screenPos.height()));
    if (imageSet.end.size())
      end = RectF::withSize(screenPos.min() + Vec2F(innerOffset + innerSize, 0), Vec2F(textureSize(imageSet.end)[0], screenPos.height()));
    else
      end = RectF::withSize(screenPos.min(), Vec2F(0, screenPos.height()));

  } else {
    innerSize = screenPos.height();
    if (imageSet.begin.size())
      innerOffset = textureSize(imageSet.begin)[1];

    if (imageSet.end.size())
      innerSize = std::max(0, innerSize - innerOffset - Vec2I(textureSize(imageSet.end))[1]);
    else
      innerSize = std::max(0, innerSize - innerOffset);

    if (imageSet.begin.size())
      begin = RectF::withSize(screenPos.min(), Vec2F(screenPos.width(), textureSize(imageSet.begin)[1]));
    else
      begin = RectF::withSize(screenPos.min(), Vec2F(screenPos.width(), 0));

    inner = RectF::withSize(screenPos.min() + Vec2F(0, innerOffset), Vec2F(screenPos.width(), innerSize));
    if (imageSet.end.size()) {
      end = RectF::withSize(screenPos.min() + Vec2F(0, innerOffset + innerSize), Vec2F(screenPos.width(), textureSize(imageSet.end)[1]));
    } else {
      end = RectF::withSize(screenPos.min(), Vec2F(screenPos.width(), 0));
    }
  }

  if (imageSet.begin.size())
    drawInterfaceQuad(imageSet.begin, RectF(Vec2F(), Vec2F(textureSize(imageSet.begin))), begin, color);

  if (imageSet.type == ImageStretchSet::ImageStretchType::Stretch) {
    drawInterfaceQuad(imageSet.inner, RectF(Vec2F(), Vec2F(textureSize(imageSet.inner))), inner, color);

  } else {
    int position = 0;
    auto texSize = Vec2F(textureSize(imageSet.inner));
    if (direction == GuiDirection::Horizontal) {
      starAssert(texSize[0] > 0);
      while (position < inner.width()) {
        RectF partialImage = RectF::withSize(Vec2F(), Vec2F(std::min(inner.width() - position, texSize[0]), texSize[1]));
        drawInterfaceQuad(imageSet.inner, partialImage, RectF::withSize(inner.min() + Vec2F(position, 0), partialImage.size()), color);
        position += partialImage.size()[0];
      }
    } else {
      starAssert(texSize[1] > 0);
      while (position < inner.height()) {
        RectF partialImage = RectF::withSize(
            Vec2F(0, max(0.0f, texSize[1] - (inner.height() - position))),
            Vec2F(texSize[0], std::min(inner.height() - position, texSize[1])));
        drawInterfaceQuad(imageSet.inner, partialImage, RectF::withSize(inner.min() + Vec2F(0, position), partialImage.size()), color);
        position += partialImage.size()[1];
      }
    }
  }

  if (imageSet.end.size())
    drawInterfaceQuad(imageSet.end, RectF(Vec2F(), Vec2F(textureSize(imageSet.end))), end, color);
}

bool GuiContext::trySetCursor(Drawable const& drawable, Vec2I const& offset, int pixelRatio) {
  if (!drawable.isImage())
    return false;

  auto assets = Root::singleton().assets();
  auto& imagePath = drawable.imagePart().image;
  return applicationController()->setCursorImage(AssetPath::join(imagePath), assets->image(imagePath), pixelRatio, offset);
}

RectF GuiContext::renderText(String const& s, TextPositioning const& position) {
  return textPainter()->renderText(s, position);
}

RectF GuiContext::renderInterfaceText(String const& s, TextPositioning const& position) {
  auto res = renderText(s, {
      position.pos * interfaceScale(),
      position.hAnchor,
      position.vAnchor,
      position.wrapWidth.apply(bind(std::multiplies<int>(), _1, interfaceScale())),
      position.charLimit
    });
  return RectF(res).scaled(1.0f / interfaceScale());
}

RectF GuiContext::determineTextSize(String const& s, TextPositioning const& positioning) {
  return textPainter()->determineTextSize(s, positioning);
}

RectF GuiContext::determineInterfaceTextSize(String const& s, TextPositioning const& positioning) {
  auto res = determineTextSize(s, {
      positioning.pos * interfaceScale(),
      positioning.hAnchor,
      positioning.vAnchor,
      positioning.wrapWidth.apply(bind(std::multiplies<int>(), _1, interfaceScale()))
    });
  return RectF(res).scaled(1.0f / interfaceScale());
}

void GuiContext::setFontSize(unsigned size) {
  setFontSize(size, interfaceScale());
}

void GuiContext::setFontSize(unsigned size, float pixelRatio) {
  textPainter()->setFontSize(size * pixelRatio);
}

void GuiContext::setFontColor(Vec4B const& color) {
  textPainter()->setFontColor(color);
}

void GuiContext::setFontMode(FontMode mode) {
  textPainter()->setMode(mode);
}

void GuiContext::setFontProcessingDirectives(String const& directives) {
  textPainter()->setProcessingDirectives(directives);
}

void GuiContext::setFont(String const& font) {
  textPainter()->setFont(font);
}

void GuiContext::setDefaultFont() {
  textPainter()->setFont("");
}

TextStyle& GuiContext::setTextStyle(TextStyle const& textStyle, int pixelRatio) {
  TextStyle& setStyle = textPainter()->setTextStyle(textStyle);
  setStyle.fontSize *= pixelRatio;
  return setStyle;
}

TextStyle& GuiContext::setTextStyle(TextStyle const& textStyle) {
  return setTextStyle(textStyle, interfaceScale());
}

void GuiContext::clearTextStyle() {
  textPainter()->clearTextStyle();
}

void GuiContext::setLineSpacing(float lineSpacing) {
  textPainter()->setLineSpacing(lineSpacing);
}

void GuiContext::setDefaultLineSpacing() {
  textPainter()->setLineSpacing(DefaultLineSpacing);
}

int GuiContext::stringWidth(String const& s) {
  return textPainter()->stringWidth(s);
}

//TODO: Make this use StringView
int GuiContext::stringInterfaceWidth(String const& s) {
  if (interfaceScale() != 0) {
    // font size is already adjusted UP by interfaceScale, so we have to adjust
    // it back down
    return stringWidth(s) / interfaceScale();
  }
  return 0;
}

StringList GuiContext::wrapText(String const& s, Maybe<unsigned> wrapWidth) {
  return textPainter()->wrapText(s, wrapWidth);
}

StringList GuiContext::wrapInterfaceText(String const& s, Maybe<unsigned> wrapWidth) {
  if (wrapWidth)
    *wrapWidth *= interfaceScale();
  return wrapText(s, wrapWidth);
}

bool GuiContext::shiftHeld() const {
  return m_shiftHeld;
}

void GuiContext::setShiftHeld(bool held) {
  m_shiftHeld = held;
}

void GuiContext::playAudio(AudioInstancePtr audioInstance) {
  m_mixer->play(audioInstance);
}

void GuiContext::playAudio(String const& audioAsset, int loops, float volume, float pitch) {
  auto assets = Root::singleton().assets();
  auto config = Root::singleton().configuration();
  auto audioInstance = make_shared<AudioInstance>(*assets->audio(audioAsset));
  audioInstance->setVolume(volume);
  audioInstance->setPitchMultiplier(pitch);
  audioInstance->setLoops(loops);
  m_mixer->play(std::move(audioInstance));
}

String GuiContext::getClipboard() const {
  return m_applicationController->getClipboard().value();
}

bool GuiContext::setClipboard(String text) {
  return m_applicationController->setClipboard(std::move(text));
}

bool GuiContext::setClipboardData(StringMap<ByteArray> data) {
  return m_applicationController->setClipboardData(std::move(data));
}

bool GuiContext::setClipboardImage(Image const& image, ByteArray* png, String const* path) {
  return m_applicationController->setClipboardImage(image, png, path);
}

bool GuiContext::setClipboardFile(String const& path) {
  return m_applicationController->setClipboardFile(path);
}

void GuiContext::cleanup() {
  int64_t textureTimeout = Root::singleton().assets()->json("/rendering.config:textureTimeout").toInt();
  if (m_textureCollection)
    m_textureCollection->cleanup(textureTimeout);
  if (m_textPainter)
    m_textPainter->cleanup(textureTimeout);
}

}
