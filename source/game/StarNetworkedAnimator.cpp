#include "StarNetworkedAnimator.hpp"
#include "StarJsonExtra.hpp"
#include "StarIterator.hpp"
#include "StarParticleDatabase.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarLexicalCast.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRandom.hpp"
#include "StarGameTypes.hpp"

namespace Star {

NetworkedAnimator::DynamicTarget::~DynamicTarget() {
  stopAudio();
}

List<AudioInstancePtr> NetworkedAnimator::DynamicTarget::pullNewAudios() {
  pendingAudios.exec([this](AudioInstancePtr const& ptr) {
    Vec2F audioBasePosition = ptr->position().value();
    currentAudioBasePositions[ptr] = audioBasePosition;
    ptr->setPosition(position + audioBasePosition);
  });
  return take(pendingAudios);
}

List<Particle> NetworkedAnimator::DynamicTarget::pullNewParticles() {
  pendingParticles.exec([this](Particle& particle) {
    particle.position += position;
    return particle;
  });
  return take(pendingParticles);
}

void NetworkedAnimator::DynamicTarget::stopAudio() {
  for (auto const& pair : currentAudioBasePositions) {
    if (pair.first->loops() != 0)
      pair.first->stop();
  }
}

void NetworkedAnimator::DynamicTarget::updatePosition(Vec2F const& p) {
  clearFinishedAudio();
  position = p;
  for (auto& audioPair : currentAudioBasePositions)
    audioPair.first->setPosition(audioPair.second + p);
}

void NetworkedAnimator::DynamicTarget::clearFinishedAudio() {
  for (auto& p : statePersistentSounds) {
    if (p.second.audio && p.second.audio->finished())
      p.second.audio.reset();
  }

  for (auto& p : stateImmediateSounds) {
    if (p.second.audio && p.second.audio->finished())
      p.second.audio.reset();
  }

  for (auto& p : independentSounds)
    eraseWhere(p.second, [](AudioInstancePtr const& audio) { return audio->finished(); });

  eraseWhere(currentAudioBasePositions, [](pair<AudioInstancePtr, Vec2F> const& pair) {
      return pair.first->finished();
    });
}

NetworkedAnimator::NetworkedAnimator() {
  m_zoom.set(1.0f);
  m_flipped.set(false);
  m_flippedRelativeCenterLine.set(0.0f);
  m_animationRate.set(1.0f);
  setupNetStates();
}

NetworkedAnimator::NetworkedAnimator(Json config, String relativePath) : NetworkedAnimator() {
  auto& root = Root::singleton();

  if (config.isNull())
    return;

  if (config.type() == Json::Type::String) {
    if (relativePath.empty())
      relativePath = config.toString();
    config = root.assets()->json(AssetPath::relativeTo(relativePath, config.toString()));
  } else {
    if (relativePath.empty())
      relativePath = "/";
  }
  m_animatorVersion = config.getUInt("version", 0);

  if (version() > 0) {
    if (config.contains("includes"))
      config = mergeIncludes(config, config.get("includes"), relativePath);
  }

  m_animatedParts = AnimatedPartSet(config.get("animatedParts", JsonObject()), version());
  m_relativePath = AssetPath::directory(relativePath);

  for (auto const& pair : config.get("globalTagDefaults", JsonObject()).iterateObject())
    setGlobalTag(pair.first, pair.second.toString());

  for (auto const& part : config.get("partTagDefaults", JsonObject()).iterateObject()) {
    for (auto const& tag : part.second.iterateObject())
      setPartTag(part.first, tag.first, tag.second.toString());
  }

  for (auto const& pair : config.get("transformationGroups", JsonObject()).iterateObject()) {
    auto& tg = m_transformationGroups[pair.first];
    tg.interpolated = pair.second.getBool("interpolated", false);
    tg.setAffineTransform(Mat3F::identity());
    tg.setAnimationAffineTransform(Mat3F::identity());
    tg.setLocalAffineTransform(Mat3F::identity());
  }

  for (auto const& pair : config.get("rotationGroups", JsonObject()).iterateObject()) {
    String rotationGroupName = pair.first;
    Json rotationGroupConfig = pair.second;
    RotationGroup& rotationGroup = m_rotationGroups[std::move(rotationGroupName)];
    rotationGroup.angularVelocity = rotationGroupConfig.getFloat("angularVelocity", 0.0f);
    rotationGroup.rotationCenter = jsonToVec2F(rotationGroupConfig.get("rotationCenter", JsonArray{0, 0}));
  }

  for (auto const& pair : config.get("particleEmitters", JsonObject()).iterateObject()) {
    String particleEmitterName = pair.first;
    Json particleEmitterConfig = pair.second;

    ParticleEmitter& emitter = m_particleEmitters[std::move(particleEmitterName)];
    emitter.emissionRate.set(particleEmitterConfig.getFloat("emissionRate", 1.0f));
    emitter.emissionRateVariance = particleEmitterConfig.getFloat("emissionRateVariance", 0.0f);
    emitter.offsetRegion.set(particleEmitterConfig.opt("offsetRegion").apply(jsonToRectF).value(RectF::null()));
    emitter.anchorPart = particleEmitterConfig.optString("anchorPart");
    emitter.transformationGroups = jsonToStringList(particleEmitterConfig.get("transformationGroups", JsonArray()));
    emitter.rotationGroup = particleEmitterConfig.optString("rotationGroup");
    emitter.rotationCenter = particleEmitterConfig.opt("rotationCenter").apply(jsonToVec2F);

    for (auto const& config : particleEmitterConfig.get("particles").iterateArray()) {
      auto creator = root.particleDatabase()->particleCreator(config.get("particle"), relativePath);
      unsigned count = config.getUInt("count", 1);
      Vec2F offset = jsonToVec2F(config.get("offset", JsonArray{0, 0}));
      bool flip = config.getBool("flip", false);
      emitter.particleList.append({creator, count, offset, flip});
    }

    // default to one cycle through the particle list in a burst
    emitter.burstCount.set(particleEmitterConfig.getUInt("burstCount", 1));

    // default to one of each to preserve current behaviour.
    emitter.randomSelectCount.set(particleEmitterConfig.getUInt("randomSelectCount", emitter.particleList.size()));

    emitter.active.set(particleEmitterConfig.getBool("active", false));
  }

  for (auto const& pair : config.get("lights", JsonObject()).iterateObject()) {
    String lightName = pair.first;
    Json lightConfig = pair.second;

    Light& light = m_lights[std::move(lightName)];
    light.active.set(lightConfig.getBool("active", true));
    auto lightPosition = lightConfig.opt("position").apply(jsonToVec2F).value();
    light.xPosition.set(lightPosition[0]);
    light.yPosition.set(lightPosition[1]);
    light.color.set(lightConfig.opt("color").apply(jsonToColor).value(Color::White));
    light.anchorPart = lightConfig.optString("anchorPart");
    light.transformationGroups = jsonToStringList(lightConfig.get("transformationGroups", JsonArray()));
    light.rotationGroup = lightConfig.optString("rotationGroup");
    light.rotationCenter = lightConfig.opt("rotationCenter").apply(jsonToVec2F);

    if (lightConfig.contains("flickerPeriod")) {
      light.flicker = PeriodicFunction<float>(
          lightConfig.getFloat("flickerPeriod"),
          lightConfig.getFloat("flickerMinIntensity", 0.0),
          lightConfig.getFloat("flickerMaxIntensity", 0.0),
          lightConfig.getFloat("flickerPeriodVariance", 0.0),
          lightConfig.getFloat("flickerIntensityVariance", 0.0)
        );
    }

    light.pointAngle.set(lightConfig.getFloat("pointAngle", 0.0f) * Constants::deg2rad);
    light.pointLight = lightConfig.getBool("pointLight", false);
    light.pointBeam = lightConfig.getFloat("pointBeam", 0.0f);
    light.beamAmbience = lightConfig.getFloat("beamAmbience", 0.0f);
  }

  for (auto const& pair : config.get("sounds", JsonObject()).iterateObject()) {
    String soundName = pair.first;
    Json soundConfig = pair.second;
    Sound& sound = m_sounds[std::move(soundName)];
    if (soundConfig.isType(Json::Type::Array)) {
      sound.rangeMultiplier = 1.0f;
      sound.soundPool.set(jsonToStringList(soundConfig).transformed(bind(&AssetPath::relativeTo, m_relativePath, _1)));
      sound.volumeTarget.set(1.0f);
      sound.volumeRampTime.set(0.0f);
      sound.pitchMultiplierTarget.set(1.0f);
      sound.pitchMultiplierRampTime.set(0.0f);
    } else {
      sound.rangeMultiplier = soundConfig.getFloat("rangeMultiplier", 1.0f);

      auto soundPosition = soundConfig.opt("position").apply(jsonToVec2F).value();
      sound.xPosition.set(soundPosition[0]);
      sound.yPosition.set(soundPosition[1]);

      sound.volumeTarget.set(soundConfig.getFloat("volume", 1.0f));
      sound.volumeRampTime.set(soundConfig.getFloat("volumeRampTime", 0.0f));

      sound.pitchMultiplierTarget.set(soundConfig.getFloat("pitchMultiplier", 1.0f));
      sound.pitchMultiplierRampTime.set(soundConfig.getFloat("pitchMultiplierRampTime", 0.0f));

      sound.soundPool.set(jsonToStringList(soundConfig.get("pool", JsonArray())).transformed(bind(&AssetPath::relativeTo, m_relativePath, _1)));
    }
  }

  for (auto const& pair : config.get("effects", JsonObject()).iterateObject()) {
    String effectName = pair.first;
    Json effectConfig = pair.second;

    Effect& effect = m_effects[effectName];
    effect.type = effectConfig.getString("type");
    effect.time = effectConfig.getFloat("time", 0.0f);
    effect.directives = effectConfig.getString("directives");
  }

  // Sort all the states that contain NetStates handles predictably by key.
  m_transformationGroups.sortByKey();
  m_rotationGroups.sortByKey();
  m_particleEmitters.sortByKey();
  m_lights.sortByKey();
  m_sounds.sortByKey();
  m_effects.sortByKey();

  // Make sure that every state type has an entry in the state info map, and
  // order it predictably by key.
  for (auto const& stateType : m_animatedParts.stateTypes())
    m_stateInfo[stateType];

  m_stateInfo.sortByKey();

  setupNetStates();
}

NetworkedAnimator::NetworkedAnimator(NetworkedAnimator&& animator) {
  operator=(std::move(animator));
}

NetworkedAnimator::NetworkedAnimator(NetworkedAnimator const& animator) {
  operator=(animator);
}

NetworkedAnimator& NetworkedAnimator::operator=(NetworkedAnimator&& animator) {
  m_relativePath = std::move(animator.m_relativePath);
  m_animatedParts = std::move(animator.m_animatedParts);
  m_stateInfo = std::move(animator.m_stateInfo);
  m_transformationGroups = std::move(animator.m_transformationGroups);
  m_rotationGroups = std::move(animator.m_rotationGroups);
  m_particleEmitters = std::move(animator.m_particleEmitters);
  m_lights = std::move(animator.m_lights);
  m_sounds = std::move(animator.m_sounds);
  m_effects = std::move(animator.m_effects);
  m_processingDirectives = std::move(animator.m_processingDirectives);
  m_zoom = std::move(animator.m_zoom);
  m_flipped = std::move(animator.m_flipped);
  m_flippedRelativeCenterLine = std::move(animator.m_flippedRelativeCenterLine);
  m_animationRate = std::move(animator.m_animationRate);
  m_globalTags = std::move(animator.m_globalTags);
  m_partTags = std::move(animator.m_partTags);
  m_cachedPartDrawables = std::move(animator.m_cachedPartDrawables);
  m_partDrawables = std::move(animator.m_partDrawables);
  m_localTags = std::move(animator.m_localTags);

  setupNetStates();

  return *this;
}

NetworkedAnimator& NetworkedAnimator::operator=(NetworkedAnimator const& animator) {
  m_relativePath = animator.m_relativePath;
  m_animatedParts = animator.m_animatedParts;
  m_stateInfo = animator.m_stateInfo;
  m_transformationGroups = animator.m_transformationGroups;
  m_rotationGroups = animator.m_rotationGroups;
  m_particleEmitters = animator.m_particleEmitters;
  m_lights = animator.m_lights;
  m_sounds = animator.m_sounds;
  m_effects = animator.m_effects;
  m_processingDirectives = animator.m_processingDirectives;
  m_zoom = animator.m_zoom;
  m_flipped = animator.m_flipped;
  m_flippedRelativeCenterLine = animator.m_flippedRelativeCenterLine;
  m_animationRate = animator.m_animationRate;
  m_globalTags = animator.m_globalTags;
  m_partTags = animator.m_partTags;
  m_cachedPartDrawables = animator.m_cachedPartDrawables;
  m_partDrawables = animator.m_partDrawables;
  m_localTags = animator.m_localTags;

  setupNetStates();

  return *this;
}

StringList NetworkedAnimator::stateTypes() const {
  return m_animatedParts.stateTypes();
}

StringList NetworkedAnimator::states(String const& stateType) const {
  return m_animatedParts.states(stateType);
}

bool NetworkedAnimator::setState(String const& stateType, String const& state, bool startNew, bool reverse) {
  if (m_animatedParts.setActiveState(stateType, state, startNew, reverse)) {
    m_stateInfo[stateType].wasUpdated = true;
    m_stateInfo[stateType].startedEvent.trigger();
    return true;
  } else {
    return false;
  }
}

bool NetworkedAnimator::setLocalState(String const& stateType, String const& state, bool startNew, bool reverse) {
  return m_animatedParts.setActiveState(stateType, state, startNew, reverse);
}

String NetworkedAnimator::state(String const& stateType) const {
  return m_animatedParts.activeState(stateType).stateName;
}
int NetworkedAnimator::stateFrame(String const& stateType) const {
  return m_animatedParts.activeState(stateType).frame;
}
float NetworkedAnimator::stateFrameProgress(String const& stateType) const {
  return m_animatedParts.activeState(stateType).frameProgress;
}
float NetworkedAnimator::stateTimer(String const& stateType) const {
  return m_animatedParts.activeState(stateType).timer;
}
bool NetworkedAnimator::stateReverse(String const& stateType) const {
  return m_animatedParts.activeState(stateType).reverse;
}

bool NetworkedAnimator::hasState(String const & stateType, Maybe<String> const & state) const {
  if (m_animatedParts.stateTypes().contains(stateType)) {
    if (state) {
      return m_animatedParts.states(stateType).contains(*state);
    }
    return true;
  }
  return false;
}

StringMap<AnimatedPartSet::Part> const& NetworkedAnimator::constParts() const {
  return m_animatedParts.constParts();
}

StringMap<AnimatedPartSet::Part>& NetworkedAnimator::parts() {
  return m_animatedParts.parts();
}

StringList NetworkedAnimator::partNames() const {
  return m_animatedParts.partNames();
}

Json NetworkedAnimator::stateProperty(String const& stateType, String const& propertyName, Maybe<String> state, Maybe<int> frame) const {
  if (state.isValid())
    return m_animatedParts.getStateFrameProperty(stateType, propertyName, *state, *frame);
  return m_animatedParts.activeState(stateType).properties.value(propertyName);
}
Json NetworkedAnimator::stateNextProperty(String const& stateType, String const& propertyName) const {
  return m_animatedParts.activeState(stateType).nextProperties.value(propertyName);
}

Json NetworkedAnimator::partProperty(String const& partName, String const& propertyName, Maybe<String> stateType, Maybe<String> state, Maybe<int> frame) const {
  if (stateType.isValid())
    return m_animatedParts.getPartStateFrameProperty(partName, propertyName, *stateType, *state, *frame);
  return m_animatedParts.activePart(partName).properties.value(propertyName);
}
Json NetworkedAnimator::partNextProperty(String const& partName, String const& propertyName) const {
  return m_animatedParts.activePart(partName).nextProperties.value(propertyName);
}

Mat3F NetworkedAnimator::globalTransformation() const {
  Mat3F transformation = Mat3F::scaling(m_zoom.get());
  if (m_flipped.get())
    transformation = Mat3F::scaling(Vec2F(-1, 1), Vec2F(m_flippedRelativeCenterLine.get(), 0)) * transformation;
  return transformation;
}

Mat3F NetworkedAnimator::groupTransformation(StringList const& transformationGroups) const {
  auto mat = Mat3F::identity();
  for (auto const& tg : transformationGroups)
    mat = m_transformationGroups.get(tg).affineTransform() * m_transformationGroups.get(tg).localAffineTransform() * m_transformationGroups.get(tg).animationAffineTransform() * mat;
  return mat;
}

Mat3F NetworkedAnimator::partTransformation(String const& partName) const {
  auto const& part = m_animatedParts.activePart(partName);
  Mat3F transformation = Mat3F::identity();

  if (auto offset = part.properties.value("offset").opt().apply(jsonToVec2F))
    transformation = Mat3F::translation(*offset) * transformation;

  transformation = part.animationAffineTransform() * transformation;

  auto transformationGroups = jsonToStringList(part.properties.value("transformationGroups", JsonArray()));
  transformation = groupTransformation(transformationGroups) * transformation;

  if (auto rotationGroupName = part.properties.value("rotationGroup").optString()) {
    auto const& rotationGroup = m_rotationGroups.get(*rotationGroupName);
    Vec2F rotationCenter = part.properties.value("rotationCenter").opt().apply(jsonToVec2F).value(rotationGroup.rotationCenter);
    transformation = Mat3F::rotation(rotationGroup.currentAngle, rotationCenter) * transformation;
  }

  if (auto anchorPart = part.properties.ptr("anchorPart"))
    transformation = partTransformation(anchorPart->toString()) * transformation;

  return transformation;
}

Mat3F NetworkedAnimator::finalPartTransformation(String const& partName) const {
  return globalTransformation() * partTransformation(partName);
}

Maybe<Vec2F> NetworkedAnimator::partPoint(String const& partName, String const& propertyName) const {
  auto const& part = m_animatedParts.activePart(partName);
  auto property = part.properties.value(propertyName);
  if (!property)
    return {};

  return finalPartTransformation(partName).transformVec2(jsonToVec2F(property));
}

Maybe<PolyF> NetworkedAnimator::partPoly(String const& partName, String const& propertyName) const {
  auto const& part = m_animatedParts.activePart(partName);
  auto property = part.properties.value(propertyName, {});
  if (!property)
    return {};

  PolyF poly = jsonToPolyF(property);
  poly.transform(finalPartTransformation(partName));
  return poly;
}

void NetworkedAnimator::setGlobalTag(String tagName, Maybe<String> tagValue) {
  if (tagValue)
    m_globalTags.set(std::move(tagName), std::move(*tagValue));
  else
    m_globalTags.remove(tagName);
}

void NetworkedAnimator::removeGlobalTag(String const& tagName) {
  m_globalTags.remove(tagName);
}

String const* NetworkedAnimator::globalTagPtr(String const& tagName) const {
  return m_globalTags.ptr(tagName);
}


void NetworkedAnimator::setPartTag(String const& partType, String tagName, Maybe<String> tagValue) {
  if (tagValue)
    m_partTags[partType].set(std::move(tagName), std::move(*tagValue));
  else
    m_partTags[partType].remove(tagName);
}

void NetworkedAnimator::setLocalTag(String tagName, Maybe<String> tagValue) {
  if (tagValue)
    m_localTags.set(tagName, *tagValue);
  else
    m_localTags.remove(tagName);
}

void NetworkedAnimator::setPartDrawables(String const& partName, List<Drawable> drawables) {
  m_partDrawables.set(partName, drawables);
}
void NetworkedAnimator::addPartDrawables(String const& partName, List<Drawable> drawables) {
  m_partDrawables.ptr(partName)->appendAll(drawables);
}
String NetworkedAnimator::applyPartTags(String const& partName, String apply) const {
  HashMap<String, String> animationTags = m_localTags;
  Maybe<unsigned> frame;
  String frameStr;
  String frameIndexStr;
  auto activePart = m_animatedParts.activePart(partName);
  auto partTags = m_partTags.get(partName);
  if (activePart.activeState) {
    unsigned stateFrame = activePart.activeState->frame;
    frame = stateFrame;
    frameStr = static_cast<String>(toString(stateFrame + 1));
    frameIndexStr = static_cast<String>(toString(stateFrame));
  }
  if (version() > 0)
    m_animatedParts.forEachActiveState([&](String const& stateTypeName, AnimatedPartSet::ActiveStateInformation const& activeState) {
      unsigned stateFrame = activeState.frame;
      Maybe<unsigned> frame;
      String frameStr;
      String frameIndexStr;

      frame = stateFrame;
      frameStr = static_cast<String>(toString(stateFrame + 1));
      frameIndexStr = static_cast<String>(toString(stateFrame));
      if (frame) {
        animationTags.set(stateTypeName + "_frame", frameStr);
        animationTags.set(stateTypeName + "_frameIndex", frameIndexStr);
      }
      animationTags.set(stateTypeName + "_state", activeState.stateName);

      if (auto p = activeState.properties.ptr("animationTags")) {
        for (auto tag : p->iterateObject())
          animationTags.set(tag.first, tag.second.toString());
      }
    });

  auto applied = apply.maybeLookupTagsView([&](StringView tag) -> StringView {
    if (tag == "frame") {
      if (frame)
        return frameStr;
    } else if (tag == "frameIndex") {
      if (frame)
        return frameIndexStr;
    } else if (auto p = animationTags.ptr(tag)) {
      return StringView(*p);
    } else if (auto p = partTags.ptr(tag)) {
      return StringView(*p);
    } else if (auto p = m_globalTags.ptr(tag)) {
      return StringView(*p);
    }

    return StringView("default");
  });
  return applied ? applied.get() : apply;
}


void NetworkedAnimator::setProcessingDirectives(Directives const& directives) {
  m_processingDirectives.set(directives);
}

void NetworkedAnimator::setZoom(float zoom) {
  m_zoom.set(zoom);
}

bool NetworkedAnimator::flipped() const {
  return m_flipped.get();
}

float NetworkedAnimator::flippedRelativeCenterLine() const {
  return m_flippedRelativeCenterLine.get();
}

void NetworkedAnimator::setFlipped(bool flipped, float relativeCenterLine) {
  m_flipped.set(flipped);
  m_flippedRelativeCenterLine.set(relativeCenterLine);
}

void NetworkedAnimator::setAnimationRate(float rate) {
  m_animationRate.set(rate);
}

bool NetworkedAnimator::hasRotationGroup(String const& rotationGroup) const {
  return m_rotationGroups.contains(rotationGroup);
}

void NetworkedAnimator::rotateGroup(String const& rotationGroup, float targetAngle, bool immediate) {
  auto& group = m_rotationGroups.get(rotationGroup);
  group.targetAngle.set(targetAngle);

  if (immediate) {
    group.currentAngle = targetAngle;
    group.netImmediateEvent.trigger();
  }
}

float NetworkedAnimator::currentRotationAngle(String const& rotationGroup) const {
  return m_rotationGroups.get(rotationGroup).currentAngle;
}

bool NetworkedAnimator::hasTransformationGroup(String const& transformationGroup) const {
  return m_transformationGroups.contains(transformationGroup);
}

void NetworkedAnimator::translateTransformationGroup(String const& transformationGroup, Vec2F const& translation) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setAffineTransform(Mat3F::translation(translation) * group.affineTransform());
}

