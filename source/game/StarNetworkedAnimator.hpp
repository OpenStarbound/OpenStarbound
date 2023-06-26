#ifndef STAR_NETWORKED_ANIMATOR_HPP
#define STAR_NETWORKED_ANIMATOR_HPP

#include "StarPeriodicFunction.hpp"
#include "StarAnimatedPartSet.hpp"
#include "StarNetElementSystem.hpp"
#include "StarDrawable.hpp"
#include "StarParticle.hpp"
#include "StarLightSource.hpp"
#include "StarMixer.hpp"

namespace Star {

STAR_CLASS(NetworkedAnimator);
STAR_EXCEPTION(NetworkedAnimatorException, StarException);

// Wraps an AnimatedPartSet with a set of optional light sources and particle
// emitters to produce a network capable animation system.
class NetworkedAnimator : public NetElementSyncGroup {
public:
  // Target for dynamic render data such as sounds and particles that are not
  // persistent and are instead produced during a call to update, and may need
  // to be tracked over time.
  class DynamicTarget {
  public:
    // Calls stopAudio()
    ~DynamicTarget();

    List<AudioInstancePtr> pullNewAudios();
    List<Particle> pullNewParticles();

    // Stops all looping audio immediately and lets non-looping audio finish
    // normally
    void stopAudio();

    // Updates the base position of all un-pulled particles and all active
    // audio.  Not necessary to call, but if not called all pulled data will be
    // relative to (0, 0).
    void updatePosition(Vec2F const& position);

  private:
    friend class NetworkedAnimator;

    void clearFinishedAudio();

    struct PersistentSound {
      String file;
      AudioInstancePtr audio;
      float stopRampTime;
    };

    struct ImmediateSound {
      String file;
      AudioInstancePtr audio;
    };

    Vec2F position;
    List<AudioInstancePtr> pendingAudios;
    List<Particle> pendingParticles;
    StringMap<PersistentSound> statePersistentSounds;
    StringMap<ImmediateSound> stateImmediateSounds;
    StringMap<List<AudioInstancePtr>> independentSounds;
    HashMap<AudioInstancePtr, Vec2F> currentAudioBasePositions;
  };

  NetworkedAnimator();
  // If passed a string as config, NetworkedAnimator will interpret this as a
  // config path, otherwise it is interpreted as the literal config.
  NetworkedAnimator(Json config, String relativePath = String());

  NetworkedAnimator(NetworkedAnimator&& animator);
  NetworkedAnimator(NetworkedAnimator const& animator);

  NetworkedAnimator& operator=(NetworkedAnimator&& animator);
  NetworkedAnimator& operator=(NetworkedAnimator const& animator);

  StringList stateTypes() const;
  StringList states(String const& stateType) const;

  // Returns whether a state change occurred.  If startNew is true, always
  // forces a state change and starts the state off at the beginning even if
  // this state is already the current state.
  bool setState(String const& stateType, String const& state, bool startNew = false);
  String state(String const& stateType) const;

  StringList parts() const;

  // Queries, if it exists, a property value from the underlying
  // AnimatedPartSet for the given state or part.  If the property does not
  // exist, returns null.
  Json stateProperty(String const& stateType, String const& propertyName) const;
  Json partProperty(String const& partName, String const& propertyName) const;

  // Returns the transformation from flipping and zooming that is applied to
  // all parts in the NetworkedAnimator.
  Mat3F globalTransformation() const;
  // The transformation applied from the given set of transformation groups
  Mat3F groupTransformation(StringList const& transformationGroups) const;
  // The transformation that is applied to the given part NOT including the
  // global transformation
  Mat3F partTransformation(String const& partName) const;
  // Returns the total transformation for the given part, which includes the
  // globalTransformation, as well as the part rotation, scaling, and
  // translation.
  Mat3F finalPartTransformation(String const& partName) const;

  // partPoint / partPoly takes a propertyName and looks up the associated part
  // property and interprets is a Vec2F or a PolyF, then applies the final part
  // transformation and returns it.
  Maybe<Vec2F> partPoint(String const& partName, String const& propertyName) const;
  Maybe<PolyF> partPoly(String const& partName, String const& propertyName) const;

