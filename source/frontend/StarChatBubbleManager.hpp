#pragma once

#include "StarChatAction.hpp"
#include "StarTextPainter.hpp"
#include "StarWorldCamera.hpp"
#include "StarChatBubbleSeparation.hpp"
#include "StarStoredFunctions.hpp"

namespace Star {

STAR_CLASS(GuiContext);
STAR_CLASS(AssetTextureGroup);
STAR_CLASS(WorldClient);
STAR_CLASS(ChatBubbleManager);

class ChatBubbleManager {
public:
  ChatBubbleManager();

  void setCamera(WorldCamera const& camera);

  void addChatActions(List<ChatAction> chatActions, bool silent = false);

  void update(float dt, WorldClientPtr world);
  void render();

private:
  typedef tuple<String, Vec2F> BubbleImage;
  typedef tuple<String, unsigned, Vec4B, bool, Vec2F> BubbleText;

  struct Bubble {
    EntityId entity;
    String text;
    Json config;
    float age;
    List<BubbleImage> backgroundImages;
    List<BubbleText> bubbleText;
    bool onscreen;
  };

  struct PortraitBubble {
    EntityId entity;
    String portrait;
    String text;
    Vec2F position;
    Json config;
    float age;
    List<BubbleImage> backgroundImages;
    List<BubbleText> bubbleText;
    bool onscreen;
  };

  // Calculate the alpha for a speech bubble based on distance from player to
  // edge of screen
  uint8_t calcDistanceFadeAlpha(Vec2F bubbleScreenPosition, StoredFunctionPtr fadeFunction) const;

  RectF bubbleImageRect(Vec2F screenPos, BubbleImage const& bubbleImage, int pixelRatio);
  void drawBubbleImage(Vec2F screenPos, BubbleImage const& bubbleImage, int pixelRatio, int alpha);
  void drawBubbleText(Vec2F screenPos, BubbleText const& bubbleText, int pixelRatio, int alpha, bool isPortrait);

  GuiContext* m_guiContext;

  WorldCamera m_camera;

  TextPositioning m_textTemplate;
  TextPositioning m_portraitTextTemplate;
  Color m_color;
  int m_fontSize;
  Vec2F m_textPadding;

  BubbleSeparator<Bubble> m_bubbles;
  int m_zoom;
  Vec2F m_bubbleOffset;
  float m_maxAge;
  float m_portraitMaxAge;
  float m_interBubbleMargin;
  int m_maxMessagePerEntity;

  Deque<PortraitBubble> m_portraitBubbles;
  String m_portraitBackgroundImage;
  String m_portraitMoreImage;
  Vec2I m_portraitMorePosition;
  Vec2I m_portraitBackgroundSize;
  Vec2I m_portraitPosition;
  Vec2I m_portraitSize;
  Vec2I m_portraitTextPosition;
  unsigned m_portraitTextWidth;
  float m_portraitChatterFramerate;
  float m_portraitChatterDuration;

  float m_furthestVisibleTextDistance; // 0.0 is directly over the player, 1.0
  // is the edge of the window
  StoredFunctionPtr m_textFadeFunction;
  StoredFunctionPtr m_bubbleFadeFunction;
};

}
