#include "StarChatBubbleManager.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarWorldClient.hpp"
#include "StarChattyEntity.hpp"
#include "StarAssets.hpp"
#include "StarAssetTextureGroup.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarGuiContext.hpp"

namespace Star {

ChatBubbleManager::ChatBubbleManager()
  : m_textTemplate(Vec2F()), m_portraitTextTemplate(Vec2F()) {
  auto assets = Root::singleton().assets();

  m_guiContext = GuiContext::singletonPtr();

  auto jsonData = assets->json("/interface/windowconfig/chatbubbles.config");

  m_color = jsonToColor(jsonData.get("textColor"));

  m_fontSize = jsonData.getInt("fontSize");
  m_textPadding = jsonToVec2F(jsonData.get("textPadding"));

  m_zoom = jsonData.getInt("textZoom");
  m_bubbleOffset = jsonToVec2F(jsonData.get("bubbleOffset"));
  m_maxAge = jsonData.getFloat("maxAge");
  m_portraitMaxAge = jsonData.getFloat("portraitMaxAge");

  unsigned textWrapWidth = jsonData.getUInt("textWrapWidth");
  m_textTemplate = TextPositioning{Vec2F(), HorizontalAnchor::HMidAnchor, VerticalAnchor::TopAnchor, textWrapWidth * m_zoom};

  m_interBubbleMargin = jsonData.getFloat("interBubbleMargin");

  m_maxMessagePerEntity = jsonData.getInt("maxMessagePerEntity");

  m_bubbles.setTweenFactor(jsonData.getFloat("tweenFactor"));
  m_bubbles.setMovementThreshold(jsonData.getFloat("movementThreshold"));

  m_portraitBackgroundImage = jsonData.getString("portraitBackgroundImage");
  m_portraitMoreImage = jsonData.getString("portraitMoreImage");
  m_portraitMorePosition = jsonToVec2I(jsonData.get("portraitMorePosition"));
  m_portraitBackgroundSize = jsonToVec2I(jsonData.get("portraitBackgroundSize"));
  m_portraitPosition = jsonToVec2I(jsonData.get("portraitPosition"));
  m_portraitSize = jsonToVec2I(jsonData.get("portraitSize"));
  m_portraitTextPosition = jsonToVec2I(jsonData.get("portraitTextPosition"));
  m_portraitTextWidth = jsonData.getUInt("portraitTextWidth");
  m_portraitChatterFramerate = jsonData.getFloat("portraitChatterFramerate");
  m_portraitChatterDuration = jsonData.getFloat("portraitChatterDuration");

  m_portraitTextTemplate = TextPositioning{Vec2F(m_portraitTextPosition), HorizontalAnchor::LeftAnchor, VerticalAnchor::TopAnchor, m_portraitTextWidth * m_zoom};

  // This is a factor(0.0 - 1.0) based on the window size.
  // 0.0 is directly over the player, 1.0 is the edge of the window
  m_furthestVisibleTextDistance = jsonData.getFloat("furthestTextDistance");

  String textFadeFunctionName = jsonData.getString("textFadeFunction");
  m_textFadeFunction = Root::singleton().functionDatabase()->function(textFadeFunctionName);

  String bubbleFadeFunctionName = jsonData.getString("bubbleFadeFunction");
  m_bubbleFadeFunction = Root::singleton().functionDatabase()->function(bubbleFadeFunctionName);
}

void ChatBubbleManager::setCamera(WorldCamera const& camera) {
  float oldPixelRatio = m_camera.pixelRatio();
  m_camera = camera;
  if (m_camera.pixelRatio() != oldPixelRatio) {
    List<ChatAction> actions;
    m_bubbles.forEach([&actions](BubbleState<Bubble> const& state, Bubble& bubble) {
        actions.append(SayChatAction{bubble.entity, bubble.text, state.idealDestination, bubble.config});
      });
    m_bubbles.clear();
    for (auto& portraitBubble : m_portraitBubbles)
      actions.append(PortraitChatAction{
          portraitBubble.entity,
          portraitBubble.portrait,
          portraitBubble.text,
          portraitBubble.position,
          portraitBubble.config
        });
    m_portraitBubbles.clear();
    addChatActions(actions, true);
  }
}

void ChatBubbleManager::update(float dt, WorldClientPtr world) {
  m_bubbles.forEach([this, dt, &world](BubbleState<Bubble>& bubbleState, Bubble& bubble) {
    bubble.age += dt;
    if (auto entity = world->get<ChattyEntity>(bubble.entity)) {
      bubble.onscreen = m_camera.worldGeometry().rectIntersectsRect(
          m_camera.worldScreenRect(), entity->metaBoundBox().translated(entity->position()));
      bubbleState.idealDestination = m_camera.worldToScreen(entity->mouthPosition() + m_bubbleOffset);
    }
  });

  for (auto& portraitBubble : m_portraitBubbles) {
    portraitBubble.age += dt;
    if (auto entity = world->entity(portraitBubble.entity)) {
      portraitBubble.onscreen = m_camera.worldGeometry().rectIntersectsRect(m_camera.worldScreenRect(), entity->metaBoundBox().translated(entity->position()));
      if (auto chatter = as<ChattyEntity>(entity))
        portraitBubble.position = chatter->mouthPosition();
      else
        portraitBubble.position = entity->position();
    }
  }

  Map<EntityId, int> count;
  filter(m_portraitBubbles, [&](PortraitBubble const& portraitBubble) -> bool {
      count[portraitBubble.entity] += m_maxMessagePerEntity;
      if (count[portraitBubble.entity] > m_maxMessagePerEntity)
        return false;
      if (world->get<ChattyEntity>(portraitBubble.entity))
        return portraitBubble.age < m_portraitMaxAge;
      return false;
    });

  m_bubbles.filter([&](BubbleState<Bubble> const&, Bubble const& bubble) -> bool {
      if (++count[bubble.entity] > m_maxMessagePerEntity)
        return false;
      if (world->get<ChattyEntity>(bubble.entity))
        return bubble.age < m_maxAge;
      return false;
    });

  m_bubbles.update(dt);
}

uint8_t ChatBubbleManager::calcDistanceFadeAlpha(Vec2F bubbleScreenPosition, StoredFunctionPtr fadeFunction) const {
  // first calculate bubble position as a factor, distance from center to edge
  // of screen (0.0-1.0)
  float halfScreenwidth = m_camera.screenSize()[0] * 0.5f;
  float distanceFactor = (fabsf(bubbleScreenPosition[0] - halfScreenwidth)) / halfScreenwidth;

  // that distance factor is divided by the max allowable distance
  // to re-space the distance as a 0 - 1 over the max allowable distance
  distanceFactor = clamp(distanceFactor / m_furthestVisibleTextDistance, 0.0f, 1.0f);

  int alpha = fadeFunction->evaluate(distanceFactor);
  return clamp(alpha, 0, 255);
}

void ChatBubbleManager::render() {
  if (m_bubbles.empty() && m_portraitBubbles.empty())
    return;
  if (!Root::singleton().configuration()->get("speechBubbles").toBool())
    return;

  m_bubbles.forEach([this](BubbleState<Bubble> const& state, Bubble& bubble) {
      if (bubble.onscreen) {
        int alpha = calcDistanceFadeAlpha(state.currentPosition, m_bubbleFadeFunction);
        if (alpha) {
          for (auto const& bubbleImage : bubble.backgroundImages)
            drawBubbleImage(state.currentPosition, bubbleImage, m_zoom, alpha);
          for (auto const& bubbleText : bubble.bubbleText)
            drawBubbleText(state.currentPosition, bubbleText, m_zoom, alpha, false);
        }
      }
    });

  for (auto portraitBubble : m_portraitBubbles) {
    if (portraitBubble.onscreen) {
      Vec2F screenPos = m_camera.worldToScreen(portraitBubble.position + m_bubbleOffset);
      int frame = 0;
      if (portraitBubble.age <= m_portraitChatterDuration)
        frame = int((portraitBubble.age / m_portraitChatterFramerate) * 2) % 2;
      // 255 here because portrait bubbles are always full opacity
      for (auto const& bubbleImage : portraitBubble.backgroundImages)
        drawBubbleImage(screenPos, make_tuple(get<0>(bubbleImage).replace("<frame>", toString(frame)), get<1>(bubbleImage)), m_zoom, 255);
      // 255 here because portrait bubbles are always full opacity
      for (auto const& bubbleText : portraitBubble.bubbleText)
        drawBubbleText(screenPos, bubbleText, m_zoom, 255, true);
    }
  }
}

void ChatBubbleManager::addChatActions(List<ChatAction> chatActions, bool silent) {
  auto assets = Root::singleton().assets();
  auto config = assets->json("/interface/windowconfig/chatbubbles.config");

  float partSize = config.getFloat("partSize");

  for (auto action : chatActions) {
    Json config = JsonObject{};
    Vec2F position;

    if (action.is<SayChatAction>()) {
      auto sayAction = action.get<SayChatAction>();
      config = sayAction.config.optObject().value(JsonObject{});
      position = sayAction.position;

      // TODO: Get rid of this stupid fucking bullshit, this is the ugliest
      // fragilest pointlessest horseshit code in the codebase.  It wouldn't
      // bother me so bad if it weren't so fucking easy to do right.

      // yea I agree
      m_guiContext->setFontSize(m_fontSize, m_zoom);
      m_guiContext->setFontProcessingDirectives("");
      m_guiContext->setDefaultFont();
      auto result = m_guiContext->determineTextSize(sayAction.text, m_textTemplate);
      float textWidth = result.width() / m_zoom + m_textPadding[0];
      float textHeight = result.height() / m_zoom + m_textPadding[1];

      Vec2I innerTiles = Vec2I::ceil(Vec2F((textWidth + 4) / partSize, (textHeight + 3) / partSize));
      if (innerTiles[0] % 2 == 0)
        innerTiles[0] += 1;
      if (innerTiles[0] < 3)
        innerTiles[0] = 3;
      int middleIdx = (innerTiles[0] - 1) / 2;

      List<BubbleImage> backgroundImages;
      if (config.getBool("drawBorder", true)) {
        for (int y = 0; y < innerTiles[1]; y++) {
          for (int x = 0; x < innerTiles[0]; x++) {
            auto partPosition = [partSize](int x, int y) {
              return Vec2F(x * partSize, y * partSize);
            };
            if (y == 0) {
              if (x == 0) {
                backgroundImages.append(make_tuple("/interface/chatbubbles/cornerBottomLeft.png", partPosition(x, y)));
              } else if (x == innerTiles[0] - 1) {
                backgroundImages.append(make_tuple("/interface/chatbubbles/cornerBottomRight.png", partPosition(x, y)));
              } else {
                if (middleIdx == x)
                  backgroundImages.append(make_tuple("/interface/chatbubbles/point.png", partPosition(x, y - 1)));
                else
                  backgroundImages.append(make_tuple("/interface/chatbubbles/sideDown.png", partPosition(x, y)));
              }
            } else if (y == innerTiles[1] - 1) {
              if (x == 0)
                backgroundImages.append(make_tuple("/interface/chatbubbles/cornerTopLeft.png", partPosition(x, y)));
              else if (x == innerTiles[0] - 1)
                backgroundImages.append(make_tuple("/interface/chatbubbles/cornerTopRight.png", partPosition(x, y)));
              else
                backgroundImages.append(make_tuple("/interface/chatbubbles/sideUp.png", partPosition(x, y)));
            } else {
              if (x == 0)
                backgroundImages.append(make_tuple("/interface/chatbubbles/sideLeft.png", partPosition(x, y)));
              else if (x == innerTiles[0] - 1)
                backgroundImages.append(make_tuple("/interface/chatbubbles/sideRight.png", partPosition(x, y)));
              else
                backgroundImages.append(make_tuple("/interface/chatbubbles/center.png", partPosition(x, y)));
            }
          }
        }
      }

      float textMultiLineShift = textHeight;
      float horizontalCenter = partSize * innerTiles[0] * 0.5f;
      float verticalShift = (partSize * innerTiles[1] - textMultiLineShift) * 0.5f + textMultiLineShift;
      Vec2F position = Vec2F(horizontalCenter, verticalShift);
      List<BubbleText> bubbleTexts;
      auto fontSize = config.getUInt("fontSize", m_fontSize);
      auto color = config.opt("color").apply(jsonToColor).value(m_color);
      bubbleTexts.append(make_tuple(sayAction.text, fontSize, color.toRgba(), true, position));

      for (auto& backgroundImage : backgroundImages)
        get<1>(backgroundImage) += Vec2F(-horizontalCenter, partSize);
      for (auto& bubbleText : bubbleTexts)
        get<4>(bubbleText) += Vec2F(-horizontalCenter, partSize);

      auto pos = m_camera.worldToScreen(sayAction.position + m_bubbleOffset);
      RectF boundBox = fold(backgroundImages, RectF::null(), [pos, this](RectF const& boundBox, BubbleImage const& bubbleImage) {
          return boundBox.combined(bubbleImageRect(pos, bubbleImage, m_zoom));
        });
      Bubble bubble = {sayAction.entity, sayAction.text, sayAction.config, 0, std::move(backgroundImages), std::move(bubbleTexts), false};
      List<BubbleState<Bubble>> oldBubbles = m_bubbles.filtered([&sayAction](BubbleState<Bubble> const&, Bubble const& bubble) {
          return bubble.entity == sayAction.entity;
        });
      m_bubbles.filter([&sayAction](BubbleState<Bubble> const&, Bubble const& bubble) { return bubble.entity != sayAction.entity; });
      m_bubbles.addBubble(pos, boundBox, std::move(bubble), m_interBubbleMargin * m_zoom);
      oldBubbles.sort([](BubbleState<Bubble> const& a, BubbleState<Bubble> const& b) { return a.contents.age < b.contents.age; });
      for (auto bubble : oldBubbles.slice(0, m_maxMessagePerEntity - 1))
        m_bubbles.addBubble(bubble.idealDestination, bubble.boundBox, bubble.contents, 0);

    } else if (action.is<PortraitChatAction>()) {
      auto portraitAction = action.get<PortraitChatAction>();
      config = portraitAction.config.optObject().value(JsonObject{});
      position = portraitAction.position;

      List<BubbleImage> backgroundImages;
      backgroundImages.append(make_tuple(m_portraitBackgroundImage, Vec2F()));
      if (config.getBool("drawMoreIndicator", false))
        backgroundImages.append(make_tuple(m_portraitMoreImage, Vec2F(m_portraitMorePosition)));
      backgroundImages.append(make_tuple(portraitAction.portrait, Vec2F(m_portraitPosition)));
      List<BubbleText> bubbleTexts;
      bubbleTexts.append(make_tuple(portraitAction.text, m_fontSize, m_color.toRgba(), false, Vec2F(m_portraitTextPosition)));

      for (auto& backgroundImage : backgroundImages)
        get<1>(backgroundImage) += Vec2F(-m_portraitBackgroundSize[0] / 2, 0);
      for (auto& bubbleText : bubbleTexts)
        get<4>(bubbleText) += Vec2F(-m_portraitBackgroundSize[0] / 2, 0);

      m_portraitBubbles.prepend({
          portraitAction.entity,
          portraitAction.portrait,
          portraitAction.text,
          portraitAction.position,
          portraitAction.config,
          0,
          std::move(backgroundImages),
          std::move(bubbleTexts),
          false
        });
    }

    if (!silent) {
      if (auto sound = config.optString("sound")) {
        auto assets = Root::singleton().assets();
        AudioInstancePtr audioInstance = make_shared<AudioInstance>(*assets->audio(*sound));
        audioInstance->setPosition(position);
        m_guiContext->playAudio(audioInstance);
      }
    }
  }
}

RectF ChatBubbleManager::bubbleImageRect(Vec2F screenPos, BubbleImage const& bubbleImage, int pixelRatio) {
  auto imgMetadata = Root::singleton().imageMetadataDatabase();
  auto image = get<0>(bubbleImage);
  return RectF::withSize(screenPos + get<1>(bubbleImage) * pixelRatio, Vec2F(imgMetadata->imageSize(image)) * pixelRatio);
}

void ChatBubbleManager::drawBubbleImage(Vec2F screenPos, BubbleImage const& bubbleImage, int pixelRatio, int alpha) {
  auto image = get<0>(bubbleImage);
  auto offset = get<1>(bubbleImage) * pixelRatio;
  m_guiContext->drawQuad(image, screenPos + offset, pixelRatio, {255, 255, 255, alpha});
}

void ChatBubbleManager::drawBubbleText(Vec2F screenPos, BubbleText const& bubbleText, int pixelRatio, int alpha, bool isPortrait) {
  Vec4B const& baseColor = get<2>(bubbleText);

  // use the alpha as a blend value for the text colour as pulled from data.
  Vec4B const& displayColor = Vec4B(baseColor[0], baseColor[1], baseColor[2], (baseColor[3] * alpha) / 255);

  m_guiContext->setDefaultFont();
  m_guiContext->setFontProcessingDirectives("");
  m_guiContext->setFontColor(displayColor);
  m_guiContext->setFontSize(get<1>(bubbleText), m_zoom);

  auto offset = get<4>(bubbleText) * pixelRatio;
  TextPositioning tp = isPortrait ? m_portraitTextTemplate : m_textTemplate;
  tp.pos = screenPos + offset;

  m_guiContext->renderText(get<0>(bubbleText), tp);
}

}