void NetworkedAnimator::rotateTransformationGroup(
    String const& transformationGroup, float rotation, Vec2F const& rotationCenter) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setAffineTransform(Mat3F::rotation(rotation, rotationCenter) * group.affineTransform());
}

void NetworkedAnimator::scaleTransformationGroup(
    String const& transformationGroup, float scale, Vec2F const& scaleCenter) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setAffineTransform(Mat3F::scaling(scale, scaleCenter) * group.affineTransform());
}

void NetworkedAnimator::scaleTransformationGroup(
    String const& transformationGroup, Vec2F const& scale, Vec2F const& scaleCenter) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setAffineTransform(Mat3F::scaling(scale, scaleCenter) * group.affineTransform());
}

void NetworkedAnimator::transformTransformationGroup(
    String const& transformationGroup, float a, float b, float c, float d, float tx, float ty) {
  auto& group = m_transformationGroups.get(transformationGroup);
  Mat3F transform = Mat3F(a, b, tx, c, d, ty, 0, 0, 1);
  group.setAffineTransform(transform * group.affineTransform());
}

void NetworkedAnimator::resetTransformationGroup(String const& transformationGroup) {
  m_transformationGroups.get(transformationGroup).setAffineTransform(Mat3F::identity());
}

void NetworkedAnimator::translateLocalTransformationGroup(String const& transformationGroup, Vec2F const& translation) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setLocalAffineTransform(Mat3F::translation(translation) * group.localAffineTransform());
}

