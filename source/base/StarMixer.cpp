#include "StarMixer.hpp"
#include "StarIterator.hpp"
#include "StarInterpolation.hpp"
#include "StarTime.hpp"
#include "StarLogging.hpp"

namespace Star {

namespace {
  float rateOfChangeFromRampTime(float rampTime) {
    static const float MaxRate = 10000.0f;

    if (rampTime < 1.0f / MaxRate)
      return MaxRate;
    else
      return 1.0f / rampTime;
  }
}

AudioInstance::AudioInstance(Audio const& audio)
  : m_audio(audio) {
  m_mixerGroup = MixerGroup::Effects;

  m_volume = {1.0f, 1.0f, 0};

  m_pitchMultiplier = 1.0f;
  m_pitchMultiplierTarget = 1.0f;
  m_pitchMultiplierVelocity = 0;

  m_loops = 0;
  m_stopping = false;
  m_finished = false;

  m_rangeMultiplier = 1.0f;

  m_clockStopFadeOut = 0.0f;
}

Maybe<Vec2F> AudioInstance::position() const {
  MutexLocker locker(m_mutex);
  return m_position;
}

void AudioInstance::setPosition(Maybe<Vec2F> position) {
  MutexLocker locker(m_mutex);
  m_position = position;
}

void AudioInstance::translate(Vec2F const& distance) {
  MutexLocker locker(m_mutex);
  if (m_position)
    *m_position += distance;
  else
    m_position = distance;
}

float AudioInstance::rangeMultiplier() const {
  MutexLocker locker(m_mutex);
  return m_rangeMultiplier;
}

void AudioInstance::setRangeMultiplier(float rangeMultiplier) {
  MutexLocker locker(m_mutex);
  m_rangeMultiplier = rangeMultiplier;
}

void AudioInstance::setVolume(float targetValue, float rampTime) {
  starAssert(targetValue >= 0);
  MutexLocker locker(m_mutex);

  if (m_stopping)
    return;

  if (rampTime <= 0.0f) {
    m_volume.value = targetValue;
    m_volume.target = targetValue;
    m_volume.velocity = 0.0f;
  } else {
    m_volume.target = targetValue;
    m_volume.velocity = rateOfChangeFromRampTime(rampTime);
  }
}

void AudioInstance::setPitchMultiplier(float targetValue, float rampTime) {
  starAssert(targetValue >= 0);
  MutexLocker locker(m_mutex);

  if (m_stopping)
    return;

  if (rampTime <= 0.0f) {
    m_pitchMultiplier = targetValue;
    m_pitchMultiplierTarget = targetValue;
    m_pitchMultiplierVelocity = 0.0f;
  } else {
    m_pitchMultiplierTarget = targetValue;
    m_pitchMultiplierVelocity = rateOfChangeFromRampTime(rampTime);
  }
}

int AudioInstance::loops() const {
  MutexLocker locker(m_mutex);
  return m_loops;
}

void AudioInstance::setLoops(int loops) {
  MutexLocker locker(m_mutex);
  m_loops = loops;
}

double AudioInstance::currentTime() const {
  return m_audio.currentTime();
}

double AudioInstance::totalTime() const {
  return m_audio.totalTime();
}

void AudioInstance::seekTime(double time) {
  m_audio.seekTime(time);
}

MixerGroup AudioInstance::mixerGroup() const {
  MutexLocker locker(m_mutex);
  return m_mixerGroup;
}

void AudioInstance::setMixerGroup(MixerGroup mixerGroup) {
  MutexLocker locker(m_mutex);
  m_mixerGroup = mixerGroup;
}

void AudioInstance::setClockStart(Maybe<int64_t> clockStartTime) {
  MutexLocker locker(m_mutex);
  m_clockStart = clockStartTime;
}

void AudioInstance::setClockStop(Maybe<int64_t> clockStopTime, int64_t fadeOutTime) {
  MutexLocker locker(m_mutex);
  m_clockStop = clockStopTime;
  m_clockStopFadeOut = fadeOutTime;
}

void AudioInstance::stop(float rampTime) {
  MutexLocker locker(m_mutex);

  if (rampTime <= 0.0f) {
    m_volume.value = 0.0f;
    m_volume.target = 0.0f;
    m_volume.velocity = 0.0f;

    m_pitchMultiplierTarget = 0.0f;
    m_pitchMultiplierVelocity = 0.0f;
  } else {
    m_volume.target = 0.0f;
    m_volume.velocity = rateOfChangeFromRampTime(rampTime);
  }

  m_stopping = true;
}

bool AudioInstance::finished() const {
  return m_finished;
}

Mixer::Mixer(unsigned sampleRate, unsigned channels) {
  m_sampleRate = sampleRate;
  m_channels = channels;

  m_volume = {1.0f, 1.0f, 0};

  m_groupVolumes[MixerGroup::Effects] = {1.0f, 1.0f, 0};
  m_groupVolumes[MixerGroup::Music] = {1.0f, 1.0f, 0};
  m_groupVolumes[MixerGroup::Cinematic] = {1.0f, 1.0f, 0};
  m_groupVolumes[MixerGroup::Instruments] = {1.0f, 1.0f, 0};

  m_speed = 1.0f;
}

unsigned Mixer::sampleRate() const {
  return m_sampleRate;
}

unsigned Mixer::channels() const {
  return m_channels;
}

void Mixer::addEffect(String const& effectName, EffectFunction effectFunction, float rampTime) {
  MutexLocker locker(m_effectsMutex);
  m_effects[effectName] = make_shared<EffectInfo>(EffectInfo{effectFunction, 0.0f, rateOfChangeFromRampTime(rampTime), false});
}

void Mixer::removeEffect(String const& effectName, float rampTime) {
  MutexLocker locker(m_effectsMutex);
  if (m_effects.contains(effectName))
    m_effects[effectName]->velocity = -rateOfChangeFromRampTime(rampTime);
}

StringList Mixer::currentEffects() {
  MutexLocker locker(m_effectsMutex);
  return m_effects.keys();
}

bool Mixer::hasEffect(String const& effectName) {
  MutexLocker locker(m_effectsMutex);
  return m_effects.contains(effectName);
}

void Mixer::setSpeed(float speed) {
  m_speed = speed;
}

void Mixer::setVolume(float volume, float rampTime) {
  MutexLocker locker(m_mutex);
  m_volume.target = volume;
  m_volume.velocity = rateOfChangeFromRampTime(rampTime);
}

void Mixer::play(AudioInstancePtr sample) {
  MutexLocker locker(m_queueMutex);
  m_audios.add(std::move(sample), AudioState{List<float>(m_channels, 1.0f)});
}

void Mixer::stopAll(float rampTime) {
  MutexLocker locker(m_queueMutex);
  float vel = rateOfChangeFromRampTime(rampTime);
  for (auto const& p : m_audios)
    p.first->stop(vel);
}

void Mixer::read(int16_t* outBuffer, size_t frameCount, ExtraMixFunction extraMixFunction) {
  // Make this method as least locky as possible by copying all the needed
  // member data before the expensive audio / effect stuff.
  unsigned sampleRate;
  unsigned channels;
  float volume;
  float volumeVelocity;
  float targetVolume;
  Map<MixerGroup, RampedValue> groupVolumes;

  {
    MutexLocker locker(m_mutex);
    sampleRate = m_sampleRate;
    channels = m_channels;
    volume = m_volume.value;
    volumeVelocity = m_volume.velocity;
    targetVolume = m_volume.target;
    groupVolumes = m_groupVolumes;
  }

  size_t bufferSize = frameCount * m_channels;
  m_mixBuffer.resize(bufferSize, 0);

  float time = (float)frameCount / sampleRate;
  float beginVolume = volume;
  float endVolume = approach(targetVolume, volume, volumeVelocity * time);

  Map<MixerGroup, float> groupEndVolumes;
  for (auto p : groupVolumes)
    groupEndVolumes[p.first] = approach(p.second.target, p.second.value, p.second.velocity * time);

  auto sampleStartTime = Time::millisecondsSinceEpoch();
  unsigned millisecondsInBuffer = (bufferSize * 1000) / (channels * sampleRate);
  auto sampleEndTime = sampleStartTime + millisecondsInBuffer;

  for (size_t i = 0; i < bufferSize; ++i)
    outBuffer[i] = 0;

  float speed = m_speed;

  {
    MutexLocker locker(m_queueMutex);
    // Mix all active sounds
    for (auto& p : m_audios) {
      auto& audioInstance = p.first;
      auto& audioState = p.second;

      MutexLocker audioLocker(audioInstance->m_mutex);

      if (audioInstance->m_finished)
        continue;

      if (audioInstance->m_clockStart && *audioInstance->m_clockStart > sampleEndTime)
        continue;

      float groupVolume = groupVolumes[audioInstance->m_mixerGroup].value;
      float groupEndVolume = groupEndVolumes[audioInstance->m_mixerGroup];

      bool finished = false;

      float audioStopVolBegin = audioInstance->m_volume.value;
      float audioStopVolEnd = (audioInstance->m_volume.velocity > 0)
          ? approach(audioInstance->m_volume.target, audioStopVolBegin, audioInstance->m_volume.velocity * time)
          : audioInstance->m_volume.value;

      float pitchMultiplier = (audioInstance->m_pitchMultiplierVelocity > 0)
          ? approach(audioInstance->m_pitchMultiplierTarget, audioInstance->m_pitchMultiplier, audioInstance->m_pitchMultiplierVelocity * time)
          : audioInstance->m_pitchMultiplier;

      if (audioInstance->m_mixerGroup == MixerGroup::Effects || audioInstance->m_mixerGroup == MixerGroup::Instruments)
        pitchMultiplier *= speed;

      if (audioStopVolEnd == 0.0f && audioInstance->m_stopping)
        finished = true;

      size_t ramt = 0;
      if (audioInstance->m_clockStart && *audioInstance->m_clockStart > sampleStartTime) {
        int silentSamples = (*audioInstance->m_clockStart - sampleStartTime) * sampleRate / 1000;
        for (unsigned i = 0; i < silentSamples * channels; ++i)
          m_mixBuffer[i] = 0;
        ramt += silentSamples * channels;
      }
      ramt += audioInstance->m_audio.resample(channels, sampleRate, m_mixBuffer.ptr() + ramt, bufferSize - ramt, pitchMultiplier);
      while (ramt != bufferSize && !finished) {
        // Only seek back to the beginning and read more data if loops is < 0
        // (loop forever), or we have more loops to go, otherwise, the sample is
        // finished.
        if (audioInstance->m_loops != 0) {
          audioInstance->m_audio.seekSample(0);
          ramt += audioInstance->m_audio.resample(channels, sampleRate, m_mixBuffer.ptr() + ramt, bufferSize - ramt, pitchMultiplier);
          if (audioInstance->m_loops > 0)
            --audioInstance->m_loops;
        } else {
          finished = true;
        }
      }
      if (audioInstance->m_clockStop && *audioInstance->m_clockStop < sampleEndTime) {
        for (size_t s = 0; s < ramt / channels; ++s) {
          unsigned millisecondsInBuffer = (s * 1000) / sampleRate;
          auto sampleTime = sampleStartTime + millisecondsInBuffer;
          if (sampleTime > *audioInstance->m_clockStop) {
            float volume = 0.0f;
            if (audioInstance->m_clockStopFadeOut > 0)
              volume = 1.0f - (float)(sampleTime - *audioInstance->m_clockStop) / (float)audioInstance->m_clockStopFadeOut;

            if (volume <= 0) {
              for (size_t c = 0; c < channels; ++c)
                m_mixBuffer[s * channels + c] = 0;
            } else {
              for (size_t c = 0; c < channels; ++c)
                m_mixBuffer[s * channels + c] *= volume;
            }
          }
        }
        if (sampleEndTime > *audioInstance->m_clockStop + audioInstance->m_clockStopFadeOut)
          finished = true;
      }

      for (size_t s = 0; s < ramt / channels; ++s) {
        float vol = lerp((float)s / frameCount, beginVolume * groupVolume * audioStopVolBegin, endVolume * groupEndVolume * audioStopVolEnd);
        for (size_t c = 0; c < channels; ++c) {
          float sample = m_mixBuffer[s * channels + c] * vol * audioState.positionalChannelVolumes[c] * audioInstance->m_volume.value;
          int16_t& outSample = outBuffer[s * channels + c];
          outSample = clamp(sample + outSample, -32767.0f, 32767.0f);
        }
      }

      audioInstance->m_volume.value = audioStopVolEnd;
      audioInstance->m_finished = finished;
    }
  }

  if (extraMixFunction)
    extraMixFunction(outBuffer, frameCount, channels);

  {
    MutexLocker locker(m_effectsMutex);
    // Apply all active effects
    for (auto const& pair : m_effects) {
      auto const& effectInfo = pair.second;
      if (effectInfo->finished)
        continue;

      float effectBegin = effectInfo->amount;
      float effectEnd;
      if (effectInfo->velocity < 0)
        effectEnd = approach(0.0f, effectBegin, -effectInfo->velocity * time);
      else
        effectEnd = approach(1.0f, effectBegin, effectInfo->velocity * time);

      std::copy(outBuffer, outBuffer + bufferSize, m_mixBuffer.begin());
      effectInfo->effectFunction(m_mixBuffer.ptr(), frameCount, channels);

      for (size_t s = 0; s < frameCount; ++s) {
        float amt = lerp((float)s / frameCount, effectBegin, effectEnd);
        for (size_t c = 0; c < channels; ++c) {
          int16_t prev = outBuffer[s * channels + c];
          outBuffer[s * channels + c] = lerp(amt, prev, m_mixBuffer[s * channels + c]);
        }
      }

      effectInfo->amount = effectEnd;
      effectInfo->finished = effectInfo->amount <= 0.0f;
    }
  }

  {
    MutexLocker locker(m_mutex);

    m_volume.value = endVolume;

    for (auto p : groupEndVolumes)
      m_groupVolumes[p.first].value = p.second;
  }
}

Mixer::EffectFunction Mixer::lowpass(size_t avgSize) const {
  struct LowPass {
    LowPass(size_t avgSize) : avgSize(avgSize) {}

    size_t avgSize;
    List<Deque<float>> filter;

    void operator()(int16_t* buffer, size_t frames, unsigned channels) {
      filter.resize(channels);
      for (size_t f = 0; f < frames; ++f) {
        for (size_t c = 0; c < channels; ++c) {
          auto& filterChannel = filter[c];
          filterChannel.append(buffer[f * channels + c] / 32767.0f);
          while (filterChannel.size() > avgSize)
            filterChannel.takeFirst();
          buffer[f * channels + c] = sum(filterChannel) / (float)avgSize * 32767.0f;
        }
      }
    }
  };

  return LowPass(avgSize);
}

Mixer::EffectFunction Mixer::echo(float time, float dry, float wet) const {
  struct Echo {
    unsigned echoLength;
    float dry;
    float wet;
    List<Deque<float>> filter;

    void operator()(int16_t* buffer, size_t frames, unsigned channels) {
      if (echoLength == 0)
        return;

      filter.resize(channels);
      for (size_t c = 0; c < channels; ++c) {
        auto& filterChannel = filter[c];
        if (filterChannel.empty())
          filterChannel.resize(echoLength, 0);
      }

      for (size_t f = 0; f < frames; ++f) {
        for (size_t c = 0; c < channels; ++c) {
          auto& filterChannel = filter[c];
          buffer[f * channels + c] = buffer[f * channels + c] * dry + filter[c][0] * wet;
          filterChannel.append(buffer[f * channels + c]);
          while (filterChannel.size() > echoLength)
            filterChannel.takeFirst();
        }
      }
    }
  };

  return Echo{(unsigned)(time * m_sampleRate), dry, wet, {}};
}

void Mixer::setGroupVolume(MixerGroup group, float targetValue, float rampTime) {
  MutexLocker locker(m_mutex);
  if (rampTime <= 0.0f) {
    m_groupVolumes[group].value = targetValue;
    m_groupVolumes[group].target = targetValue;
    m_groupVolumes[group].velocity = 0.0f;
  } else {
    m_groupVolumes[group].target = targetValue;
    m_groupVolumes[group].velocity = rateOfChangeFromRampTime(rampTime);
  }
}

void Mixer::update(float dt, PositionalAttenuationFunction positionalAttenuationFunction) {
  {
    MutexLocker locker(m_queueMutex);
    eraseWhere(m_audios, [&](auto& p) {
        if (p.first->m_finished)
          return true;

        if (positionalAttenuationFunction && p.first->m_position) {
          for (unsigned c = 0; c < m_channels; ++c)
            p.second.positionalChannelVolumes[c] = 1.0f - positionalAttenuationFunction(c, *p.first->m_position, p.first->m_rangeMultiplier);
        } else {
          for (unsigned c = 0; c < m_channels; ++c)
            p.second.positionalChannelVolumes[c] = 1.0f;
        }
        return false;
      });
  }

  {
    MutexLocker locker(m_effectsMutex);
    eraseWhere(m_effects, [](auto const& p) {
        return p.second->finished;
      });
  }
}

}