  // Every part image can have one or more <tag> directives in it, which if set
  // here will be replaced by the tag value when constructing Drawables.  All
  // Drawables can also have a <frame> tag which will be set to whatever the
  // current state frame is (1 indexed, so the first frame is 1).
  void setGlobalTag(String tagName, String tagValue);
  void setPartTag(String const& partType, String tagName, String tagValue);

  void setProcessingDirectives(Directives const& directives);
  void setZoom(float zoom);
  bool flipped() const;
  float flippedRelativeCenterLine() const;
  void setFlipped(bool flipped, float relativeCenterLine = 0.0f);

  // Animation rate defaults to 1.0, which means normal animation speed.  This
  // can be used to globally speed up or slow down all components of
  // NetworkedAnimator together.
  void setAnimationRate(float rate);

  // Given angle is an absolute angle.  Will rotate over time at the configured
  // angular velocity unless the immediate flag is set.
  bool hasRotationGroup(String const& rotationGroup) const;
  void rotateGroup(String const& rotationGroup, float targetAngle, bool immediate = false);
  float currentRotationAngle(String const& rotationGroup) const;

  // Transformation groups can be used for arbitrary part transforamtions.
  // They apply immediately, and are optionally interpolated on slaves.
  bool hasTransformationGroup(String const& transformationGroup) const;
  void translateTransformationGroup(String const& transformationGroup, Vec2F const& translation);
  void rotateTransformationGroup(String const& transformationGroup, float rotation, Vec2F const& rotationCenter = Vec2F());
  void scaleTransformationGroup(String const& transformationGroup, float scale, Vec2F const& scaleCenter = Vec2F());
  void scaleTransformationGroup(String const& transformationGroup, Vec2F const& scale, Vec2F const& scaleCenter = Vec2F());
  void transformTransformationGroup(String const& transformationGroup, float a, float b, float c, float d, float tx, float ty);
  void resetTransformationGroup(String const& transformationGroup);

  bool hasParticleEmitter(String const& emitterName) const;
  // Active particle emitters emit over time based on emission rate/variance.
  void setParticleEmitterActive(String const& emitterName, bool active);
  // Set the emission rate in particles / sec for a given emitter
  void setParticleEmitterEmissionRate(String const& emitterName, float emissionRate);
  // Set the optional particle emitter offset region, which particles will be
  // spread around randomly before being spawned
  void setParticleEmitterOffsetRegion(String const& emitterName, RectF const& offsetRegion);

  // Number of times to cycle when emitting a burst of particles.
  void setParticleEmitterBurstCount(String const& emitterName, unsigned burstCount);

  // Cause one time burst of all types of particles in an emitter looping around
  // burstCount times
  void burstParticleEmitter(String const& emitterName);

  bool hasLight(String const& lightName) const;
  void setLightActive(String const& lightName, bool active);
  void setLightPosition(String const& lightName, Vec2F position);
  void setLightColor(String const& lightName, Color color);
  void setLightPointAngle(String const& lightName, float angle);

  bool hasSound(String const& soundName) const;
  void setSoundPool(String const& soundName, StringList soundPool);
  // Plays a sound from the given independent sound pool.  Multiple sounds may
  // be played as part of this group, and playing a new one will not interrupt
  // an older one.
  void playSound(String const& soundName, int loops = 0);

  // Setting the sound position, volume, and speed will affect future sounds in
  // this group, as well as any still active sounds from this group.
  void setSoundPosition(String const& soundName, Vec2F const& position);

  void setSoundVolume(String const& soundName, float volume, float rampTime = 0.0f);
  void setSoundPitchMultiplier(String const& soundName, float pitchMultiplier, float rampTime = 0.0f);

  // Stop all sounds played from this sound group
  void stopAllSounds(String const& soundName, float rampTime = 0.0f);

  void setEffectEnabled(String const& effect, bool enabled);