void NetworkedAnimator::rotateLocalTransformationGroup(
    String const& transformationGroup, float rotation, Vec2F const& rotationCenter) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setLocalAffineTransform(Mat3F::rotation(rotation, rotationCenter) * group.localAffineTransform());
}

void NetworkedAnimator::scaleLocalTransformationGroup(
    String const& transformationGroup, float scale, Vec2F const& scaleCenter) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setLocalAffineTransform(Mat3F::scaling(scale, scaleCenter) * group.localAffineTransform());
}

void NetworkedAnimator::scaleLocalTransformationGroup(
    String const& transformationGroup, Vec2F const& scale, Vec2F const& scaleCenter) {
  auto& group = m_transformationGroups.get(transformationGroup);
  group.setLocalAffineTransform(Mat3F::scaling(scale, scaleCenter) * group.localAffineTransform());
}

void NetworkedAnimator::transformLocalTransformationGroup(
    String const& transformationGroup, float a, float b, float c, float d, float tx, float ty) {
  auto& group = m_transformationGroups.get(transformationGroup);
  Mat3F transform = Mat3F(a, b, tx, c, d, ty, 0, 0, 1);
  group.setLocalAffineTransform(transform * group.localAffineTransform());
}

void NetworkedAnimator::resetLocalTransformationGroup(String const& transformationGroup) {
  m_transformationGroups.get(transformationGroup).setLocalAffineTransform(Mat3F::identity());
}

