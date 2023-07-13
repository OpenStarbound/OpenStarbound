#ifndef STAR_VOICE_HPP
#define STAR_VOICE_HPP
#include "StarJson.hpp"
#include "StarBiMap.hpp"
#include "StarException.hpp"
#include "StarGameTypes.hpp"
#include "StarMaybe.hpp"
#include "StarThread.hpp"
#include "StarApplicationController.hpp"

struct OpusDecoder;
typedef std::unique_ptr<OpusDecoder, void(*)(OpusDecoder*)> OpusDecoderPtr;
struct OpusEncoder;
typedef std::unique_ptr<OpusEncoder, void(*)(OpusEncoder*)> OpusEncoderPtr;

namespace Star {

STAR_EXCEPTION(VoiceException, StarException);

enum class VoiceInputMode : uint8_t { VoiceActivity, PushToTalk };
extern EnumMap<VoiceInputMode> const VoiceInputModeNames;

enum class VoiceChannelMode: uint8_t { Mono = 1, Stereo = 2 };
extern EnumMap<VoiceChannelMode> const VoiceChannelModeNames;

STAR_CLASS(Voice);
STAR_CLASS(VoiceAudioStream);
STAR_CLASS(ApplicationController);

class Voice {
public:
  // Individual speakers are represented by their connection ID.
  typedef ConnectionId SpeakerId;

  class Speaker {
  public:
    SpeakerId speakerId = 0;
    EntityId entityId = 0;

    Vec2F position = Vec2F();
    String name = "Unnamed";

    OpusDecoderPtr decoderMono;
    OpusDecoderPtr decoderStereo;
    VoiceAudioStreamPtr audioStream;
    Mutex mutex;

    Speaker(SpeakerId speakerId);
  };

  typedef std::shared_ptr<Speaker> SpeakerPtr;

  // Get pointer to the singleton Voice instance, if it exists.  Otherwise,
  // returns nullptr.
  static Voice* singletonPtr();

  // Gets reference to Voice singleton, throws VoiceException if root
  // is not initialized.
  static Voice& singleton();

  Voice(ApplicationControllerPtr appController);
  ~Voice();

  Voice(Voice const&) = delete;
  Voice& operator=(Voice const&) = delete;

  void init();

  void loadJson(Json const& config);
  Json saveJson() const;

  void save() const;
  void scheduleSave();

  // Sets the local speaker ID and returns the local speaker. Must be called upon loading into a world.
  SpeakerPtr setLocalSpeaker(SpeakerId speakerId);
  SpeakerPtr speaker(SpeakerId speakerId);

  // Called when receiving input audio data from SDL, on its own thread.
  void getAudioData(uint8_t* stream, int len);

  // Called to mix voice audio with the game.
  void mix(int16_t* buffer, size_t frames, unsigned channels);

  typedef function<float(unsigned, Vec2F, float)> PositionalAttenuationFunction;
  void update(PositionalAttenuationFunction positionalAttenuationFunction = {});

  void setDeviceName(Maybe<String> device);

  inline int encoderChannels() const {
    return m_channelMode == VoiceChannelMode::Mono ? 1 : 2;
  }
private:
  static Voice* s_singleton;

  static OpusDecoder* createDecoder(int channels);
  static OpusEncoder* createEncoder(int channels);
  void resetEncoder();
  void openDevice();
  void closeDevice();

  SpeakerId m_speakerId = 0;
  SpeakerPtr m_clientSpeaker;
  HashMap<SpeakerId, SpeakerPtr> m_speakers;

  HashSet<SpeakerPtr> m_activeSpeakers;

  OpusEncoderPtr m_encoder;

  float m_outputVolume = 1.0f;
  float m_inputVolume = 1.0f;
  float m_threshold = -50.0f;

  bool m_enabled = true;
  bool m_inputEnabled = true;

  bool m_deviceOpen = false;
  Maybe<String> m_deviceName;
  VoiceInputMode m_inputMode;
  VoiceChannelMode m_channelMode;

  ApplicationControllerPtr m_applicationController;

  double nextSaveTime = 0.0f;
};
  
}

#endif