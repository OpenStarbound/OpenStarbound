#ifndef STAR_VOICE_HPP
#define STAR_VOICE_HPP
#include "StarJson.hpp"
#include "StarBiMap.hpp"
#include "StarException.hpp"
#include "StarGameTypes.hpp"
#include "StarMaybe.hpp"
#include "StarThread.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarApplicationController.hpp"

#include <queue>

struct OpusDecoder;
typedef std::unique_ptr<OpusDecoder, void(*)(OpusDecoder*)> OpusDecoderPtr;
struct OpusEncoder;
typedef std::unique_ptr<OpusEncoder, void(*)(OpusEncoder*)> OpusEncoderPtr;

namespace Star {

String const VoiceBroadcastPrefix = "Voice\0"s;

STAR_EXCEPTION(VoiceException, StarException);

enum class VoiceInputMode : uint8_t { VoiceActivity, PushToTalk };
extern EnumMap<VoiceInputMode> const VoiceInputModeNames;

enum class VoiceChannelMode: uint8_t { Mono = 1, Stereo = 2 };
extern EnumMap<VoiceChannelMode> const VoiceChannelModeNames;

STAR_CLASS(Voice);
STAR_CLASS(VoiceAudioStream);
STAR_CLASS(ApplicationController);

struct VoiceAudioChunk {
  std::unique_ptr<int16_t[]> data;
  size_t remaining;
  size_t offset = 0;

  VoiceAudioChunk(int16_t* ptr, size_t size) {
    data.reset(ptr);
    remaining = size;
    offset = 0;
  }

  inline size_t takeSamples(std::vector<int16_t>& out, size_t count) {
    size_t toRead = min<size_t>(count, remaining);
    int16_t* start = data.get() + offset;
    out.insert(out.end(), start, start + toRead);
    offset += toRead;
    remaining -= toRead;
    return toRead;
  }

  //this one's unsafe
  inline int16_t takeSample() {
    --remaining;
    return *(data.get() + offset++);
  }

  inline bool exhausted() { return remaining == 0; }
};


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

    int64_t lastReceiveTime = 0;
    int64_t lastPlayTime = 0;
    float smoothDb = -96.0f;
    Array<float, 10> dbHistory = Array<float, 10>::filled(0);

    atomic<bool> muted = false;
    atomic<bool> playing = 0;
    atomic<float> decibelLevel = -96.0f;
    atomic<float> volume = 1.0f;
    atomic<Array<float, 2>> channelVolumes = Array<float, 2>::filled(1);

    unsigned int minimumPlaySamples = 4096;

    Speaker(SpeakerId speakerId);
    Json toJson() const;
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

  void loadJson(Json const& config, bool skipSave = false);
  Json saveJson() const;

  void save() const;
  void scheduleSave();

  // Sets the local speaker ID and returns the local speaker. Must be called upon loading into a world.
  SpeakerPtr setLocalSpeaker(SpeakerId speakerId);
  SpeakerPtr localSpeaker();
  SpeakerPtr speaker(SpeakerId speakerId);
  HashMap<SpeakerId, SpeakerPtr>& speakers();
  List<Voice::SpeakerPtr> sortedSpeakers(bool onlyPlaying);
  void clearSpeakers();

  // Called when receiving input audio data from SDL, on its own thread.
  void readAudioData(uint8_t* stream, int len);

  // Called to mix voice audio with the game.
  void mix(int16_t* buffer, size_t frames, unsigned channels);

  typedef function<float(unsigned, Vec2F, float)> PositionalAttenuationFunction;
  void update(PositionalAttenuationFunction positionalAttenuationFunction = {});

  void setDeviceName(Maybe<String> device);
  StringList availableDevices();

  int send(DataStreamBuffer& out, size_t budget = 0);
  bool receive(SpeakerPtr speaker, std::string_view view);

  // Must be called every frame with input state, expires after 1s.
  void setInput(bool input = true);

  inline int encoderChannels() const { return (int)m_channelMode; }

  static OpusDecoder* createDecoder(int channels);
  static OpusEncoder* createEncoder(int channels);
private:
  static Voice* s_singleton;
  void resetEncoder();
  void resetDevice();
  void openDevice();
  void closeDevice();
  inline bool shouldEnableInput() const { return m_enabled && m_inputEnabled; }

  bool playSpeaker(SpeakerPtr const& speaker, int channels);

  void thread();

  SpeakerId m_speakerId = 0;
  SpeakerPtr m_clientSpeaker;
  HashMap<SpeakerId, SpeakerPtr> m_speakers;

  Mutex m_activeSpeakersMutex;
  HashSet<SpeakerPtr> m_activeSpeakers;



  OpusEncoderPtr m_encoder;

  float m_outputVolume = 1.0f;
  float m_inputVolume = 1.0f;
  float m_threshold = -50.0f;
  
  int64_t m_lastSentTime = 0;
  int64_t m_lastInputTime = 0;
  int64_t m_lastThresholdTime = 0;
  int64_t m_nextSaveTime = 0;
  bool m_enabled = true;
  bool m_inputEnabled = false;
  bool m_loopback = false;

  int m_deviceChannels = 1;
  bool m_deviceOpen = false;
  Maybe<String> m_deviceName;
  VoiceInputMode m_inputMode;
  VoiceChannelMode m_channelMode;

  ThreadFunction<void> m_thread;
  Mutex m_threadMutex;
  ConditionVariable m_threadCond;
  atomic<bool> m_stopThread;

  std::vector<int16_t> m_decodeBuffer;
  std::vector<int16_t> m_resampleBuffer;

  ApplicationControllerPtr m_applicationController;

  struct EncodedChunk {
    std::unique_ptr<unsigned char[]> data;
    size_t size;

    EncodedChunk(unsigned char* _data, size_t len) {
      data.reset(_data);
      size = len;
    }
  };

  Mutex m_encodeMutex;
  std::vector<ByteArray> m_encodedChunks;
  size_t m_encodedChunksLength = 0;

  Mutex m_captureMutex;
  std::queue<VoiceAudioChunk> m_capturedChunks;
  size_t m_capturedChunksFrames = 0;
};
  
}

#endif