bool NetworkedAnimator::hasParticleEmitter(String const& emitterName) const {
  return m_particleEmitters.contains(emitterName);
}

void NetworkedAnimator::setParticleEmitterActive(String const& emitterName, bool active) {
  m_particleEmitters.get(emitterName).active.set(active);
}

void NetworkedAnimator::setParticleEmitterEmissionRate(String const& emitterName, float emissionRate) {
  m_particleEmitters.get(emitterName).emissionRate.set(emissionRate);
}

void NetworkedAnimator::setParticleEmitterOffsetRegion(String const& emitterName, RectF const& offsetRegion) {
  m_particleEmitters.get(emitterName).offsetRegion.set(offsetRegion);
}

void NetworkedAnimator::setParticleEmitterBurstCount(String const& emitterName, unsigned burstCount) {
  m_particleEmitters.get(emitterName).burstCount.set(burstCount);
}

void NetworkedAnimator::burstParticleEmitter(String const& emitterName) {
  m_particleEmitters.get(emitterName).burstEvent.trigger();
}

bool NetworkedAnimator::hasLight(String const& lightName) const {
  return m_lights.contains(lightName);
}

void NetworkedAnimator::setLightActive(String const& lightName, bool active) {
  m_lights.get(lightName).active.set(active);
}

void NetworkedAnimator::setLightPosition(String const& lightName, Vec2F position) {
  auto& light = m_lights.get(lightName);
  light.xPosition.set(position[0]);
  light.yPosition.set(position[1]);
}

void NetworkedAnimator::setLightColor(String const& lightName, Color color) {
  m_lights.get(lightName).color.set(color);
}

void NetworkedAnimator::setLightPointAngle(String const& lightName, float angle) {
  m_lights.get(lightName).pointAngle.set(angle * Constants::deg2rad);
}

bool NetworkedAnimator::hasSound(String const& soundName) const {
  return m_sounds.contains(soundName);
}

void NetworkedAnimator::setSoundPool(String const& soundName, StringList soundPool) {
  m_sounds.get(soundName).soundPool.set(std::move(soundPool));
}

void NetworkedAnimator::setSoundPosition(String const& soundName, Vec2F const& position) {
  auto& sound = m_sounds.get(soundName);
  sound.xPosition.set(position[0]);
  sound.yPosition.set(position[1]);
}

void NetworkedAnimator::setSoundVolume(String const& soundName, float volume, float rampTime) {
  auto& sound = m_sounds.get(soundName);
  sound.volumeTarget.set(volume);
  sound.volumeRampTime.set(rampTime);
}

void NetworkedAnimator::setSoundPitchMultiplier(String const& soundName, float pitchMultiplier, float rampTime) {
  auto& sound = m_sounds.get(soundName);
  sound.pitchMultiplierTarget.set(pitchMultiplier);
  sound.pitchMultiplierRampTime.set(rampTime);
}

void NetworkedAnimator::playSound(String const& soundName, int loops) {
  auto& sound = m_sounds.get(soundName);
  sound.loops.set(loops);
  sound.signals.send(SoundSignal::Play);
}

