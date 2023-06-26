#ifndef STAR_ANIMATION_HPP
#define STAR_ANIMATION_HPP

#include "StarDrawable.hpp"
#include "StarBiMap.hpp"

namespace Star {

STAR_CLASS(Animation);

class Animation {
public:
  // config can be either a path to a config or a literal config.
  Animation(Json config = {}, String const& directory = {});

  void setAngle(float angle);

  void setProcessing(DirectivesGroup processing);

  void setColor(Color color);

  void setTag(String tagName, String tagValue);
  void clearTags();

  Drawable drawable(float pixelSize) const;

  void update(float dt);

  bool isComplete() const;
  void reset();

private:
  enum AnimationMode { Stop, EndAndDisappear, LoopForever };
  static EnumMap<AnimationMode> AnimationModeNames;

  AnimationMode m_mode;
  String m_directory;
  String m_base;
  bool m_appendFrame;
  int m_frameNumber;
  float m_animationCycle;
  float m_animationTime;
  float m_angle;
  Vec2F m_offset;
  bool m_centered;
  DirectivesGroup m_processing;
  Color m_color;
  int m_variantOffset;

  StringMap<String> m_tagValues;
  int m_frame;
  float m_animationTimer;
  float m_timeToLive;
  bool m_completed;
};

}

#endif
