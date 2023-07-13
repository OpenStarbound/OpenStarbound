#ifndef STAR_VOICE_HPP
#define STAR_VOICE_HPP
#include "StarJson.hpp"
#include "StarBiMap.hpp"
#include "StarGameTypes.hpp"
#include "StarException.hpp"

struct OpusDecoder;
typedef std::unique_ptr<OpusDecoder, void(*)(OpusDecoder*)> OpusDecoderPtr;
struct OpusEncoder;
typedef std::unique_ptr<OpusEncoder, void(*)(OpusEncoder*)> OpusEncoderPtr;

namespace Star {

STAR_EXCEPTION(VoiceException, StarException);

enum class VoiceTriggerMode : uint8_t { VoiceActivity, PushToTalk };
extern EnumMap<VoiceTriggerMode> const VoiceTriggerModeNames;

enum class VoiceChannelMode: uint8_t { Mono = 1, Stereo = 2 };
extern EnumMap<VoiceChannelMode> const VoiceChannelModeNames;

STAR_CLASS(Voice);

class Voice {
public:
  // Individual speakers are represented by their connection ID.
  typedef ConnectionId SpeakerId;

  struct Speaker {
    SpeakerId speakerId = 0;
    EntityId entityId = 0;
    Vec2F position = Vec2F();
    String name = "Unnamed";

    OpusDecoderPtr decoderMono;
    OpusDecoderPtr decoderStereo;

    atomic<bool> active = false;
    atomic<float> currentLoudness = 0.0f;
    atomic<Array<float, 2>> channelVolumes = Array<float, 2>::filled(1.0f);

    Speaker(SpeakerId speakerId);
  };

  typedef std::shared_ptr<Speaker> SpeakerPtr;

  // Get pointer to the singleton Voice instance, if it exists.  Otherwise,
  // returns nullptr.
  static Voice* singletonPtr();

  // Gets reference to Voice singleton, throws VoiceException if root
  // is not initialized.
  static Voice& singleton();

  Voice();
  ~Voice();

  Voice(Voice const&) = delete;
  Voice& operator=(Voice const&) = delete;

  void load(Json const& config);
  Json save() const;
  
  // Sets the local speaker ID and returns the local speaker. Must be called upon loading into a world.
  SpeakerPtr setLocalSpeaker(SpeakerId speakerId);
  SpeakerPtr speaker(SpeakerId speakerId);

  // Called to mix voice audio with the game.
  void mix(int16_t* buffer, size_t frames, unsigned channels);

  typedef function<float(unsigned, Vec2F, float)> PositionalAttenuationFunction;
  void update(PositionalAttenuationFunction positionalAttenuationFunction = {});

  inline int encoderChannels() const {
    return m_channelMode == VoiceChannelMode::Mono ? 1 : 2;
  }
private:
  static Voice* s_singleton;

  static OpusDecoder* createDecoder(int channels);
  static OpusEncoder* createEncoder(int channels);
  void resetEncoder();

  SpeakerId m_speakerId = 0;
  SpeakerPtr m_clientSpeaker;
  HashMap<SpeakerId, SpeakerPtr> m_speakers;

  HashSet<SpeakerPtr> m_activeSpeakers;

  OpusEncoderPtr m_encoder;

  VoiceTriggerMode m_triggerMode;
  VoiceChannelMode m_channelMode;
};
  
}

#endif