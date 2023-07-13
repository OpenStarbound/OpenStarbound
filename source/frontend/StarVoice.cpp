#include "StarVoice.hpp"
#include "StarFormat.hpp"
#include "StarApplicationController.hpp"
#include "StarTime.hpp"
#include "StarRoot.hpp"
#include "opus/include/opus.h"

#include <queue>
#include "SDL.h"

constexpr int VOICE_SAMPLE_RATE = 48000;
constexpr int VOICE_FRAME_SIZE = 960;

constexpr int VOICE_MAX_FRAME_SIZE = 6 * VOICE_FRAME_SIZE;
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

float getAudioChunkLoudness(int16_t* data, size_t samples) {
	if (!samples)
		return 0.f;

	double rms = 0.;
	for (size_t i = 0; i != samples; ++i) {
		float sample = (float)data[i] / 32767.f;
		rms += (double)(sample * sample);
	}

  float fRms = sqrtf((float)(rms / samples));

	if (fRms > 0)
		return std::clamp<float>(20.f * log10f(fRms), -127.f, 0.f);
	else
		return -127.f;
}

float getAudioLoudness(int16_t* data, size_t samples) {
	constexpr size_t CHUNK_SIZE = 50;

	float highest = -127.f;
	for (size_t i = 0; i < samples; i += CHUNK_SIZE) {
		float level = getAudioChunkLoudness(data + i, std::min<size_t>(i + CHUNK_SIZE, samples) - i);
		if (level > highest)
      highest = level;
	}

	return highest;
}

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
		size_t toRead = std::min<size_t>(count, remaining);
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

	inline bool exhausted() {
		return remaining == 0;
	}
};

struct VoiceAudioStream {
  // TODO: This should really be a ring buffer instead.
  std::queue<VoiceAudioChunk> chunks{};
  size_t samples = 0;
  atomic<bool> muted = false;
  atomic<bool> playing = false;
  atomic<float> decibelLevel = 0.0f;
  atomic<Array<float, 2>> channelVolumes = Array<float, 2>::filled(1.0f);

  Mutex mutex;

  inline int16_t getSample() {
		int16_t sample = 0;
		while (!chunks.empty()) {
			auto& front = chunks.front();
			if (front.exhausted()) {
        chunks.pop();
				continue;
			}
			--samples;
			return front.takeSample();
		}
		return 0;
	}

	void nukeSamples(size_t count) {
		while (!chunks.empty() && count > 0) {
			auto& front = chunks.front();
			if (count >= front.remaining) {
				count -= front.remaining;
				samples -= front.remaining;
        chunks.pop();
			}
			else {
				for (size_t i = 0; i != count; ++i) {
					--samples;
					front.takeSample();
				}
				break;
			}
		}
	}

	inline bool empty() { return chunks.empty(); }

	void take(int16_t* ptr, size_t size) {
    MutexLocker lock(mutex);
	  while (samples > 22050 && !chunks.empty()) {
		  samples -= chunks.front().remaining;
		  chunks.pop();
    }
    chunks.emplace(ptr, size);
		samples += size;
	}
};

Voice::Speaker::Speaker(SpeakerId id)
  : decoderMono  (createDecoder(1), opus_decoder_destroy)
  , decoderStereo(createDecoder(2), opus_decoder_destroy) {
  speakerId = id;
  audioStream = make_shared<VoiceAudioStream>();
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

  s_singleton = this;
}

Voice::~Voice() {
  save();

  s_singleton = nullptr;
}

void Voice::init() {
  resetEncoder();
  if (m_inputEnabled)
    openDevice();
}

void Voice::loadJson(Json const& config) {
  m_enabled      = config.getBool("enabled",         m_enabled);
  m_inputEnabled = config.getBool("inputEnabled",    m_inputEnabled);
  m_deviceName   = config.optQueryString("inputDevice");
  m_threshold    = config.getFloat("threshold", m_threshold);
  m_inputVolume  = config.getFloat("inputVolume", m_inputVolume);
  m_outputVolume = config.getFloat("outputVolume",   m_outputVolume);
  m_inputMode    = VoiceInputModeNames.getLeft(config.getString("inputMode", "pushToTalk"));
  m_channelMode  = VoiceChannelModeNames.getLeft(config.getString("channelMode", "mono"));
}



Json Voice::saveJson() const {
  return JsonObject{
    {"enabled",      m_enabled},
    {"inputEnabled", m_inputEnabled},
    {"inputDevice",  m_deviceName ? *m_deviceName : Json()},
    {"threshold",    m_threshold},
    {"inputVolume",  m_inputVolume},
    {"outputVolume", m_outputVolume},
    {"inputMode",    VoiceInputModeNames.getRight(m_inputMode)},
    {"channelMode",  VoiceChannelModeNames.getRight(m_channelMode)},
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
  if (nextSaveTime == 0.0)
    nextSaveTime = Time::monotonicTime() + 2.0;
}

Voice::SpeakerPtr Voice::setLocalSpeaker(SpeakerId speakerId) {
  if (m_speakers.contains(m_speakerId))
    m_speakers.remove(m_speakerId);

  m_clientSpeaker->speakerId = m_speakerId = speakerId;
  return m_speakers.insert(m_speakerId, m_clientSpeaker).first->second;
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

void Voice::getAudioData(uint8_t* stream, int len) {

}

void Voice::mix(int16_t* buffer, size_t frames, unsigned channels) {

}

void Voice::update(PositionalAttenuationFunction positionalAttenuationFunction) {
  if (positionalAttenuationFunction) {
    for (auto& entry : m_speakers) {
      if (SpeakerPtr& speaker = entry.second) {
        speaker->audioStream->channelVolumes = {
          positionalAttenuationFunction(0, speaker->position, 1.0f),
          positionalAttenuationFunction(1, speaker->position, 1.0f)
        };
      }
    }
  }

  auto now = Time::monotonicTime();
  if (now > nextSaveTime) {
    nextSaveTime = 0.0;
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
  m_encoder.reset(createEncoder(channels));
  opus_encoder_ctl(m_encoder.get(), OPUS_SET_BITRATE(channels == 2 ? 50000 : 24000));
}

void Voice::openDevice() {
  closeDevice();

  m_applicationController->openAudioInputDevice(m_deviceName ? m_deviceName->utf8Ptr() : nullptr, VOICE_SAMPLE_RATE, encoderChannels(), this, [](void* userdata, uint8_t* stream, int len) {
    ((Voice*)(userdata))->getAudioData(stream, len);
  });

  m_deviceOpen = true;
}

void Voice::closeDevice() {
  if (!m_deviceOpen)
    return;

  m_applicationController->closeAudioInputDevice();

  m_deviceOpen = false;
}

}