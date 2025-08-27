#include "StarVoice.hpp"
#include "StarFormat.hpp"
#include "StarJsonExtra.hpp"
#include "StarApplicationController.hpp"
#include "StarTime.hpp"
#include "StarRoot.hpp"
#include "StarLogging.hpp"
#include "StarInterpolation.hpp"
#include "StarAudio.hpp"
#include "opus/opus.h"

#include "SDL3/SDL.h"

constexpr int VOICE_SAMPLE_RATE = 48000;
constexpr int VOICE_FRAME_SIZE = 960;

constexpr int VOICE_MAX_PACKET_SIZE = 3 * 1276;

constexpr uint16_t VOICE_VERSION = 1;

namespace Star {

EnumMap<VoiceInputMode> const VoiceInputModeNames{
  {VoiceInputMode::VoiceActivity, "VoiceActivity"},
  {VoiceInputMode::PushToTalk, "PushToTalk"}
};

EnumMap<VoiceChannelMode> const VoiceChannelModeNames{
  {VoiceChannelMode::Mono, "Mono"},
  {VoiceChannelMode::Stereo, "Stereo"}
};

inline float getAudioChunkLoudness(int16_t* data, size_t samples, float volume) {
  if (!samples)
    return 0.f;

  double rms = 0.;
  for (size_t i = 0; i != samples; ++i) {
    float sample = ((float)data[i] / 32767.f) * volume;
    rms += (double)(sample * sample);
  }

  float fRms = sqrtf((float)(rms / samples));

  if (fRms > 0)
    return std::clamp<float>(20.f * log10f(fRms), -127.f, 0.f);
  else
    return -127.f;
}

float getAudioLoudness(int16_t* data, size_t samples, float volume = 1.0f) {
  constexpr size_t CHUNK_SIZE = 50;

  float highest = -127.f;
  for (size_t i = 0; i < samples; i += CHUNK_SIZE) {
    float level = getAudioChunkLoudness(data + i, std::min<size_t>(i + CHUNK_SIZE, samples) - i, volume);
    if (level > highest)
      highest = level;
  }

  return highest;
}

class VoiceAudioStream {
public:
  // TODO: This should really be a ring buffer instead.
  std::queue<int16_t> samples;
  SDL_AudioStream* sdlAudioStreamMono;
  SDL_AudioStream* sdlAudioStreamStereo;
  Mutex mutex;

  VoiceAudioStream() {
    SDL_AudioSpec srcSpec = {SDL_AUDIO_S16LE, 1, 48000};
    SDL_AudioSpec dstSpec = {SDL_AUDIO_S16,   1, 44100};
    sdlAudioStreamMono = SDL_CreateAudioStream(&srcSpec, &dstSpec);
    srcSpec.channels = dstSpec.channels = 2;
    sdlAudioStreamStereo = SDL_CreateAudioStream(&srcSpec, &dstSpec);
  }
  ~VoiceAudioStream() {
    SDL_DestroyAudioStream(sdlAudioStreamMono);
    SDL_DestroyAudioStream(sdlAudioStreamStereo);
  }

  inline int16_t take() {
    int16_t sample = 0;
    if (!samples.empty()) {
      sample = samples.front();
      samples.pop();
    }
    return sample;
  }

