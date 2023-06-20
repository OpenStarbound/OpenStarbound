#ifndef STAR_CINEMATIC_HPP
#define STAR_CINEMATIC_HPP

#include "StarTime.hpp"
#include "StarRenderer.hpp"
#include "StarDrawable.hpp"
#include "StarWorldCamera.hpp"
#include "StarInputEvent.hpp"
#include "StarTextPainter.hpp"
#include "StarMixer.hpp"

namespace Star {

STAR_CLASS(Cinematic);
STAR_CLASS(Player);

class Cinematic {
public:
  Cinematic();

  void load(Json const& definition);

  void setPlayer(PlayerPtr player);

  void update();
  void render();

  bool completed() const;
  bool completable() const;

  // this won't synchronize audio, so it should only be used for testing
  void setTime(float timecode);

  void stop();

  bool handleInputEvent(InputEvent const& event);
  bool suppressInput() const;

  bool muteSfx() const;
  bool muteMusic() const;

private:
  struct TimeSkip {
    float availableTime;
    float skipToTime;
  };

  struct KeyFrame {
    float timecode;
    Json command;
  };

  struct CameraKeyFrame {
    float timecode;
    float zoom;
    Vec2F pan;
  };

  struct Panel {
    bool useCamera;
    String avatar;
    JsonArray drawables;
    int animationFrames;
    String text;
    TextPositioning textPosition;
    Vec4B fontColor;
    unsigned fontSize;
    List<KeyFrame> keyFrames;
    float startTime;
    float endTime;
    float loopTime;
  };
  typedef shared_ptr<Panel> PanelPtr;

  struct PanelValues {
    float zoom;
    float alpha;
    Vec2F position;
    float frame;
    float textPercentage;
    bool completable;
  };

  struct AudioCue {
    AudioCue() : timecode(), endTimecode() {}
    String resource;
    int loops;
    float timecode;
    float endTimecode;
  };

  void drawDrawable(Drawable const& drawable, float drawableScale, Vec2F const& drawableTranslation);

  void updateCamera(float timecode);

  // since the clock time includes the background fade in/out time, this function gives the adjusted
  // timecode to use for events within the cinematic
  float currentTimecode() const;

  PanelValues determinePanelValues(PanelPtr panel, float timecode);

  List<TimeSkip> m_timeSkips;
  Maybe<TimeSkip> m_currentTimeSkip;

  List<CameraKeyFrame> m_cameraKeyFrames;
  List<PanelPtr> m_panels;
  List<AudioCue> m_audioCues;
  std::vector<AudioInstancePtr> m_activeAudio;

  // these include the time for background fades so they may not reflect the completion timecode
  Clock m_timer;
  float m_completionTime;

  Maybe<Vec4B> m_backgroundColor;
  float m_backgroundFadeTime;

  float m_cameraZoom;
  Vec2F m_cameraPan;

  float m_drawableScale;
  Vec2F m_drawableTranslation;
  Vec2F m_windowSize;
  RectI m_scissorRect;

  bool m_scissor;
  bool m_letterbox;

  PlayerPtr m_player;

  Vec2F m_offset;

  bool m_skippable;
  bool m_suppressInput;

  bool m_muteSfx;
  bool m_muteMusic;

  bool m_completable;
};

}

#endif