void NetworkedAnimator::stopAllSounds(String const& soundName, float rampTime) {
  auto& sound = m_sounds.get(soundName);
  sound.volumeRampTime.set(rampTime);
  sound.signals.send(SoundSignal::StopAll);
}

void NetworkedAnimator::setEffectEnabled(String const& effect, bool enabled) {
  m_effects.get(effect).enabled.set(enabled);
}

List<Drawable> NetworkedAnimator::drawables(Vec2F const& position) const {
  List<Drawable> drawables;
  for (auto& p : drawablesWithZLevel(position))
    drawables.append(std::move(p.first));
  return drawables;
}

List<pair<Drawable, float>> NetworkedAnimator::drawablesWithZLevel(Vec2F const& position) const {
  size_t partCount = m_animatedParts.constParts().size();
  if (!partCount)
    return {};

  List<Directives> baseProcessingDirectives = { m_processingDirectives.get() };
  for (auto& pair : m_effects) {
    auto const& effectState = pair.second;

    if (effectState.enabled.get()) {
      auto const& effect = m_effects.get(pair.first);
      if (effect.type == "flash") {
        if (effectState.timer > effect.time / 2) {
          baseProcessingDirectives.append(effect.directives);
        }
      } else if (effect.type == "directive") {
        baseProcessingDirectives.append(effect.directives);
      } else {
        throw NetworkedAnimatorException(strf("No such NetworkedAnimator effect type '{}'", effect.type));
      }
    }
  }
  HashMap<String, String> animationTags = m_localTags;
  if (version() > 0)
    m_animatedParts.forEachActiveState([&](String const& stateTypeName, AnimatedPartSet::ActiveStateInformation const& activeState) {
      unsigned stateFrame = activeState.frame;
      Maybe<unsigned> frame;
      String frameStr;
      String frameIndexStr;

      frame = stateFrame;
      frameStr = static_cast<String>(toString(stateFrame + 1));
      frameIndexStr = static_cast<String>(toString(stateFrame));
      if (frame) {
        animationTags.set(stateTypeName + "_frame", frameStr);
        animationTags.set(stateTypeName + "_frameIndex", frameIndexStr);
      }
      animationTags.set(stateTypeName + "_state", activeState.stateName);

      if (auto p = activeState.properties.ptr("animationTags")) {
        for (auto tag : p->iterateObject())
          animationTags.set(tag.first, tag.second.toString());
      }
    });

  List<tuple<AnimatedPartSet::ActivePartInformation const*, String const*, float>> parts;
  parts.reserve(partCount);
  int drawableCount = 0;
  m_animatedParts.forEachActivePart([&](String const& partName, AnimatedPartSet::ActivePartInformation const& activePart) {
    Maybe<float> maybeZLevel;
    if (m_flipped.get()) {
      if (auto maybeFlipped = activePart.properties.value("flippedZLevel").optFloat())
        maybeZLevel = *maybeFlipped;
    }
    if (!maybeZLevel)
      maybeZLevel = activePart.properties.value("zLevel").optFloat();

    if (auto drawables = m_partDrawables.contains(partName))
      drawableCount += m_partDrawables.get(partName).size();
    parts.append(make_tuple(&activePart, &partName, maybeZLevel.value(0.0f)));
  });

  sort(parts, [](auto const& a, auto const& b) { return get<2>(a) < get<2>(b); });

  List<pair<Drawable, float>> drawables;
  drawables.reserve(partCount + drawableCount);
  for (auto& entry : parts) {
    auto& activePart = *get<0>(entry);
    auto& partName = *get<1>(entry);
    // Make sure we don't copy the original image
    String fallback = "";
    Json jImage = activePart.properties.value("image", {});
    String const& image = jImage.isType(Json::Type::String) ? *jImage.stringPtr() : fallback;

    bool centered = activePart.properties.value("centered").optBool().value(true);
    bool fullbright = activePart.properties.value("fullbright").optBool().value(false);

    size_t originalDirectivesSize = baseProcessingDirectives.size();

    auto const& partTags = m_partTags.get(partName);

    if (auto directives = activePart.properties.value("processingDirectives").optString()) {
      if (version() > 0){
        directives = directives->maybeLookupTagsView([&](StringView tag) -> StringView {
          if (auto p = animationTags.ptr(tag)) {
            return StringView(*p);
          } else if (auto p = partTags.ptr(tag)) {
            return StringView(*p);
          } else if (auto p = m_globalTags.ptr(tag)) {
            return StringView(*p);
          }
          return StringView("default");
        });
      }
      baseProcessingDirectives.append(*directives);
    }

    Maybe<unsigned> frame;
    String frameStr;
    String frameIndexStr;
    if (activePart.activeState) {
      unsigned stateFrame = activePart.activeState->frame;
      frame = stateFrame;
      frameStr = static_cast<String>(toString(stateFrame + 1));
      frameIndexStr = static_cast<String>(toString(stateFrame));

      if (auto directives = activePart.activeState->properties.value("processingDirectives").optString()) {
        if (version() > 0){
          directives = directives->maybeLookupTagsView([&](StringView tag) -> StringView {
            if (auto p = animationTags.ptr(tag)) {
              return StringView(*p);
            } else if (auto p = partTags.ptr(tag)) {
              return StringView(*p);
            } else if (auto p = m_globalTags.ptr(tag)) {
              return StringView(*p);
            }
            return StringView("default");
          });
        }
        baseProcessingDirectives.append(*directives);
      }
    }

    Maybe<String> processedImage = image.maybeLookupTagsView([&](StringView tag) -> StringView {
      if (tag == "frame") {
        if (frame)
          return frameStr;
      } else if (tag == "frameIndex") {
        if (frame)
          return frameIndexStr;
      } else if (auto p = animationTags.ptr(tag)) {
        return StringView(*p);
      } else if (auto p = partTags.ptr(tag)) {
        return StringView(*p);
      } else if (auto p = m_globalTags.ptr(tag)) {
        return StringView(*p);
      }

      return StringView("default");
    });
    String const& usedImage = processedImage ? processedImage.get() : image;

    auto transformation = globalTransformation() * partTransformation(partName);
    transformation.translate(position);

    if (!usedImage.empty() && usedImage[0] != ':' && usedImage[0] != '?') {
      size_t hash = hashOf(usedImage);
      auto find = m_cachedPartDrawables.find(partName);
      if (find == m_cachedPartDrawables.end() || find->second.first != hash) {
        String relativeImage;
        if (usedImage[0] != '/')
          relativeImage = AssetPath::relativeTo(m_relativePath, usedImage);

        Drawable drawable = Drawable::makeImage(!relativeImage.empty() ? relativeImage : usedImage, 1.0f / TilePixels, centered, Vec2F());
        if (find == m_cachedPartDrawables.end())
          find = m_cachedPartDrawables.emplace(partName, std::pair{ hash, std::move(drawable) }).first;
        else {
          find->second.first = hash;
          find->second.second = std::move(drawable);
        }
      }

      Drawable drawable = find->second.second;
      auto& imagePart = drawable.imagePart();
      for (Directives const& directives : baseProcessingDirectives)
        imagePart.addDirectives(directives, centered);
      drawable.fullbright = fullbright;
      drawable.transform(transformation);
      drawables.append({std::move(drawable), get<2>(entry)});
    }

    if (m_partDrawables.contains(partName)) {
      auto partDrawables = m_partDrawables.get(partName);
      Drawable::transformAll(partDrawables, transformation);
      for (auto drawable : partDrawables) {
      drawables.append({drawable, get<2>(entry)});
      }
    }

    baseProcessingDirectives.resize(originalDirectivesSize);
  }

  return drawables;
}