  size_t resample(int16_t* in, size_t inSamples, std::vector<int16_t>& out, bool mono) {
    SDL_AudioStream* stream = mono ? sdlAudioStreamMono : sdlAudioStreamStereo;
    SDL_PutAudioStreamData(stream, in, inSamples * sizeof(int16_t));
    if (int available = SDL_GetAudioStreamAvailable(stream)) {
      out.resize(available / 2);
      SDL_GetAudioStreamData(stream, out.data(), available);
      return available;
    }
    return 0;
  }
};

Voice::Speaker::Speaker(SpeakerId id)
  : decoderMono  (createDecoder(1), opus_decoder_destroy)
  , decoderStereo(createDecoder(2), opus_decoder_destroy) {
  speakerId = id;
  audioStream = make_shared<VoiceAudioStream>();
}

Json Voice::Speaker::toJson() const {
  return JsonObject{
    {"speakerId",      speakerId},
    {"entityId",       entityId },
    {"name",           name     },
    {"playing",        (bool)playing},
    {"muted",          (bool)muted  },
    {"decibels",       (float)decibelLevel},
    {"smoothDecibels", (float)smoothDb    },
    {"position",       jsonFromVec2F(position)}
  };
}

Voice* Voice::s_singleton;

Voice* Voice::singletonPtr() {
  return s_singleton;
}

Voice& Voice::singleton() {
  if (!s_singleton)
    throw VoiceException("Voice::singleton() called with no Voice instance available");
  else
    return *s_singleton;
}

Voice::Voice(ApplicationControllerPtr appController) : m_encoder(nullptr, opus_encoder_destroy) {
  if (s_singleton)
    throw VoiceException("Singleton Voice has been constructed twice");

  m_clientSpeaker = make_shared<Speaker>(m_speakerId);
  m_inputMode = VoiceInputMode::PushToTalk;
  m_channelMode = VoiceChannelMode::Mono;
  m_applicationController = appController;

  m_stopThread = false;
  m_thread = Thread::invoke("Voice::thread", mem_fn(&Voice::thread), this);

  s_singleton = this;
}

Voice::~Voice() {
  m_stopThread = true;

  {
    MutexLocker locker(m_threadMutex);
    m_threadCond.broadcast();
  }

  m_thread.finish();

  if (m_nextSaveTime)
    save();

  closeDevice();

  s_singleton = nullptr;
}

void Voice::init() {
  resetEncoder();
  if (shouldEnableInput())
    openDevice();
}


template <typename T>
inline bool change(T& value, T newValue, bool& out) {
  bool changed = value != newValue;
  out |= changed;
  value = std::move(newValue);
  return changed;
}

void Voice::loadJson(Json const& config, bool skipSave) {
  // Not all keys are required

  bool changed = false;
  bool shouldResetDevice = false;
  {
    bool enabled = shouldEnableInput();
    m_enabled      = config.getBool("enabled", m_enabled);
    m_inputEnabled = config.getBool("inputEnabled", m_inputEnabled);
    if (shouldEnableInput() != enabled)
      shouldResetDevice = changed = true;
  }

  if (config.contains("deviceName") // Make sure null-type key exists
  && change(m_deviceName, config.optString("deviceName"), changed))
    shouldResetDevice = true;

  m_threshold = config.getFloat("threshold", m_threshold);
  m_inputAmplitude = perceptualToAmplitude(
    m_inputVolume = config.getFloat("inputVolume", m_inputVolume));
  m_outputAmplitude = perceptualToAmplitude(
    m_outputVolume = config.getFloat("outputVolume", m_outputVolume));
  
  if (change(m_loopback, config.getBool("loopback", m_loopback), changed))
    m_clientSpeaker->playing = false;

  if (auto inputMode = config.optString("inputMode")) {
    if (change(m_inputMode, VoiceInputModeNames.getLeft(*inputMode), changed))
      m_lastInputTime = 0;
  }

  bool shouldResetEncoder = false;
  if (auto channelMode = config.optString("channelMode")) {
    if (change(m_channelMode, VoiceChannelModeNames.getLeft(*channelMode), changed)) {
      closeDevice();
      shouldResetEncoder = true;
      shouldResetDevice = true;
    }
  }

  // not saving this setting to disk, as it's just for audiophiles
  // don't want someone fudging their bitrate from the intended defaults and forgetting
  if (auto bitrate = config.opt("bitrate")) {
    unsigned newBitrate = bitrate->canConvert(Json::Type::Int)
      ? clamp((unsigned)bitrate->toUInt(), 6000u, 510000u) : 0;
    shouldResetEncoder |= change(m_bitrate, newBitrate, changed);
  }

  if (shouldResetEncoder)
    resetEncoder();

  if (shouldResetDevice)
    resetDevice();

  if (changed && !skipSave)
    scheduleSave();
}



Json Voice::saveJson() const {
  return JsonObject{
    {"enabled",      m_enabled},
    {"deviceName",  m_deviceName ? *m_deviceName : Json()},
    {"inputEnabled", m_inputEnabled},
    {"threshold",    m_threshold},
    {"inputVolume",  m_inputVolume},
    {"outputVolume", m_outputVolume},
    {"inputMode",    VoiceInputModeNames.getRight(m_inputMode)},
    {"channelMode",  VoiceChannelModeNames.getRight(m_channelMode)},
    {"loopback",     m_loopback},
    {"version",      1}
  };
}

void Voice::save() const {
  if (Root* root = Root::singletonPtr()) {
    if (auto config = root->configuration())
      config->set("voice", saveJson());
  }
}

void Voice::scheduleSave() {
  if (!m_nextSaveTime)
    m_nextSaveTime = Time::monotonicMilliseconds() + 2000;
}

Voice::SpeakerPtr Voice::setLocalSpeaker(SpeakerId speakerId) {
  if (m_speakers.contains(m_speakerId))
    m_speakers.remove(m_speakerId);

  m_clientSpeaker->speakerId = m_speakerId = speakerId;
  return m_speakers.insert(m_speakerId, m_clientSpeaker).first->second;
}

Voice::SpeakerPtr Voice::localSpeaker() {
  return m_clientSpeaker;
}

Voice::SpeakerPtr Voice::speaker(SpeakerId speakerId) {
  if (m_speakerId == speakerId)
    return m_clientSpeaker;
  else {
    if (SpeakerPtr const* ptr = m_speakers.ptr(speakerId))
      return *ptr;
    else
      return m_speakers.emplace(speakerId, make_shared<Speaker>(speakerId)).first->second;
  }
}

HashMap<Voice::SpeakerId, Voice::SpeakerPtr>& Voice::speakers() {
  return m_speakers;
}

List<Voice::SpeakerPtr> Voice::sortedSpeakers(bool onlyPlaying) {
  List<SpeakerPtr> result;

  auto sorter = [](SpeakerPtr const& a, SpeakerPtr const& b) -> bool {
    if (a->lastPlayTime != b->lastPlayTime)
      return a->lastPlayTime < b->lastPlayTime;
    else
      return a->speakerId < b->speakerId;
  };

  for (auto& p : m_speakers) {
    if (!onlyPlaying || p.second->playing)
      result.insertSorted(p.second, sorter);
  }

  return result;
}

void Voice::clearSpeakers() {
  auto it = m_speakers.begin();
  while (it != m_speakers.end()) {
    if (it->second == m_clientSpeaker)
      it = ++it;
    else
      it = m_speakers.erase(it);
  }
}

void Voice::readAudioData(uint8_t* stream, int len) {
  auto now = Time::monotonicMilliseconds();
  bool active = m_encoder && m_encodedChunksLength < 2048
       && (m_inputMode == VoiceInputMode::VoiceActivity || now < m_lastInputTime);

  size_t sampleCount = len / 2;

  if (active) {
    float decibels = getAudioLoudness((int16_t*)stream, sampleCount);

    if (m_inputMode == VoiceInputMode::VoiceActivity) {
      if (decibels > m_threshold)
        m_lastThresholdTime = now;
      active = now - m_lastThresholdTime < 50;
    }
  }

  m_clientSpeaker->decibelLevel = getAudioLoudness((int16_t*)stream, sampleCount, m_inputAmplitude);

  if (!m_loopback) {
    if (active && !m_clientSpeaker->playing)
      m_clientSpeaker->lastPlayTime = now;

    m_clientSpeaker->playing = active;
  }

  MutexLocker captureLock(m_captureMutex);
  if (active) {
    m_capturedChunksFrames += sampleCount / m_deviceChannels;
    auto data = (opus_int16*)malloc(len);
    memcpy(data, stream, len);
    m_capturedChunks.emplace(data, sampleCount); // takes ownership
    m_threadCond.signal();
  }
  else { // Clear out any residual data so they don't manifest at the start of the next encode, whenever that is
    while (!m_capturedChunks.empty())
      m_capturedChunks.pop();

    m_capturedChunksFrames = 0;
  }
}

void Voice::mix(int16_t* buffer, size_t frameCount, unsigned channels) {
  size_t samples = frameCount * channels;
  static std::vector<int16_t> finalBuffer, speakerBuffer;
  static std::vector<int32_t> sharedBuffer; //int32 to reduce clipping
  speakerBuffer.resize(samples);
  sharedBuffer.resize(samples);

  bool mix = false;
  {
    MutexLocker lock(m_activeSpeakersMutex);
    auto it = m_activeSpeakers.begin();
    while (it != m_activeSpeakers.end()) {
      SpeakerPtr const& speaker = *it;
      VoiceAudioStream* audio = speaker->audioStream.get();
      MutexLocker audioLock(audio->mutex);
      if (speaker->playing && !audio->samples.empty()) {
        for (size_t i = 0; i != samples; ++i)
          speakerBuffer[i] = audio->take();

        if (speaker != m_clientSpeaker)
          speaker->decibelLevel = getAudioLoudness(speakerBuffer.data(), samples);

        if (!speaker->muted) {
          mix = true;

          float volume = speaker->volume;
          std::array<float, 2> levels;
          {
            MutexLocker locker(speaker->mutex);
            levels = speaker->channelVolumes;
          }
          for (size_t i = 0; i != samples; ++i)
            sharedBuffer[i] += (int32_t)(speakerBuffer[i]) * levels[i % 2] * volume;
          //Blends the weaker channel into the stronger one,
          /* unused, is a bit too strong on stereo music.
          float maxLevel = max(levels[0], levels[1]);
          float leftToRight = maxLevel != 0.0f ? 1.0f - (levels[0] / maxLevel) : 0.0f;
          float rightToLeft = maxLevel != 0.0f ? 1.0f - (levels[1] / maxLevel) : 0.0f;

          int16_t* speakerData = speakerBuffer.data();
          int32_t* sharedData  = sharedBuffer.data();
          for (size_t i = 0; i != frameCount; ++i) {
            auto leftSample  = (float)*speakerData++;
            auto rightSample = (float)*speakerData++;
            
            if (rightToLeft != 0.0f)
              leftSample  = ( leftSample + rightSample * rightToLeft) / (1.0f + rightToLeft);
            if (leftToRight != 0.0f)
              rightSample = (rightSample +  leftSample * leftToRight) / (1.0f + leftToRight);

            *sharedData++ += (int32_t)leftSample  * levels[0];
            *sharedData++ += (int32_t)rightSample * levels[1];
          }
          //*/
        }
        ++it;
      }
      else {
        speaker->playing = false;
        if (speaker != m_clientSpeaker)
          speaker->decibelLevel = -96.0f;
        it = m_activeSpeakers.erase(it);
      }
    }
  }

  if (mix) {
    finalBuffer.resize(sharedBuffer.size(), 0);

    float vol = m_outputAmplitude;
    for (size_t i = 0; i != sharedBuffer.size(); ++i)
      finalBuffer[i] = (int16_t)clamp<int>(sharedBuffer[i] * vol, INT16_MIN, INT16_MAX);

    SDL_MixAudio((Uint8*)buffer, (Uint8*)finalBuffer.data(), SDL_AUDIO_S16LE, finalBuffer.size() * sizeof(int16_t), 1.0f);
    memset(sharedBuffer.data(), 0, sharedBuffer.size() * sizeof(int32_t));
  }
}

void Voice::update(float, PositionalAttenuationFunction positionalAttenuationFunction) {
  for (auto& entry : m_speakers) {
    if (SpeakerPtr& speaker = entry.second) {
      Vec2F newChannelVolumes = positionalAttenuationFunction ? Vec2F{
          1.0f - positionalAttenuationFunction(0, speaker->position, 1.0f),
          1.0f - positionalAttenuationFunction(1, speaker->position, 1.0f)
      } : Vec2F{1.0f, 1.0f};
      {
        MutexLocker locker(speaker->mutex);
        speaker->channelVolumes = newChannelVolumes;
      }
      auto& dbHistory = speaker->dbHistory;
      memmove(&dbHistory[1], &dbHistory[0], (dbHistory.size() - 1) * sizeof(float));
      dbHistory[0] = speaker->decibelLevel;
      float smoothDb = 0.0f;
      for (float dB : dbHistory)
        smoothDb += dB;

      speaker->smoothDb = smoothDb / dbHistory.size();
    }
  }

  if (m_nextSaveTime && Time::monotonicMilliseconds() > m_nextSaveTime) {
    m_nextSaveTime = 0;
    save();
  }
}


void Voice::setDeviceName(Maybe<String> deviceName) {
  if (m_deviceName == deviceName)
    return;

  m_deviceName = deviceName;
  if (m_deviceOpen)
    openDevice();
}

StringList Voice::availableDevices() {
  StringList list;
  int i, num_devices;
  if (SDL_AudioDeviceID* devices = SDL_GetAudioRecordingDevices(&num_devices)) {
    list.reserve(num_devices);
    for (i = 0; i < num_devices; ++i)
      list.emplace_back(SDL_GetAudioDeviceName(devices[i]));
    SDL_free(devices);
  }
  list.sort();
  return list;
}

int Voice::send(DataStreamBuffer& out, size_t budget) {
  out.setByteOrder(ByteOrder::LittleEndian);
  out.write<uint16_t>(VOICE_VERSION);

  MutexLocker encodeLock(m_encodeMutex);

  if (m_encodedChunks.empty())
    return 0;

  std::vector<ByteArray> encodedChunks = std::move(m_encodedChunks);
  m_encodedChunksLength = 0;

  encodeLock.unlock();

  for (auto& chunk : encodedChunks) {
    out.write<uint32_t>(chunk.size());
    out.writeBytes(chunk);
    if (budget && (budget -= min<size_t>(budget, chunk.size())) == 0)
      break;
  }

  m_lastSentTime = Time::monotonicMilliseconds();
  if (m_loopback)
    receive(m_clientSpeaker, { out.ptr(), out.size() });
  return 1;
}

bool Voice::receive(SpeakerPtr speaker, std::string_view view) {
  if (!m_enabled || !speaker || view.empty())
    return false;

  try {
    DataStreamExternalBuffer reader(view.data(), view.size());
    reader.setByteOrder(ByteOrder::LittleEndian);

    if (reader.read<uint16_t>() > VOICE_VERSION)
      return false;

    uint32_t opusLength = 0;
    while (!reader.atEnd()) {
      reader >> opusLength;
      if (reader.pos() + opusLength > reader.size())
        throw VoiceException("Opus packet length goes past end of buffer"s, false);
      auto opusData = (unsigned char*)reader.ptr() + reader.pos();
      reader.seek(opusLength, IOSeek::Relative);

      int channels = opus_packet_get_nb_channels(opusData);
      if (channels == OPUS_INVALID_PACKET)
        continue;

      bool mono = channels == 1;
      OpusDecoder* decoder = mono ? speaker->decoderMono.get() : speaker->decoderStereo.get();
      int samples = opus_decoder_get_nb_samples(decoder, opusData, opusLength);
      if (samples < 0)
        throw VoiceException(strf("Decoder error: {}", opus_strerror(samples)), false);

      m_decodeBuffer.resize(samples * (size_t)channels);

      int decodedSamples = opus_decode(decoder, opusData, opusLength, m_decodeBuffer.data(), m_decodeBuffer.size() * sizeof(int16_t), 0);
      if (decodedSamples <= 0) {
        if (decodedSamples < 0)
          throw VoiceException(strf("Decoder error: {}", opus_strerror(samples)), false);
        return true;
      }

      //Logger::info("Voice: decoded Opus chunk {} bytes -> {} samples", opusLength, decodedSamples * channels);

      speaker->audioStream->resample(m_decodeBuffer.data(), (size_t)decodedSamples * channels, m_resampleBuffer, mono);

      {
        MutexLocker lock(speaker->audioStream->mutex);
        auto& samples = speaker->audioStream->samples;

        auto now = Time::monotonicMilliseconds();
        if (now - speaker->lastReceiveTime < 1000) {
          auto limit = (size_t)speaker->minimumPlaySamples + 22050;
          if (samples.size() > limit) { // skip ahead if we're getting too far
            for (size_t i = samples.size(); i >= limit; --i)
              samples.pop();
          }
        }
        else
          samples = std::queue<int16_t>();

        speaker->lastReceiveTime = now;

        if (mono) {
          for (int16_t sample : m_resampleBuffer) {
            samples.push(sample);
            samples.push(sample);
          }
        }
        else {
          for (int16_t sample : m_resampleBuffer)
            samples.push(sample);
        }
      }
      playSpeaker(speaker, channels);
    }
    return true;
  }
  catch (StarException const& e) {
    Logger::error("Voice: Error receiving voice data for speaker #{} ('{}'): {}", speaker->speakerId, speaker->name, e.what());
    return false;
  }
}

void Voice::setInput(bool input) {
  m_lastInputTime = (m_deviceOpen && input) ? Time::monotonicMilliseconds() + 1000 : 0;
}

OpusDecoder* Voice::createDecoder(int channels) {
  int error;
  OpusDecoder* decoder = opus_decoder_create(VOICE_SAMPLE_RATE, channels, &error);
  if (error != OPUS_OK)
    throw VoiceException::format("Could not create decoder: {}", opus_strerror(error));
  else
    return decoder;
}

OpusEncoder* Voice::createEncoder(int channels) {
  int error;
  OpusEncoder* encoder = opus_encoder_create(VOICE_SAMPLE_RATE, channels, OPUS_APPLICATION_AUDIO, &error);
  if (error != OPUS_OK)
    throw VoiceException::format("Could not create encoder: {}", opus_strerror(error));
  else
    return encoder;
}

void Voice::resetEncoder() {
  int channels = encoderChannels();
  MutexLocker locker(m_threadMutex);
  m_encoder.reset(createEncoder(channels));
  int bitrate = m_bitrate > 0 ? (int)m_bitrate : (channels == 2 ? 50000 : 24000);
  opus_encoder_ctl(m_encoder.get(), OPUS_SET_BITRATE(bitrate));
}

void Voice::resetDevice() {
  closeDevice();
  if (shouldEnableInput())
    openDevice();
}

void Voice::openDevice() {
  if (m_deviceOpen)
    return;
  closeDevice();

  uint32_t deviceId = SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
  if (m_deviceName) {
    int i, num_devices;
    if (SDL_AudioDeviceID* devices = SDL_GetAudioRecordingDevices(&num_devices)) {
      for (i = 0; i < num_devices; ++i) {
        if (*m_deviceName == SDL_GetAudioDeviceName(devices[i])) {
          deviceId = devices[i];
          break;
        }
      }
      SDL_free(devices);
    }
  }
  m_applicationController->openAudioInputDevice(
    deviceId,
    VOICE_SAMPLE_RATE,
    m_deviceChannels = encoderChannels(),
    [this](uint8_t* stream, int len) {
      readAudioData(stream, len);
    }
  );

  m_deviceOpen = true;
}

void Voice::closeDevice() {
  if (!m_deviceOpen)
    return;

  m_applicationController->closeAudioInputDevice();
  m_clientSpeaker->playing = false;
  m_clientSpeaker->decibelLevel = -96.0f;
  m_deviceOpen = false;
}

bool Voice::playSpeaker(SpeakerPtr const& speaker, int) {
  if (speaker->playing || speaker->audioStream->samples.size() < speaker->minimumPlaySamples)
    return false;

  if (!speaker->playing) {
    speaker->lastPlayTime = Time::monotonicMilliseconds();
    speaker->playing = true;
    MutexLocker lock(m_activeSpeakersMutex);
    m_activeSpeakers.insert(speaker);
  }
  return true;
}

void Voice::thread() {
  while (true) {
    MutexLocker locker(m_threadMutex);

    m_threadCond.wait(m_threadMutex);
    if (m_stopThread)
      return;

    {
      MutexLocker locker(m_captureMutex);
      ByteArray encoded(VOICE_MAX_PACKET_SIZE, 0);
      size_t frameSamples = VOICE_FRAME_SIZE * (size_t)m_deviceChannels;
      while (m_capturedChunksFrames >= VOICE_FRAME_SIZE) {
        std::vector<opus_int16> samples;
        samples.reserve(frameSamples);
        size_t samplesLeft = frameSamples;
        while (samplesLeft && !m_capturedChunks.empty()) {
          auto& front = m_capturedChunks.front();
          if (front.exhausted())
            m_capturedChunks.pop();
          else
            samplesLeft -= front.takeSamples(samples, samplesLeft);
        }
        m_capturedChunksFrames -= VOICE_FRAME_SIZE;

        if (m_inputAmplitude != 1.0f) {
          for (size_t i = 0; i != samples.size(); ++i)
            samples[i] *= m_inputAmplitude;
        }

        if (int encodedSize = opus_encode(m_encoder.get(), samples.data(), VOICE_FRAME_SIZE, (unsigned char*)encoded.ptr(), encoded.size())) {
          if (encodedSize == 1)
            continue;

          encoded.resize(encodedSize);

          {
            MutexLocker locker(m_encodeMutex);
            m_encodedChunks.emplace_back(std::move(encoded));
            m_encodedChunksLength += encodedSize;

            encoded = ByteArray(VOICE_MAX_PACKET_SIZE, 0);
          }

          //Logger::info("Voice: encoded Opus chunk {} samples -> {} bytes", frameSamples, encodedSize);
        }
        else if (encodedSize < 0)
          Logger::error("Voice: Opus encode error {}", opus_strerror(encodedSize));
      }
    }

    continue;

    locker.unlock();
    Thread::yield();
  }
  return;
}

}// namespace Star