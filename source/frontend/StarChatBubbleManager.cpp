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
  m_cachedInterfaceScale = m_guiContext->interfaceScale();

  auto jsonData = assets->json("/interface/windowconfig/chatbubbles.config");

  m_textStyle.color = jsonToColor(jsonData.get("textColor")).toRgba();
  m_textStyle.loadJson(jsonData.get("textStyle"));
  m_textPadding = jsonToVec2F(jsonData.get("textPadding"));

  m_partSize = jsonData.getFloat("partSize");
  m_bubbleOffset = jsonToVec2F(jsonData.get("bubbleOffset"));
  m_maxAge = jsonData.getFloat("maxAge");
  m_portraitMaxAge = jsonData.getFloat("portraitMaxAge");

  unsigned textWrapWidth = jsonData.getUInt("textWrapWidth");
  m_textTemplate = TextPositioning{Vec2F(), HorizontalAnchor::HMidAnchor, VerticalAnchor::TopAnchor, textWrapWidth};

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

  m_portraitTextTemplate = TextPositioning{Vec2F(m_portraitTextPosition), HorizontalAnchor::LeftAnchor, VerticalAnchor::TopAnchor, m_portraitTextWidth};

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
  float oldInterfaceScale = m_cachedInterfaceScale;
  m_camera = camera;
  m_cachedInterfaceScale = m_guiContext->interfaceScale();
  if (m_camera.pixelRatio() != oldPixelRatio || m_cachedInterfaceScale != oldInterfaceScale) {
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

  float pixelRatio = m_guiContext->interfaceScale();

  m_bubbles.forEach([this, pixelRatio](BubbleState<Bubble> const& state, Bubble& bubble) {
      if (bubble.onscreen) {
        int alpha = calcDistanceFadeAlpha(state.currentPosition, m_bubbleFadeFunction);
        if (alpha) {
          for (auto const& bubbleImage : bubble.backgroundImages)
            drawBubbleImage(state.currentPosition, bubbleImage, pixelRatio, alpha);
          for (auto const& bubbleText : bubble.bubbleText)
            drawBubbleText(state.currentPosition, bubbleText, pixelRatio, alpha, false);
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
        drawBubbleImage(screenPos, make_tuple(get<0>(bubbleImage).replace("<frame>", toString(frame)), get<1>(bubbleImage)), pixelRatio, 255);
      // 255 here because portrait bubbles are always full opacity
      for (auto const& bubbleText : portraitBubble.bubbleText)
        drawBubbleText(screenPos, bubbleText, pixelRatio, 255, true);
    }
  }
}

void ChatBubbleManager::addChatActions(List<ChatAction> chatActions, bool silent) {
  float pixelRatio = m_guiContext->interfaceScale();

  for (auto action : chatActions) {
    Json config = JsonObject{};
    Vec2F position;

    if (action.is<SayChatAction>()) {
      auto sayAction = action.get<SayChatAction>();
      config = sayAction.config.optObject().value(JsonObject{});
      position = sayAction.position;

      // Get rid of this stupid fucking bullshit, this is the ugliest
      // fragilest pointlessest horseshit code in the codebase.  It wouldn't
      // bother me so bad if it weren't so fucking easy to do right.

      // yea I agree

      // tada~!

      // Measure with the style the text is drawn with, so bubbles using a
      // custom font size or style are still sized to fit.
      TextStyle textStyle = bubbleTextStyle(config);
      Vec2F textSize = measureText(sayAction.text, textStyle, pixelRatio) + m_textPadding;

      Vec2I innerTiles = Vec2I::ceil((textSize + Vec2F::filled(4)) / m_partSize);
      if (innerTiles[0] % 2 == 0)
        innerTiles[0] += 1;
      if (innerTiles[0] < 3)
        innerTiles[0] = 3;

      List<BubbleImage> backgroundImages;
      if (config.getBool("drawBorder", true))
        backgroundImages = bubbleBackground(innerTiles, m_partSize);

      float horizontalCenter = m_partSize * innerTiles[0] * 0.5f;
      float verticalShift = (m_partSize * innerTiles[1] - textSize[1]) * 0.5f + textSize[1] - 1.0f;
      // Anchor everything on the bottom middle of the bubble, just above the speaker
      Vec2F origin = Vec2F(-horizontalCenter, m_partSize);
      for (auto& backgroundImage : backgroundImages)
        get<1>(backgroundImage) += origin;
      List<BubbleText> bubbleTexts;
      bubbleTexts.append(make_tuple(sayAction.text, textStyle, true, Vec2F(horizontalCenter, verticalShift) + origin));

      auto pos = m_camera.worldToScreen(sayAction.position + m_bubbleOffset);
      RectF boundBox = fold(backgroundImages, RectF::null(), [pos, pixelRatio, this](RectF const& boundBox, BubbleImage const& bubbleImage) {
          return boundBox.combined(bubbleImageRect(pos, bubbleImage, pixelRatio));
        });
      Bubble bubble = {sayAction.entity, sayAction.text, sayAction.config, 0, std::move(backgroundImages), std::move(bubbleTexts), false};
      List<BubbleState<Bubble>> oldBubbles = m_bubbles.filtered([&sayAction](BubbleState<Bubble> const&, Bubble const& bubble) {
          return bubble.entity == sayAction.entity;
        });
      m_bubbles.filter([&sayAction](BubbleState<Bubble> const&, Bubble const& bubble) { return bubble.entity != sayAction.entity; });
      m_bubbles.addBubble(pos, boundBox, std::move(bubble), m_interBubbleMargin * pixelRatio);
      oldBubbles.sort([](BubbleState<Bubble> const& a, BubbleState<Bubble> const& b) { return a.contents.age < b.contents.age; });
      for (auto& bubble : oldBubbles.slice(0, m_maxMessagePerEntity - 1))
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
      bubbleTexts.append(make_tuple(portraitAction.text, m_textStyle, false, Vec2F(m_portraitTextPosition)));

      for (auto& backgroundImage : backgroundImages)
        get<1>(backgroundImage) += Vec2F(-m_portraitBackgroundSize[0] / 2, 0);
      for (auto& bubbleText : bubbleTexts)
        get<3>(bubbleText) += Vec2F(-m_portraitBackgroundSize[0] / 2, 0);

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
        audioInstance->setVolume(config.getFloat("volume", 1.0f));
        audioInstance->setPitchMultiplier(config.getFloat("pitch", 1.0f));
        m_guiContext->playAudio(audioInstance);
      }
    }
  }
}

RectF ChatBubbleManager::bubbleImageRect(Vec2F screenPos, BubbleImage const& bubbleImage, float pixelRatio) {
  auto imgMetadata = Root::singleton().imageMetadataDatabase();
  auto& image = get<0>(bubbleImage);
  return RectF::withSize(screenPos + get<1>(bubbleImage) * pixelRatio, Vec2F(imgMetadata->imageSize(image)) * pixelRatio);
}

void ChatBubbleManager::drawBubbleImage(Vec2F screenPos, BubbleImage const& bubbleImage, float pixelRatio, int alpha) {
  auto& image = get<0>(bubbleImage);
  auto offset = get<1>(bubbleImage) * pixelRatio;
  m_guiContext->drawQuad(image, screenPos + offset, pixelRatio, {255, 255, 255, alpha});
}

void ChatBubbleManager::drawBubbleText(Vec2F screenPos, BubbleText const& bubbleText, float pixelRatio, int alpha, bool isPortrait) {
  TextStyle style = get<1>(bubbleText);
  style.color[3] *= ((float)alpha / 255.f);
  m_guiContext->setTextStyle(style, pixelRatio);
  auto offset = get<3>(bubbleText) * pixelRatio;
  TextPositioning tp = isPortrait ? m_portraitTextTemplate : m_textTemplate;
  tp.pos = screenPos + offset;
  m_guiContext->renderText(get<0>(bubbleText), scaledPositioning(tp, pixelRatio));
}

TextPositioning ChatBubbleManager::scaledPositioning(TextPositioning positioning, float pixelRatio) const {
  positioning.wrapWidth = positioning.wrapWidth.apply([=](unsigned w) { return (unsigned)(w * pixelRatio); });
  return positioning;
}

Vec2F ChatBubbleManager::measureText(String const& text, TextStyle const& style, float pixelRatio) const {
  m_guiContext->setTextStyle(style, pixelRatio);
  auto size = m_guiContext->determineTextSize(text, scaledPositioning(m_textTemplate, pixelRatio));
  return Vec2F(size.width(), size.height()) / pixelRatio;
}

TextStyle ChatBubbleManager::bubbleTextStyle(Json const& config) const {
  TextStyle style = m_textStyle;
  style.fontSize = config.getUInt("fontSize", style.fontSize);
  if (auto jColor = config.opt("color"))
    style.color = jsonToColor(*jColor).toRgba();
  style.loadJson(config.get("style", Json()));
  return style;
}

// Every bubble is made of tiles, with a point on the bottom edge in the middle:
// the bottom row (y = 0) has the point, the top row is the roof and the
// rows in between are walls and filling.
List<ChatBubbleManager::BubbleImage> ChatBubbleManager::bubbleBackground(Vec2I innerTiles, float partSize) {
  List<BubbleImage> images;
  int middleIdx = (innerTiles[0] - 1) / 2;
  auto partPosition = [partSize](int x, int y) { return Vec2F(x * partSize, y * partSize); };
  auto add = [&](char const* part, int x, int y, int yOffset = 0) {
    images.append(make_tuple(strf("/interface/chatbubbles/{}.png", part), partPosition(x, y + yOffset)));
  };

  for (int y = 0; y < innerTiles[1]; y++) {
    for (int x = 0; x < innerTiles[0]; x++) {
      bool left = x == 0;
      bool right = x == innerTiles[0] - 1;
      if (y == 0) {
        if (left)
          add("cornerBottomLeft", x, y);
        else if (right)
          add("cornerBottomRight", x, y);
        else if (x == middleIdx)
          add("point", x, y, -1);
        else
          add("sideDown", x, y);
      } else if (y == innerTiles[1] - 1) {
        if (left)
          add("cornerTopLeft", x, y);
        else if (right)
          add("cornerTopRight", x, y);
        else
          add("sideUp", x, y);
      } else {
        if (left)
          add("sideLeft", x, y);
        else if (right)
          add("sideRight", x, y);
        else
          add("center", x, y);
      }
    }
  }
  return images;
}

}