  List<Drawable> drawables(Vec2F const& translate = Vec2F()) const;
  List<pair<Drawable, float>> drawablesWithZLevel(Vec2F const& translate = Vec2F()) const;

  List<LightSource> lightSources(Vec2F const& translate = Vec2F()) const;

  // Dynamic target is optional, if not given, generated particles and sounds
  // will be discarded
  void update(float dt, DynamicTarget* dynamicTarget);

  // Run through the current animations until the final frame, including any
  // transition animations.
  void finishAnimations();

private:
  struct RotationGroup {
    float angularVelocity;
    Vec2F rotationCenter;

    NetElementFloat targetAngle;
    float currentAngle;

    NetElementEvent netImmediateEvent;
  };

  struct TransformationGroup {
    Mat3F affineTransform() const;
    void setAffineTransform(Mat3F const& matrix);

    bool interpolated;

    NetElementFloat xTranslation;
    NetElementFloat yTranslation;
    NetElementFloat xScale;
    NetElementFloat yScale;
    NetElementFloat xShear;
    NetElementFloat yShear;
  };

  struct ParticleEmitter {
    struct ParticleConfig {
      ParticleVariantCreator creator;
      unsigned count;
      Vec2F offset;
      bool flip;
    };

    NetElementFloat emissionRate;
    float emissionRateVariance;
    NetElementData<RectF> offsetRegion;
    Maybe<String> anchorPart;
    StringList transformationGroups;
    Maybe<String> rotationGroup;
    Maybe<Vec2F> rotationCenter;

    List<ParticleConfig> particleList;

    NetElementBool active;
    NetElementUInt burstCount;
    NetElementUInt randomSelectCount;
    NetElementEvent burstEvent;

    float timer;
  };

  struct Light {
    NetElementBool active;
    NetElementFloat xPosition;
    NetElementFloat yPosition;
    NetElementData<Color> color;
    NetElementFloat pointAngle;
    Maybe<String> anchorPart;
    StringList transformationGroups;
    Maybe<String> rotationGroup;
    Maybe<Vec2F> rotationCenter;

    Maybe<PeriodicFunction<float>> flicker;
    bool pointLight;
    float pointBeam;
    float beamAmbience;
  };

  enum class SoundSignal {
    Play,
    StopAll
  };

  struct Sound {
    float rangeMultiplier;
    NetElementData<StringList> soundPool;
    NetElementFloat xPosition;
    NetElementFloat yPosition;
    NetElementFloat volumeTarget;
    NetElementFloat volumeRampTime;
    NetElementFloat pitchMultiplierTarget;
    NetElementFloat pitchMultiplierRampTime;
    NetElementInt loops;
    NetElementSignal<SoundSignal> signals;
  };

  struct Effect {
    String type;
    float time;
    Directives directives;

    NetElementBool enabled;
    float timer;
  };

  struct StateInfo {
    NetElementSize stateIndex;
    NetElementEvent startedEvent;
  };

  void setupNetStates();

  void netElementsNeedLoad(bool full) override;
  void netElementsNeedStore() override;

  String m_relativePath;

  AnimatedPartSet m_animatedParts;
  OrderedHashMap<String, StateInfo> m_stateInfo;
  OrderedHashMap<String, RotationGroup> m_rotationGroups;
  OrderedHashMap<String, TransformationGroup> m_transformationGroups;
  OrderedHashMap<String, ParticleEmitter> m_particleEmitters;
  OrderedHashMap<String, Light> m_lights;
  OrderedHashMap<String, Sound> m_sounds;
  OrderedHashMap<String, Effect> m_effects;

  NetElementData<Directives> m_processingDirectives;
  NetElementFloat m_zoom;

  NetElementBool m_flipped;
  NetElementFloat m_flippedRelativeCenterLine;

  NetElementFloat m_animationRate;

  NetElementHashMap<String, String> m_globalTags;
  StableStringMap<NetElementHashMap<String, String>> m_partTags;
  mutable StringMap<std::pair<size_t, Drawable>> m_cachedPartDrawables;
};

}

#endif