List<LightSource> NetworkedAnimator::lightSources(Vec2F const& translate) const {
  List<LightSource> lightSources;
  for (auto const& pair : m_lights) {
    if (!pair.second.active.get())
      continue;

    Vec2F position = {pair.second.xPosition.get(), pair.second.yPosition.get()};
    float pointAngle = constrainAngle(pair.second.pointAngle.get());
    Mat3F transformation = Mat3F::identity();
    if (pair.second.anchorPart)
      transformation = partTransformation(*pair.second.anchorPart);
    transformation = groupTransformation(pair.second.transformationGroups) * transformation;
    position = transformation.transformVec2(position);
    pointAngle = transformation.transformAngle(pointAngle);
    if (pair.second.rotationGroup) {
      auto const& rg = m_rotationGroups.get(*pair.second.rotationGroup);
      position = (position - pair.second.rotationCenter.value(rg.rotationCenter)).rotate(rg.currentAngle)
          + pair.second.rotationCenter.value(rg.rotationCenter);
      pointAngle += rg.currentAngle;
    }
    position = globalTransformation().transformVec2(position);
    if (m_flipped.get()) {
      if (pointAngle > 0)
        pointAngle = Constants::pi / 2 + constrainAngle(Constants::pi / 2 - pointAngle);
      else
        pointAngle = -Constants::pi / 2 - constrainAngle(pointAngle + Constants::pi / 2);
    }

    Color color = pair.second.color.get();
    if (pair.second.flicker)
      color.setValue(clamp(color.value() * pair.second.flicker->value(SinWeightOperator<float>()), 0.0f, 1.0f));

    lightSources.append(LightSource{
      position + translate,
      color.toRgbF(),
      pair.second.pointLight ? LightType::Point : LightType::Spread,
      pair.second.pointBeam,
      pointAngle,
      pair.second.beamAmbience
    });
  }
  return lightSources;
}

