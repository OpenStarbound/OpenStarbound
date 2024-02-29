#include "StarCinematic.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarWorldClient.hpp"
#include "StarAssets.hpp"
#include "StarGuiContext.hpp"
#include "StarPlayer.hpp"

namespace Star {

const float vWidth = 960.0f;
const float vHeight = 540.0f;

Cinematic::Cinematic() {
  m_completable = false;
  m_suppressInput = false;
}

void Cinematic::load(Json const& definition) {
  stop();

  for (auto timeSkipDefinition : definition.getArray("timeSkips", JsonArray())) {
    TimeSkip timeSkip;
    timeSkip.availableTime = timeSkipDefinition.getFloat("available");
    timeSkip.skipToTime = timeSkipDefinition.getFloat("skipTo");
    m_timeSkips.append(timeSkip);
  }
  sort(m_timeSkips, [](TimeSkip const& a, TimeSkip const& b) -> bool {
      return a.availableTime < b.availableTime;
    });

  for (auto cameraDefinition : definition.getArray("camera", JsonArray())) {
    CameraKeyFrame keyFrame;
    keyFrame.timecode = cameraDefinition.getFloat("timecode");
    keyFrame.zoom = cameraDefinition.getFloat("zoom", 1.0f);
    if (cameraDefinition.contains("pan"))
      keyFrame.pan = jsonToVec2F(cameraDefinition.get("pan"));
    m_cameraKeyFrames.append(keyFrame);
  }

  for (auto panelDefinition : definition.getArray("panels")) {
    PanelPtr panel = make_shared<Panel>();
    panel->useCamera = panelDefinition.getBool("useCamera", true);
    panel->drawables = panelDefinition.getArray("drawables", {});
    panel->animationFrames = panelDefinition.getInt("animationFrames", std::numeric_limits<int>::max());
    panel->text = panelDefinition.getString("text", "");
    panel->textPosition = TextPositioning(panelDefinition.get("textPosition", JsonObject()));
    panel->fontColor = panelDefinition.opt("fontColor").apply(jsonToVec4B).value(Vec4B(255, 255, 255, 255));
    panel->fontSize = panelDefinition.getUInt("fontSize", 8);
    panel->avatar = panelDefinition.getString("avatar", "");
    panel->startTime = panelDefinition.getFloat("startTime", 0);
    panel->endTime = panelDefinition.getFloat("endTime", 0);
    panel->loopTime = panelDefinition.getFloat("loopTime", 0);
    for (auto keyframeDefinition : panelDefinition.getArray("keyframes")) {
      KeyFrame keyframe;
      keyframe.timecode = keyframeDefinition.getFloat("timecode");
      keyframe.command = keyframeDefinition;
      panel->keyFrames.append(keyframe);
      float endTimecode = panel->endTime > 0 ? std::min(panel->endTime, panel->startTime + keyframe.timecode) : panel->startTime + keyframe.timecode;
      m_completionTime = std::max(m_completionTime, endTimecode);
    }
    m_panels.append(panel);
  }

  for (auto audioDefinition : definition.getArray("audio")) {
    AudioCue cue;
    cue.timecode = audioDefinition.getFloat("timecode");
    cue.loops = audioDefinition.getInt("loops", 0);
    cue.endTimecode = audioDefinition.getFloat("endTimecode", 0);
    cue.resource = audioDefinition.getString("resource");
    m_audioCues.append(cue);
    m_completionTime = std::max(m_completionTime, cue.timecode);
    m_completionTime = std::max(m_completionTime, cue.endTimecode);
  }
  m_activeAudio = std::vector<AudioInstancePtr>(m_audioCues.size());

  if (definition.contains("offset"))
    m_offset = jsonToVec2F(definition.get("offset"));
  else
    m_offset = {};

  if (definition.contains("backgroundColor"))
    m_backgroundColor = jsonToVec4B(definition.get("backgroundColor"));
  else
    m_backgroundColor = {};
  m_backgroundFadeTime = definition.getFloat("backgroundFadeTime", 0);

  m_completionTime += 2 * m_backgroundFadeTime;

  m_scissor = definition.getBool("scissor", true);
  m_letterbox = definition.getBool("letterbox", true);
  m_skippable = definition.getBool("skippable", true);
  m_suppressInput = definition.getBool("suppressInput", true);
  m_muteSfx = definition.getBool("muteSfx", false);
  m_muteMusic = definition.getBool("muteMusic", false);

  m_timer.reset();
  m_timer.start();
}

void Cinematic::setPlayer(PlayerPtr player) {
  m_player = player;
}

void Cinematic::update(float) {
  m_currentTimeSkip = {};
  for (auto timeSkip : m_timeSkips) {
    if (currentTimecode() >= timeSkip.availableTime && currentTimecode() < timeSkip.skipToTime)
      m_currentTimeSkip = timeSkip;
  }
}

bool Cinematic::completed() const {
  for (size_t i = 0; i < m_audioCues.size(); ++i) {
    if (m_activeAudio[i] && !m_activeAudio[i]->finished())
      return false;
  }

  return m_timer.time() >= m_completionTime;
}

bool Cinematic::completable() const {
  return m_completable;
}

void Cinematic::render() {
  if (completed())
    return;

  auto& guiContext = GuiContext::singleton();
  auto mixer = guiContext.mixer();
  auto renderer = guiContext.renderer();
  auto textPainter = guiContext.textPainter();

  m_windowSize = Vec2F(renderer->screenSize());
  Vec2F screenWindowSize = Vec2F(vWidth * m_drawableScale, vHeight * m_drawableScale);
  m_drawableScale = std::min(m_windowSize[0] / vWidth, m_windowSize[1] / vHeight);
  m_drawableTranslation = Vec2F(0.5f * (m_windowSize[0] - vWidth * m_drawableScale), 0.5f * (m_windowSize[1] - vHeight * m_drawableScale));
  m_scissorRect = RectI::withSize(Vec2I::floor((m_windowSize / 2) - (screenWindowSize / 2)), Vec2I::ceil(screenWindowSize));

  updateCamera(m_timer.time());

  float fadeFactor = 1.0;
  if (m_backgroundFadeTime > 0) {
    if (m_timer.time() < m_backgroundFadeTime)
      fadeFactor = m_timer.time() / m_backgroundFadeTime;
    else if (m_completionTime - m_timer.time() < m_backgroundFadeTime)
      fadeFactor = max<float>(0.0f, m_completionTime - m_timer.time()) / m_backgroundFadeTime;
  }

  if (m_backgroundColor) {
    Vec4B backgroundColor = m_backgroundColor.get();
    backgroundColor[3] *= fadeFactor;
    renderer->render(renderFlatRect(RectF::withSize(Vec2F(0, 0), m_windowSize), backgroundColor, 0.0f));
  }

  if (m_letterbox && !m_backgroundColor) {
    Vec4B letterboxColor = Vec4B(0, 0, 0, 255 * fadeFactor);
    if (m_windowSize[0] / vWidth > m_windowSize[1] / vHeight) {
      renderer->render(renderFlatRect(RectF(0, 0, m_scissorRect.xMin(), m_windowSize[1]), letterboxColor, 0.0f));
      renderer->render(renderFlatRect(RectF(m_scissorRect.xMax(), 0, m_windowSize[0], m_windowSize[1]), letterboxColor, 0.0f));
    } else {
      renderer->render(renderFlatRect(RectF(0, 0, m_windowSize[0], m_scissorRect.yMin()), letterboxColor, 0.0f));
      renderer->render(renderFlatRect(RectF(0, m_scissorRect.yMax(), m_windowSize[0], m_windowSize[1]), letterboxColor, 0.0f));
    }
  }

  if (fadeFactor < 1.0f)
    return;

  if (m_scissor)
    renderer->setScissorRect(m_scissorRect);

  String playerSpecies = "";
  if (m_player)
    playerSpecies = m_player->species();

  for (auto panel : m_panels) {
    float drawableScale = m_drawableScale;
    Vec2F drawableTranslation = m_drawableTranslation;
    if (panel->useCamera) {
      drawableScale *= m_cameraZoom;
      drawableTranslation += m_cameraPan * drawableScale;
    }

    auto values = determinePanelValues(panel, currentTimecode());
    if (values.completable)
      m_completable = true;
    if (!values.alpha)
      continue;
    auto frame = toString(((int)values.frame) % panel->animationFrames);
    auto alphaColor = Color::rgbaf(1.0f, 1.0f, 1.0f, values.alpha);
    for (auto const& d : panel->drawables) {
      Drawable drawable = Drawable(d.set("image", d.getString("image").replaceTags(StringMap<String>{{"species", playerSpecies}, {"frame", frame}})));
      drawable.translate(m_offset);
      drawable.scale(values.zoom);
      drawable.translate(values.position);
      drawable.color *= alphaColor;
      drawDrawable(std::move(drawable), drawableScale, drawableTranslation);
    }
    if (!panel->avatar.empty() && m_player) {
      for (auto drawable : m_player->portrait(PortraitModeNames.getLeft(panel->avatar))) {
        drawable.translate(m_offset);
        drawable.scale(values.zoom);
        drawable.translate(values.position);
        drawable.color *= alphaColor;
        drawDrawable(std::move(drawable), drawableScale, drawableTranslation);
      }
    }
    if (!panel->text.empty()) {
      textPainter->setFontSize(floor(panel->fontSize * drawableScale));
      auto fontColor = panel->fontColor;
      fontColor[3] *= values.alpha;
      textPainter->setFontColor(fontColor);
      Vec2F position = (m_offset + values.position + Vec2F(panel->textPosition.pos)) * drawableScale + drawableTranslation;
      TextPositioning tp = TextPositioning(position, panel->textPosition.hAnchor, panel->textPosition.vAnchor, {}, {});
      if (panel->textPosition.wrapWidth)
        tp.wrapWidth = floor(panel->textPosition.wrapWidth.get() * drawableScale);
      if (values.textPercentage < 1.0)
        tp.charLimit = floor(panel->text.length() * values.textPercentage);
      textPainter->renderText(panel->text, tp);
    }
  }

  if (m_scissor)
    renderer->setScissorRect({});

  for (size_t i = 0; i < m_audioCues.size(); ++i) {
    if (m_audioCues[i].endTimecode > 0 && m_audioCues[i].endTimecode <= currentTimecode()) {
      if (!m_activeAudio[i])
        continue;
      m_activeAudio[i]->stop();
    } else if (m_audioCues[i].timecode <= currentTimecode()) {
      if (m_activeAudio[i])
        continue;
      AudioInstancePtr audioInstance = make_shared<AudioInstance>(*Root::singleton().assets()->audio(m_audioCues[i].resource));
      audioInstance->setLoops(m_audioCues[i].loops);
      audioInstance->setMixerGroup(MixerGroup::Cinematic);
      mixer->play(audioInstance);
      m_activeAudio[i] = audioInstance;
    }
  }
}

void Cinematic::drawDrawable(Drawable const& drawable, float drawableScale, Vec2F const& drawableTranslation) {
  auto& guiContext = GuiContext::singleton();
  auto& renderer = guiContext.renderer();
  auto& textureGroup = guiContext.assetTextureGroup();

  auto& primitives = renderer->immediatePrimitives();

  if (drawable.isImage()) {
    auto const& imagePart = drawable.imagePart();
    auto texture = textureGroup->loadTexture(imagePart.image);
    auto size = Vec2F(texture->size());

    RectF imageRect(Vec2F(), size);

    Vec2F screenTranslation = drawable.position * drawableScale + drawableTranslation;

    Vec2F lowerLeft =
        imagePart.transformation.transformVec2(Vec2F(imageRect.xMin(), imageRect.yMin())) * drawableScale
        + screenTranslation;
    Vec2F lowerRight =
        imagePart.transformation.transformVec2(Vec2F(imageRect.xMax(), imageRect.yMin())) * drawableScale
        + screenTranslation;
    Vec2F upperRight =
        imagePart.transformation.transformVec2(Vec2F(imageRect.xMax(), imageRect.yMax())) * drawableScale
        + screenTranslation;
    Vec2F upperLeft =
        imagePart.transformation.transformVec2(Vec2F(imageRect.xMin(), imageRect.yMax())) * drawableScale
        + screenTranslation;

    Vec4B drawableColor = drawable.color.toRgba();

    primitives.emplace_back(std::in_place_type_t<RenderQuad>(), std::move(texture),
        lowerLeft,  Vec2F{0, 0},
        lowerRight, Vec2F{size[0], 0},
        upperRight, Vec2F{size[0], size[1]},
        upperLeft,  Vec2F{0, size[1]},
        drawableColor, 0.0f);
  } else {
    starAssert(drawable.part.empty());
  }
}

void Cinematic::updateCamera(float timecode) {
  float startZoom = 1.0f;
  float startZoomTimecode = -1;
  float endZoom = startZoom;
  float endZoomTimecode = startZoomTimecode;

  Vec2F startPan = {0, 0};
  float startPanTimecode = -1;
  Vec2F endPan = startPan;
  float endPanTimecode = startPanTimecode;

  for (auto keyframe : m_cameraKeyFrames) {
    if (keyframe.timecode <= timecode) {
      startZoom = keyframe.zoom;
      startZoomTimecode = keyframe.timecode;
    }
    if (endZoomTimecode < timecode) {
      endZoom = keyframe.zoom;
      endZoomTimecode = keyframe.timecode;
    }

    if (keyframe.timecode <= timecode) {
      startPan = keyframe.pan;
      startPanTimecode = keyframe.timecode;
    }
    if (endPanTimecode < timecode) {
      endPan = keyframe.pan;
      endPanTimecode = keyframe.timecode;
    }
  }

  if (startZoom == endZoom)
    m_cameraZoom = startZoom;
  else if (timecode <= startZoomTimecode)
    m_cameraZoom = startZoom;
  else if (timecode >= endZoomTimecode)
    m_cameraZoom = endZoom;
  else
    m_cameraZoom = lerp((timecode - startZoomTimecode) / (endZoomTimecode - startZoomTimecode), startZoom, endZoom);

  if (startPan == endPan)
    m_cameraPan = startPan;
  else if (timecode <= startPanTimecode)
    m_cameraPan = startPan;
  else if (timecode >= endPanTimecode)
    m_cameraPan = endPan;
  else
    m_cameraPan = lerp((timecode - startPanTimecode) / (endPanTimecode - startPanTimecode), startPan, endPan);
}

float Cinematic::currentTimecode() const {
  return std::min((float)m_timer.time() - m_backgroundFadeTime, m_completionTime - 2 * m_backgroundFadeTime);
}

Cinematic::PanelValues Cinematic::determinePanelValues(PanelPtr panel, float timecode) {
  if (panel->endTime != 0) {
    if (timecode > panel->endTime) {
      Cinematic::PanelValues result{};
      result.alpha = 0;
      return result;
    }
  }

  if (panel->startTime != 0) {
    if (timecode < panel->startTime) {
      Cinematic::PanelValues result{};
      result.alpha = 0;
      return result;
    } else {
      timecode -= panel->startTime;
    }
  }

  if (panel->loopTime != 0) {
    timecode = fmod(timecode, panel->loopTime);
  }

  float startZoom = 0;
  float startZoomTimecode = -1;
  float endZoom = startZoom;
  float endZoomTimecode = startZoomTimecode;

  float startAlpha = 0;
  float startAlphaTimecode = -1;
  float endAlpha = startAlpha;
  float endAlphaTimecode = startAlphaTimecode;

  Vec2F startPosition = {};
  float startPositionTimecode = -1;
  Vec2F endPosition = startPosition;
  float endPositionTimecode = startPositionTimecode;

  float startFrame = 0;
  float startFrameTimecode = -1;
  float endFrame = startFrame;
  float endFrameTimecode = startFrameTimecode;

  float startTextPercentage = 1;
  float startTextPercentageTimecode = -1;
  float endTextPercentage = 1;
  float endTextPercentageTimecode = -1;

  bool completable = false;

  for (auto keyframe : panel->keyFrames) {
    if (keyframe.command.contains("complete")) {
      if (keyframe.command.getBool("complete"))
        if (keyframe.timecode <= timecode)
          completable = true;
    }

    if (keyframe.command.contains("zoom")) {
      float zoom = keyframe.command.getFloat("zoom");
      if (keyframe.timecode <= timecode) {
        startZoom = zoom;
        startZoomTimecode = keyframe.timecode;
      }
      if (endZoomTimecode < timecode) {
        endZoom = zoom;
        endZoomTimecode = keyframe.timecode;
      }
    }

    if (keyframe.command.contains("alpha")) {
      float alpha = keyframe.command.getFloat("alpha");
      if (keyframe.timecode <= timecode) {
        startAlpha = alpha;
        startAlphaTimecode = keyframe.timecode;
      }
      if (endAlphaTimecode < timecode) {
        endAlpha = alpha;
        endAlphaTimecode = keyframe.timecode;
      }
    }

    if (keyframe.command.contains("position")) {
      Vec2F position = jsonToVec2F(keyframe.command.get("position"));
      if (keyframe.timecode <= timecode) {
        startPosition = position;
        startPositionTimecode = keyframe.timecode;
      }
      if (endPositionTimecode < timecode) {
        endPosition = position;
        endPositionTimecode = keyframe.timecode;
      }
    }

    if (keyframe.command.contains("frame")) {
      float frame = keyframe.command.getFloat("frame");
      if (keyframe.timecode <= timecode) {
        startFrame = frame;
        startFrameTimecode = keyframe.timecode;
      }
      if (endFrameTimecode < timecode) {
        endFrame = frame;
        endFrameTimecode = keyframe.timecode;
      }
    }

    if (keyframe.command.contains("textPercentage")) {
      float textPercentage = keyframe.command.getFloat("textPercentage");
      if (keyframe.timecode <= timecode) {
        startTextPercentage = textPercentage;
        startTextPercentageTimecode = keyframe.timecode;
      }
      if (endTextPercentageTimecode < timecode) {
        endTextPercentage = textPercentage;
        endTextPercentageTimecode = keyframe.timecode;
      }
    }
  }

  Cinematic::PanelValues result;

  result.completable = completable;

  if (startZoom == endZoom)
    result.zoom = startZoom;
  else if (timecode <= startZoomTimecode)
    result.zoom = startZoom;
  else if (timecode >= endZoomTimecode)
    result.zoom = endZoom;
  else
    result.zoom = lerp((timecode - startZoomTimecode) / (endZoomTimecode - startZoomTimecode), startZoom, endZoom);

  if (startAlpha == endAlpha)
    result.alpha = startAlpha;
  else if (timecode <= startAlphaTimecode)
    result.alpha = startAlpha;
  else if (timecode >= endAlphaTimecode)
    result.alpha = endAlpha;
  else
    result.alpha = lerp((timecode - startAlphaTimecode) / (endAlphaTimecode - startAlphaTimecode), startAlpha, endAlpha);

  if (startPosition == endPosition)
    result.position = startPosition;
  else if (timecode <= startPositionTimecode)
    result.position = startPosition;
  else if (timecode >= endPositionTimecode)
    result.position = endPosition;
  else
    result.position = lerp((timecode - startPositionTimecode) / (endPositionTimecode - startPositionTimecode), startPosition, endPosition);

  if (startFrame == endFrame)
    result.frame = startFrame;
  else if (timecode <= startFrameTimecode)
    result.frame = startFrame;
  else if (timecode >= endFrameTimecode)
    result.frame = endFrame;
  else
    result.frame = lerp((timecode - startFrameTimecode) / (endFrameTimecode - startFrameTimecode), startFrame, endFrame);

  if (startTextPercentage == endTextPercentage)
    result.textPercentage = startTextPercentage;
  else if (timecode <= startTextPercentageTimecode)
    result.textPercentage = startTextPercentage;
  else if (timecode >= endTextPercentageTimecode)
    result.textPercentage = endTextPercentage;
  else
    result.textPercentage =
        lerp((timecode - startTextPercentageTimecode) / (endTextPercentageTimecode - startTextPercentageTimecode),
            startTextPercentage,
            endTextPercentage);

  return result;
}

void Cinematic::setTime(float timecode) {
  m_timer.setTime(timecode + m_backgroundFadeTime);
}

void Cinematic::stop() {
  m_timeSkips.clear();
  m_cameraKeyFrames.clear();
  m_panels.clear();
  m_completionTime = 0;
  m_timer.stop();
  m_timer.reset();
  for (size_t i = 0; i < m_audioCues.size(); ++i) {
    if (m_activeAudio[i])
      m_activeAudio[i]->stop();
  }
  m_audioCues.clear();
  m_activeAudio.clear();
  m_completable = false;
  m_suppressInput = false;
}

bool Cinematic::handleInputEvent(InputEvent const& event) {
  if (completed())
    return false;
  if (event.is<MouseButtonUpEvent>() || event.is<KeyUpEvent>())
    return false;
  if (event.is<KeyDownEvent>()) {
    if (m_currentTimeSkip) {
      setTime(m_currentTimeSkip.take().skipToTime);
      return true;
    } else if (m_skippable && GuiContext::singleton().actions(event).contains(InterfaceAction::CinematicSkip)) {
      stop();
      return true;
    }
  }
  return m_suppressInput;
}

bool Cinematic::suppressInput() const {
  return m_suppressInput && !completed();
}

bool Cinematic::muteSfx() const {
  return m_muteSfx && !completed();
}

bool Cinematic::muteMusic() const {
  return m_muteMusic && !completed();
}

}
