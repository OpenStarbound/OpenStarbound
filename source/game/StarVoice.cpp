#include "StarVoice.hpp"
#include "StarFormat.hpp"
#include "opus/include/opus.h"

#include "SDL.h"

namespace Star {

EnumMap<VoiceTriggerMode> const VoiceTriggerModeNames{
  {VoiceTriggerMode::VoiceActivity, "VoiceActivity"},
  {VoiceTriggerMode::PushToTalk, "PushToTalk"}
};

EnumMap<VoiceChannelMode> const VoiceChannelModeNames{
  {VoiceChannelMode::Mono, "Mono"},
  {VoiceChannelMode::Stereo, "Stereo"}
};

static SDL_AudioDeviceID sdlInputDevice = 0;

constexpr int VOICE_SAMPLE_RATE = 48000;
constexpr int VOICE_FRAME_SIZE = 960;

constexpr int VOICE_MAX_FRAME_SIZE = 6 * VOICE_FRAME_SIZE;
constexpr int VOICE_MAX_PACKET_SIZE = 3 * 1276;

constexpr uint16_t VOICE_VERSION = 1;

Voice::Speaker::Speaker(SpeakerId id)
  : decoderMono  (createDecoder(1), opus_decoder_destroy)
  , decoderStereo(createDecoder(2), opus_decoder_destroy) {
  speakerId = id;
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

Voice::Voice() : m_encoder(nullptr, opus_encoder_destroy) {
  if (s_singleton)
    throw VoiceException("Singleton Voice has been constructed twice");

  m_clientSpeaker = make_shared<Speaker>(m_speakerId);
  m_triggerMode = VoiceTriggerMode::PushToTalk;
  m_channelMode = VoiceChannelMode::Mono;

  resetEncoder();
  s_singleton = this;
}

Voice::~Voice() {
  s_singleton = nullptr;
}

void Voice::load(Json const& config) {
  // do stuff
}
Json Voice::save() const {
  return JsonObject{};
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

void Voice::mix(int16_t* buffer, size_t frames, unsigned channels) {

}

void Voice::update(PositionalAttenuationFunction positionalAttenuationFunction) {
  if (positionalAttenuationFunction) {
    for (auto& entry : m_speakers) {
      if (SpeakerPtr& speaker = entry.second) {
        speaker->channelVolumes = {
          positionalAttenuationFunction(0, speaker->position, 1.0f),
          positionalAttenuationFunction(1, speaker->position, 1.0f)
        };
      }
    }
  }
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

}