void NetworkedAnimator::update(float dt, DynamicTarget* dynamicTarget) {
  dt *= m_animationRate.get();

  m_animatedParts.update(dt);

  m_animatedParts.forEachActiveState([&](String const& stateTypeName, AnimatedPartSet::ActiveStateInformation const& activeState) {
      if (dynamicTarget) {
        dynamicTarget->clearFinishedAudio();

        Json persistentSound = activeState.properties.value("persistentSound", "");
        String persistentSoundFile;

        if (persistentSound.isType(Json::Type::String))
          persistentSoundFile = persistentSound.toString();
        else if (persistentSound.isType(Json::Type::Array))
          persistentSoundFile = Random::randValueFrom(persistentSound.toArray(), "").toString();

        if (!persistentSoundFile.empty())
          persistentSoundFile = AssetPath::relativeTo(m_relativePath, persistentSoundFile);

        auto& activePersistentSound = dynamicTarget->statePersistentSounds[stateTypeName];

        bool changedPersistentSound = persistentSound != activePersistentSound.sound;
        if (changedPersistentSound || !activePersistentSound.audio) {
          activePersistentSound.sound = std::move(persistentSound);
          if (activePersistentSound.audio)
            activePersistentSound.audio->stop(activePersistentSound.stopRampTime);

          if (!persistentSoundFile.empty()) {
            activePersistentSound.audio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(persistentSoundFile));
            activePersistentSound.audio->setRangeMultiplier(activeState.properties.value("persistentSoundRangeMultiplier", 1.0f).toFloat());
            activePersistentSound.audio->setLoops(-1);
            activePersistentSound.audio->setPosition(globalTransformation().transformVec2(Vec2F()));
            activePersistentSound.stopRampTime = activeState.properties.value("persistentSoundStopTime", 0.0f).toFloat();
            dynamicTarget->pendingAudios.append(activePersistentSound.audio);
          } else {
            dynamicTarget->statePersistentSounds.remove(stateTypeName);
          }
        }

        Json immediateSound = activeState.properties.value("immediateSound", "");
        String immediateSoundFile = "";

        if (immediateSound.isType(Json::Type::String))
          immediateSoundFile = immediateSound.toString();
        else if (immediateSound.isType(Json::Type::Array))
          immediateSoundFile = Random::randValueFrom(immediateSound.toArray(), "").toString();

        if (!immediateSoundFile.empty())
          immediateSoundFile = AssetPath::relativeTo(m_relativePath, immediateSoundFile);

        auto& activeImmediateSound = dynamicTarget->stateImmediateSounds[stateTypeName];

        bool changedImmediateSound = immediateSound != activeImmediateSound.sound;
        if (changedImmediateSound) {
          activeImmediateSound.sound = std::move(immediateSound);
          if (!immediateSoundFile.empty()) {
            activeImmediateSound.audio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(immediateSoundFile));
            activeImmediateSound.audio->setRangeMultiplier(activeState.properties.value("immediateSoundRangeMultiplier", 1.0f).toFloat());
            activeImmediateSound.audio->setPosition(globalTransformation().transformVec2(Vec2F()));
            dynamicTarget->pendingAudios.append(activeImmediateSound.audio);
          }
        }
      }

      if (auto lightsOn = activeState.properties.ptr("lightsOn")) {
        for (auto const& name : lightsOn->iterateArray())
          m_lights.get(name.toString()).active.set(true);
      }
      if (auto lightsOff = activeState.properties.ptr("lightsOff")) {
        for (auto const& name : lightsOff->iterateArray())
          m_lights.get(name.toString()).active.set(false);
      }

      if (auto particleEmittersOn = activeState.properties.ptr("particleEmittersOn")) {
        for (auto const& name : particleEmittersOn->iterateArray())
          m_particleEmitters.get(name.toString()).active.set(true);
      }
      if (auto particleEmittersOff = activeState.properties.ptr("particleEmittersOff")) {
        for (auto const& name : particleEmittersOff->iterateArray())
          m_particleEmitters.get(name.toString()).active.set(false);
      }

      if (version() > 0){
        auto processTransforms = [](Mat3F mat, JsonArray transforms, JsonObject properties) -> Mat3F {
          for (auto const& v : transforms) {
            auto action = v.getString(0);
            if (action == "reset") {
              mat = Mat3F::identity();
            } else if (action == "translate") {
              mat.translate(jsonToVec2F(v.getArray(1)));
            } else if (action == "rotate") {
              mat.rotate(v.getFloat(1), jsonToVec2F(v.getArray(2, properties.maybe("rotationCenter").value(JsonArray({0,0})).toArray())));
            } else if (action == "scale") {
              mat.scale(jsonToVec2F(v.getArray(1)), jsonToVec2F(v.getArray(2, properties.maybe("scalingCenter").value(JsonArray({0,0})).toArray())));
            } else if (action == "transform") {
              mat = Mat3F(v.getFloat(1), v.getFloat(2), v.getFloat(3), v.getFloat(4), v.getFloat(5), v.getFloat(6), 0, 0, 1) * mat;
            }
          }
          return mat;
        };


        for (auto& pair : m_transformationGroups) {
          if (auto transforms = activeState.properties.ptr(pair.first)) {
            auto mat = processTransforms(pair.second.animationAffineTransform(), transforms->toArray(), activeState.properties);
            if (pair.second.interpolated) {
              if (auto nextTransforms = activeState.nextProperties.ptr(pair.first)) {
                auto nextMat = processTransforms(pair.second.animationAffineTransform(), nextTransforms->toArray(), activeState.nextProperties);
                pair.second.setAnimationAffineTransform(mat, nextMat, activeState.frameProgress);
              } else {
                pair.second.setAnimationAffineTransform(mat);
              }
            } else {
              pair.second.setAnimationAffineTransform(mat);
            }
          }
        }
      }
    });

  for (auto& pair : m_rotationGroups) {
    auto& rotationGroup = pair.second;
    if (rotationGroup.angularVelocity == 0.0f)
      rotationGroup.currentAngle = rotationGroup.targetAngle.get();
    else
      rotationGroup.currentAngle = approachAngle(rotationGroup.targetAngle.get(), rotationGroup.currentAngle, rotationGroup.angularVelocity * dt);
  }

  if (dynamicTarget) {
    auto addParticles = [this, dynamicTarget](ParticleEmitter::ParticleConfig const& config, RectF const& offsetRegion, Mat3F const& transformation) {
      for (unsigned i = 0; i < config.count; ++i) {
        Particle particle = config.creator();
        particle.position += config.offset;

        if (!offsetRegion.isNull()) {
          particle.position[0] += Random::randf() * offsetRegion.width() + offsetRegion.xMin();
          particle.position[1] += Random::randf() * offsetRegion.height() + offsetRegion.yMin();
        }

        float speed = particle.velocity.magnitude();
        particle.velocity = Vec2F::withAngle(transformation.transformAngle(particle.velocity.angle())) * speed;
        particle.position = transformation.transformVec2(particle.position);
        particle.rotation = transformation.transformAngle(particle.rotation);

        particle.size *= m_zoom.get();
        if (config.flip)
          particle.flip = !particle.flip;

        if (transformation.determinant() < 0) {
          particle.flip = !particle.flip;
          particle.rotation += Constants::pi;
        }

        dynamicTarget->pendingParticles.append(std::move(particle));
      }
    };

    for (auto& pair : m_particleEmitters) {
      Mat3F transformation = Mat3F::identity();
      if (pair.second.anchorPart)
        transformation = partTransformation(*pair.second.anchorPart);
      transformation = groupTransformation(pair.second.transformationGroups) * transformation;

      if (pair.second.rotationGroup) {
        auto const& rg = m_rotationGroups.get(*pair.second.rotationGroup);
        Vec2F rotationCenter = pair.second.rotationCenter.value(rg.rotationCenter);
        transformation = Mat3F::rotation(rg.currentAngle, rotationCenter) * transformation;
      }

      transformation = globalTransformation() * transformation;

      // assume we emit no particles
      unsigned numEmissionCycles = 0;

      if (pair.second.active.get()) {
        pair.second.timer = min(pair.second.timer, 1.0f / (pair.second.emissionRate.get() + pair.second.emissionRateVariance));
        if (pair.second.timer <= 0.0f) {
          // timer causes us to emit one set
          ++numEmissionCycles;
          pair.second.timer = 1.0f / (pair.second.emissionRate.get() + Random::randf(-pair.second.emissionRateVariance, pair.second.emissionRateVariance));
        } else {
          pair.second.timer -= dt;
        }
      }

      auto bursts = pair.second.burstEvent.pullOccurrences();
      for (uint64_t i = 0; i < bursts; ++i)
        numEmissionCycles += pair.second.burstCount.get();

      if (numEmissionCycles > 0) {
        RectF rect = pair.second.offsetRegion.get();
        unsigned numToSelect = pair.second.randomSelectCount.get();

        for (unsigned i = 0; i < numEmissionCycles; ++i) {
          if (numToSelect >= pair.second.particleList.size()) {
            for (auto const& particleConfig : pair.second.particleList)
              addParticles(particleConfig, rect, transformation);
          } else {
            List<ParticleEmitter::ParticleConfig> shuffledList = pair.second.particleList;
            Random::shuffle(shuffledList);

            for (unsigned i = 0; i < numToSelect; ++i)
              addParticles(shuffledList.at(i), rect, transformation);
          }
        }
      }
    }

    for (auto& pair : m_sounds) {
      auto const& soundName = pair.first;
      auto& soundEntry = pair.second;

      for (auto signal : soundEntry.signals.receive()) {
        if (signal == SoundSignal::StopAll) {
          for (auto& sound : take(dynamicTarget->independentSounds[soundName]))
            sound->stop(soundEntry.volumeRampTime.get());
        } else if (signal == SoundSignal::Play) {
          String soundFile = Random::randValueFrom(soundEntry.soundPool.get());
          if (!soundFile.empty()) {
            auto sound = make_shared<AudioInstance>(*Root::singleton().assets()->audio(soundFile));
            sound->setRangeMultiplier(soundEntry.rangeMultiplier);
            sound->setLoops(soundEntry.loops.get());
            sound->setPosition(globalTransformation().transformVec2(Vec2F(soundEntry.xPosition.get(), soundEntry.yPosition.get())));
            sound->setVolume(soundEntry.volumeTarget.get(), soundEntry.volumeRampTime.get());
            sound->setPitchMultiplier(soundEntry.pitchMultiplierTarget.get(), soundEntry.pitchMultiplierRampTime.get());
            dynamicTarget->independentSounds[soundName].append(sound);
            dynamicTarget->pendingAudios.append(std::move(sound));
          }
        }
      }

      // Update all still active independent sounds position, volume, and speed
      for (auto const& activeIndependentSound : dynamicTarget->independentSounds.value(soundName)) {
        if (auto basePosition = dynamicTarget->currentAudioBasePositions.ptr(activeIndependentSound))
          *basePosition = globalTransformation().transformVec2(Vec2F(soundEntry.xPosition.get(), soundEntry.yPosition.get()));
        activeIndependentSound->setVolume(soundEntry.volumeTarget.get(), soundEntry.volumeRampTime.get());
        activeIndependentSound->setPitchMultiplier(soundEntry.pitchMultiplierTarget.get(), soundEntry.pitchMultiplierRampTime.get());
      }
    }
  }

  for (auto& pair : m_lights) {
    if (pair.second.flicker)
      pair.second.flicker->update(dt);
  }

  for (auto& pair : m_effects) {
    if (pair.second.enabled.get()) {
      auto& effect = pair.second;
      if (effect.timer <= 0.0f)
        effect.timer = effect.time;
      else
        effect.timer -= dt;
    }
  }
}

void NetworkedAnimator::finishAnimations() {
  m_animatedParts.finishAnimations();
}

Mat3F NetworkedAnimator::TransformationGroup::affineTransform() const {
  return Mat3F(
      xScale.get() * cos(xShear.get()), xScale.get() * sin(xShear.get()), xTranslation.get(),
      yScale.get() * sin(yShear.get()), yScale.get() * cos(yShear.get()), yTranslation.get(),
      0, 0, 1
    );
}

void NetworkedAnimator::TransformationGroup::setAffineTransform(Mat3F const& matrix) {
  xTranslation.set(matrix[0][2]);
  yTranslation.set(matrix[1][2]);
  xScale.set(sqrt(square(matrix[0][0]) + square(matrix[0][1])));
  yScale.set(sqrt(square(matrix[1][0]) + square(matrix[1][1])));
  xShear.set(atan2(matrix[0][1], matrix[0][0]));
  yShear.set(atan2(matrix[1][0], matrix[1][1]));
}

void NetworkedAnimator::TransformationGroup::setLocalAffineTransform(Mat3F const& matrix) {
  localTransform = matrix;
}

Mat3F NetworkedAnimator::TransformationGroup::localAffineTransform() const {
  return localTransform;
}

