#include "StarVoice.hpp"
#include "StarFormat.hpp"
#include "StarApplicationController.hpp"
#include "StarTime.hpp"
#include "StarRoot.hpp"
#include "StarLogging.hpp"
#include "opus/include/opus.h"

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

struct VoiceAudioStream {
  // TODO: This should really be a ring buffer instead.
  std::queue<int16_t> samples;
	SDL_AudioStream* sdlAudioStream;
  Mutex mutex;

	VoiceAudioStream() : sdlAudioStream(SDL_NewAudioStream(AUDIO_S16, 2, 48000, AUDIO_S16SYS, 2, 44100)) {};
	~VoiceAudioStream() { SDL_FreeAudioStream(sdlAudioStream); }

	inline int16_t take() {
		int16_t sample = 0;
		if (!samples.empty()) {
			sample = samples.front();
			samples.pop();
		}
		return sample;
	}

	size_t resample(int16_t* in, size_t inSamples, std::vector<int16_t>& out) {
		SDL_AudioStreamPut(sdlAudioStream, in, inSamples * sizeof(int16_t));
		if (int available = SDL_AudioStreamAvailable(sdlAudioStream)) {
			out.resize(available / 2);
			SDL_AudioStreamGet(sdlAudioStream, out.data(), available);
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

  save();
	closeDevice();

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
  m_inputMode    = VoiceInputModeNames.getLeft(config.getString("inputMode", "PushToTalk"));
  m_channelMode  = VoiceChannelModeNames.getLeft(config.getString("channelMode", "Mono"));
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
  if (!m_nextSaveTime)
		m_nextSaveTime = Time::monotonicMilliseconds() + 2000;
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

void Voice::readAudioData(uint8_t* stream, int len) {
	auto now = Time::monotonicMilliseconds();
	if (!m_encoder || m_inputMode == VoiceInputMode::PushToTalk && now > m_lastInputTime)
		return;

	// Stop encoding if 2048 bytes have been encoded and not taken by the game thread yet
	if (m_encodedChunksLength > 2048)
		return;

	size_t sampleCount = len / 2;
	float decibels = getAudioLoudness((int16_t*)stream, sampleCount);
	m_clientSpeaker->decibelLevel = decibels;

	bool active = true;

	if (m_inputMode == VoiceInputMode::VoiceActivity) {
		if (decibels > m_threshold)
			m_lastThresholdTime = now;
		active = now - m_lastThresholdTime < 50;
	}

	bool added = false;

	MutexLocker captureLock(m_captureMutex);
	if (active) {
		m_capturedChunksFrames += sampleCount / m_deviceChannels;
		auto data = (opus_int16*)malloc(len);
		memcpy(data, stream, len);
		m_capturedChunks.emplace(data, sampleCount); // takes ownership
		added = true;
	}
	else { // Clear out any residual data so they don't manifest at the start of the next encode, whenever that is
		while (!m_capturedChunks.empty())
			m_capturedChunks.pop();

		m_capturedChunksFrames = 0;
	}

	if (added)
		m_threadCond.signal();
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
			if (!audio->samples.empty()) {
				SDL_AudioStream* sdlStream = audio->sdlAudioStream;
				if (!speaker->muted) {
					mix = true;
					for (size_t i = 0; i != samples; ++i)
						speakerBuffer[i] = audio->take();

					speaker->decibelLevel = getAudioLoudness(speakerBuffer.data(), samples);
					auto channelVolumes = speaker->channelVolumes.load();
					for (size_t i = 0; i != samples; ++i)
						sharedBuffer[i] += (int32_t)(speakerBuffer[i]) * channelVolumes[i % 2];
				}
				else {
					for (size_t i = 0; i != samples; ++i)
						audio->take();
				}
				++it;
			}
			else {
				speaker->playing = false;
				it = m_activeSpeakers.erase(it);
			}
		}
	}

	if (mix) {
		finalBuffer.resize(sharedBuffer.size(), 0);

		float vol = m_outputVolume;
		for (size_t i = 0; i != sharedBuffer.size(); ++i)
			finalBuffer[i] = (int16_t)clamp<int>(sharedBuffer[i] * vol, INT16_MIN, INT16_MAX);

		SDL_MixAudioFormat((Uint8*)buffer, (Uint8*)finalBuffer.data(), AUDIO_S16, finalBuffer.size() * sizeof(int16_t), SDL_MIX_MAXVOLUME);
		memset(sharedBuffer.data(), 0, sharedBuffer.size() * sizeof(int32_t));
	}
}

void Voice::update(PositionalAttenuationFunction positionalAttenuationFunction) {
  if (positionalAttenuationFunction) {
    for (auto& entry : m_speakers) {
      if (SpeakerPtr& speaker = entry.second) {
        speaker->channelVolumes = {
          1.0f - positionalAttenuationFunction(0, speaker->position, 1.0f),
					1.0f - positionalAttenuationFunction(1, speaker->position, 1.0f)
        };
      }
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

int Voice::send(DataStreamBuffer& out, size_t budget) {
	out.setByteOrder(ByteOrder::LittleEndian);
	out.write<uint16_t>(VOICE_VERSION);

	MutexLocker encodeLock(m_encodeMutex);

	if (m_encodedChunks.empty())
		return 0;

	std::vector<ByteArray> encodedChunks = move(m_encodedChunks);
	size_t encodedChunksLength = m_encodedChunksLength;
	m_encodedChunksLength = 0;

	encodeLock.unlock();

	for (auto& chunk : encodedChunks) {
		out.write<uint32_t>(chunk.size());
		out.writeBytes(chunk);
		if ((budget -= min<size_t>(budget, chunk.size())) == 0)
			break;
	}

	m_lastSentTime = Time::monotonicMilliseconds();
	return 1;
}

bool Voice::receive(SpeakerPtr speaker, std::string_view view) {
	if (!speaker || view.empty())
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

			size_t decodeBufferSize = samples * sizeof(opus_int16) * (size_t)channels;
			opus_int16* decodeBuffer = (opus_int16*)malloc(decodeBufferSize);

			int decodedSamples = opus_decode(decoder, opusData, opusLength, decodeBuffer, decodeBufferSize, 0);
			if (decodedSamples <= 0) {
				free(decodeBuffer);
				if (decodedSamples < 0)
				  throw VoiceException(strf("Decoder error: {}", opus_strerror(samples)), false);
				return true;
			}

			decodedSamples *= channels;
			//Logger::info("Voice: decoded Opus chunk {} bytes -> {} samples", opusLength, decodedSamples);

			{
				std::vector<int16_t> resamBuffer(decodedSamples, 0);
				speaker->audioStream->resample(decodeBuffer, decodedSamples, resamBuffer);

			  MutexLocker lock(speaker->audioStream->mutex);
				auto& samples = speaker->audioStream->samples;

				auto now = Time::monotonicMilliseconds();
				if (now - speaker->lastReceiveTime < 1000) {
					auto limit = ((size_t)speaker->minimumPlaySamples + 22050) * (size_t)channels;
					if (samples.size() > limit) { // skip ahead if we're getting too far
						for (size_t i = samples.size(); i >= limit; --i)
							samples.pop();
					}
				}
				else
					samples = std::queue<int16_t>();

				speaker->lastReceiveTime = now;

				if (mono) {
					for (int16_t sample : resamBuffer) {
						samples.push(sample);
						samples.push(sample);
					}
				}
				else {
					for (int16_t sample : resamBuffer)
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
  m_lastInputTime = input ? Time::monotonicMilliseconds() + 1000 : 0;
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

  m_applicationController->openAudioInputDevice(
		m_deviceName ? m_deviceName->utf8Ptr() : nullptr,
		VOICE_SAMPLE_RATE,
		m_deviceChannels = encoderChannels(),
		this,
		[](void* userdata, uint8_t* stream, int len) {
      ((Voice*)(userdata))->readAudioData(stream, len);
    }
	);

  m_deviceOpen = true;
}

void Voice::closeDevice() {
  if (!m_deviceOpen)
    return;

  m_applicationController->closeAudioInputDevice();

  m_deviceOpen = false;
}

bool Voice::playSpeaker(SpeakerPtr const& speaker, int channels) {
	unsigned int minSamples = speaker->minimumPlaySamples * channels;
	if (speaker->playing || speaker->audioStream->samples.size() < minSamples)
		return false;

	speaker->playing = true;
	MutexLocker lock(m_activeSpeakersMutex);
	m_activeSpeakers.insert(speaker);
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

				if (m_inputVolume != 1.0f) {
					for (size_t i = 0; i != samples.size(); ++i)
						samples[i] *= m_inputVolume;
				}

				if (int encodedSize = opus_encode(m_encoder.get(), samples.data(), VOICE_FRAME_SIZE, (unsigned char*)encoded.ptr(), encoded.size())) {
					if (encodedSize == 1)
						continue;

					encoded.resize(encodedSize);

					{
						MutexLocker lock(m_encodeMutex);
						m_encodedChunks.emplace_back(move(encoded));
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

}