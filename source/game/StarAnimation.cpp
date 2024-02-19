#include "StarAnimation.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGameTypes.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

Animation::Animation(Json config, String const& directory) {
  m_directory = directory;
  if (m_directory.empty()) {
    if (config.isType(Json::Type::String))
      m_directory = AssetPath::directory(config.toString());
    else
      m_directory = "/";
  }
  if (config.isNull())
    config = JsonObject();

  config = Root::singleton().assets()->fetchJson(config);

  m_mode = AnimationModeNames.getLeft(config.getString("mode", "endAndDisappear"));
  m_base = config.getString("frames", "");
  // If the base image has no index tag, then assume that the frame is appended
  // to the end.
  m_appendFrame = !m_base.contains("<index>");
  m_frameNumber = config.getInt("frameNumber", 1);
  m_animationCycle = config.getDouble("animationCycle", 1.0);
  m_animationTime = m_animationCycle * config.getDouble("loops", 1.0);
  m_angle = config.getFloat("angle", 0.0f);
  m_offset = jsonToVec2F(config.get("offset", JsonArray{0.0f, 0.0f}));
  m_centered = config.getBool("centered", true);
  m_processing = config.getString("processing", "");
  m_color = config.opt("color").apply(jsonToColor).value(Color::White);
  m_variantOffset = Random::randInt(config.getInt("variants", 1) - 1) * m_frameNumber;

  reset();
}

void Animation::setAngle(float angle) {
  m_angle = angle;
}

void Animation::setProcessing(DirectivesGroup processing) {
  m_processing = std::move(processing);
}

void Animation::setColor(Color color) {
  m_color = color;
}

void Animation::setTag(String tagName, String tagValue) {
  m_tagValues[std::move(tagName)] = std::move(tagValue);
}

void Animation::clearTags() {
  m_tagValues.clear();
}

Drawable Animation::drawable(float pixelSize) const {
  if (m_base.empty() || m_frame < 0)
    return Drawable();

  // Replace <index> with the frame index, if it is not found then add
  // :<index> to the end
  String baseFrame = AssetPath::relativeTo(m_directory, m_base).replaceTags(m_tagValues);
  if (m_appendFrame)
    baseFrame += ":" + toString(m_frame);

  Drawable drawable = Drawable::makeImage(std::move(baseFrame), pixelSize, m_centered, m_offset);
  drawable.imagePart().addDirectivesGroup(m_processing);
  drawable.rotate(m_angle);
  drawable.color = m_color;
  return drawable;
}

void Animation::update(float dt) {
  if (m_completed)
    return;

  float time_within_cycle = fmod(m_animationTimer, m_animationCycle);
  float time_per_frame = m_animationCycle / m_frameNumber;
  float frame_number = time_within_cycle / time_per_frame;
  m_frame = m_variantOffset + clamp<int>(frame_number, 0, m_frameNumber - 1);
  m_animationTimer += dt;

  if (m_mode == Animation::LoopForever) {
    // to prevent floating point jitter and seizing after a very long time
    m_animationTimer = fmod(m_animationTimer, m_animationCycle);
  } else if (m_animationTimer >= m_timeToLive) {
    if (m_mode == Animation::EndAndDisappear)
      m_frame = -1;
    m_completed = true;
  }

  m_tagValues["index"] = toString(m_frame);
}

bool Animation::isComplete() const {
  return m_completed;
}

void Animation::reset() {
  m_frame = 0;
  m_animationTimer = 0;
  m_timeToLive = m_animationTime;
  m_completed = false;
  m_tagValues["index"] = "0";
}

EnumMap<Animation::AnimationMode> Animation::AnimationModeNames = {{Animation::AnimationMode::Stop, "Stop"},
    {Animation::AnimationMode::EndAndDisappear, "EndAndDisappear"},
    {Animation::AnimationMode::LoopForever, "LoopForever"}};
}