void NetworkedAnimator::TransformationGroup::setAnimationAffineTransform(Mat3F const& matrix) {
  xTranslationAnimation = matrix[0][2];
  yTranslationAnimation = matrix[1][2];
  xScaleAnimation = sqrt(square(matrix[0][0]) + square(matrix[0][1]));
  yScaleAnimation = sqrt(square(matrix[1][0]) + square(matrix[1][1]));
  xShearAnimation = atan2(matrix[0][1], matrix[0][0]);
  yShearAnimation = atan2(matrix[1][0], matrix[1][1]);
}
void NetworkedAnimator::TransformationGroup::setAnimationAffineTransform(Mat3F const& mat1, Mat3F const& mat2, float progress) {
  xTranslationAnimation = mat1[0][2];
  yTranslationAnimation = mat1[1][2];
  xScaleAnimation = sqrt(square(mat1[0][0]) + square(mat1[0][1]));
  yScaleAnimation = sqrt(square(mat1[1][0]) + square(mat1[1][1]));
  xShearAnimation = atan2(mat1[0][1], mat1[0][0]);
  yShearAnimation = atan2(mat1[1][0], mat1[1][1]);

  xTranslationAnimation += (mat2[0][2] - xTranslationAnimation) * progress;
  yTranslationAnimation += (mat2[1][2] - yTranslationAnimation) * progress;
  xScaleAnimation += (sqrt(square(mat2[0][0]) + square(mat2[0][1])) - xScaleAnimation) * progress;
  yScaleAnimation += (sqrt(square(mat2[1][0]) + square(mat2[1][1])) - yScaleAnimation) * progress;
  xShearAnimation += (atan2(mat2[0][1], mat2[0][0]) - xShearAnimation) * progress;
  yShearAnimation += (atan2(mat2[1][0], mat2[1][1]) - yShearAnimation) * progress;

}

Mat3F NetworkedAnimator::TransformationGroup::animationAffineTransform() const {
  return Mat3F(
      xScaleAnimation * cos(xShearAnimation), xScaleAnimation * sin(xShearAnimation), xTranslationAnimation,
      yScaleAnimation * sin(yShearAnimation), yScaleAnimation * cos(yShearAnimation), yTranslationAnimation,
      0, 0, 1
    );
}

void NetworkedAnimator::setupNetStates() {
  clearNetElements();

  addNetElement(&m_processingDirectives);
  addNetElement(&m_zoom);
  addNetElement(&m_flipped);
  addNetElement(&m_flippedRelativeCenterLine);

  addNetElement(&m_animationRate);
  m_animationRate.setInterpolator(lerp<float, float>);

  addNetElement(&m_globalTags);

  for (auto const& part : sorted(m_animatedParts.partNames()))
    addNetElement(&m_partTags[part]);

  for (auto& pair : m_stateInfo) {
    pair.second.wasUpdated = true;
    pair.second.reverse.setCompatibilityVersion(8);
    addNetElement(&pair.second.reverse);
    addNetElement(&pair.second.stateIndex);
    addNetElement(&pair.second.startedEvent);
  }

  for (auto& pair : m_transformationGroups) {
    addNetElement(&pair.second.xTranslation);
    addNetElement(&pair.second.yTranslation);
    addNetElement(&pair.second.xScale);
    addNetElement(&pair.second.yScale);
    addNetElement(&pair.second.xShear);
    addNetElement(&pair.second.yShear);

    if (pair.second.interpolated) {
      pair.second.xTranslation.setInterpolator(lerp<float, float>);
      pair.second.yTranslation.setInterpolator(lerp<float, float>);
      pair.second.xScale.setInterpolator(lerp<float, float>);
      pair.second.yScale.setInterpolator(lerp<float, float>);
      pair.second.xShear.setInterpolator(angleLerp<float, float>);
      pair.second.yShear.setInterpolator(angleLerp<float, float>);
    }
  }

  for (auto& pair : m_rotationGroups) {
    addNetElement(&pair.second.targetAngle);
    addNetElement(&pair.second.netImmediateEvent);
  }

  for (auto& pair : m_particleEmitters) {
    addNetElement(&pair.second.emissionRate);
    addNetElement(&pair.second.burstCount);
    addNetElement(&pair.second.randomSelectCount);
    addNetElement(&pair.second.offsetRegion);
    addNetElement(&pair.second.active);
    addNetElement(&pair.second.burstEvent);

    pair.second.burstEvent.setIgnoreOccurrencesOnNetLoad(true);
  }

  for (auto& pair : m_lights) {
    addNetElement(&pair.second.active);
    addNetElement(&pair.second.xPosition);
    addNetElement(&pair.second.yPosition);
    addNetElement(&pair.second.color);
    addNetElement(&pair.second.pointAngle);

    pair.second.xPosition.setFixedPointBase(0.0125f);
    pair.second.yPosition.setFixedPointBase(0.0125f);
    pair.second.pointAngle.setFixedPointBase(0.01f);

    pair.second.xPosition.setInterpolator(lerp<float, float>);
    pair.second.yPosition.setInterpolator(lerp<float, float>);
    pair.second.pointAngle.setInterpolator(angleLerp<float, float>);
  }

  for (auto& pair : m_sounds) {
    addNetElement(&pair.second.soundPool);
    addNetElement(&pair.second.xPosition);
    addNetElement(&pair.second.yPosition);
    addNetElement(&pair.second.volumeTarget);
    addNetElement(&pair.second.volumeRampTime);
    addNetElement(&pair.second.pitchMultiplierTarget);
    addNetElement(&pair.second.pitchMultiplierRampTime);
    addNetElement(&pair.second.loops);
    addNetElement(&pair.second.signals);

    pair.second.xPosition.setFixedPointBase(0.0125f);
    pair.second.yPosition.setFixedPointBase(0.0125f);

    pair.second.xPosition.setInterpolator(lerp<float, float>);
    pair.second.yPosition.setInterpolator(lerp<float, float>);
  }

  for (auto& pair : m_effects)
    addNetElement(&pair.second.enabled);

}

void NetworkedAnimator::netElementsNeedLoad(bool initial) {
  for (auto& pair : m_stateInfo) {
    if (pair.second.startedEvent.pullOccurred() || initial)
      m_animatedParts.setActiveStateIndex(pair.first, pair.second.stateIndex.get(), true, pair.second.reverse.get());
  }

  for (auto& pair : m_rotationGroups) {
    if (pair.second.netImmediateEvent.pullOccurred() || initial)
      pair.second.currentAngle = pair.second.targetAngle.get();
  }
}

void NetworkedAnimator::netElementsNeedStore() {
  for (auto& pair : m_stateInfo) {
    if (pair.second.wasUpdated || (version() < 1)) {
      pair.second.stateIndex.set(m_animatedParts.activeStateIndex(pair.first));
      pair.second.reverse.set(m_animatedParts.activeStateReverse(pair.first));
    }
  }
}

uint8_t NetworkedAnimator::version() const {
  return m_animatorVersion;
}

Json NetworkedAnimator::mergeIncludes(Json config, Json includes, String relativePath){
  for (Json const& path : includes.iterateArray()) {
    auto includeConfig = Root::singleton().assets()->json(AssetPath::relativeTo(relativePath, path.toString()));
    if (includeConfig.contains("includes"))
      includeConfig = mergeIncludes(includeConfig, includeConfig.get("includes"), relativePath);
    config = jsonMerge(includeConfig, config);
  }
  return config;
}

}
