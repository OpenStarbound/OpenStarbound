#include "StarMainMixer.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarUniverseClient.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"
#include "StarWorldClient.hpp"
#include "StarWorldPainter.hpp"
#include "StarVoice.hpp"

namespace Star {

MainMixer::MainMixer(unsigned sampleRate, unsigned channels) {
  m_mixer = make_shared<Mixer>(sampleRate, channels);
}

void MainMixer::setUniverseClient(UniverseClientPtr universeClient) {
  m_universeClient = move(universeClient);
}

void MainMixer::setWorldPainter(WorldPainterPtr worldPainter) {
  m_worldPainter = move(worldPainter);
}

void MainMixer::update(bool muteSfx, bool muteMusic) {
  auto assets = Root::singleton().assets();

  auto updateGroupVolume = [&](MixerGroup group, bool muted, String const& settingName) {
      if (m_mutedGroups.contains(group) != muted) {
        if (muted) {
          m_mutedGroups.add(group);
          m_mixer->setGroupVolume(group, 0, 1.0f);
        } else {
          m_mutedGroups.remove(group);
          m_mixer->setGroupVolume(group, m_groupVolumes[group], 1.0f);
        }
      } else if (!m_mutedGroups.contains(group)) {
        float volumeSetting = Root::singleton().configuration()->get(settingName).toFloat() / 100.0f;
        if (!m_groupVolumes.contains(group) || volumeSetting != m_groupVolumes[group]) {
          m_mixer->setGroupVolume(group, volumeSetting);
          m_groupVolumes[group] = volumeSetting;
        }
      }
    };

  updateGroupVolume(MixerGroup::Effects, muteSfx, "sfxVol");
  updateGroupVolume(MixerGroup::Music, muteMusic, "musicVol");
  updateGroupVolume(MixerGroup::Cinematic, false, "sfxVol");

  WorldClientPtr currentWorld;
  if (m_universeClient)
    currentWorld = m_universeClient->worldClient();

  if (currentWorld) {
    for (auto audioInstance : currentWorld->pullPendingAudio()) {
      audioInstance->setMixerGroup(MixerGroup::Effects);
      m_mixer->play(audioInstance);
    }
    for (auto audioInstance : currentWorld->pullPendingMusic()) {
      audioInstance->setMixerGroup(MixerGroup::Music);
      m_mixer->play(audioInstance);
    }

    if (m_universeClient && m_universeClient->mainPlayer()->underwater()) {
      if (!m_mixer->hasEffect("lowpass"))
        m_mixer->addEffect("lowpass", m_mixer->lowpass(32), 0.50f);
      if (!m_mixer->hasEffect("echo"))
        m_mixer->addEffect("echo", m_mixer->echo(0.2f, 0.6f, 0.4f), 0.50f);
    } else {
      if (m_mixer->hasEffect("lowpass"))
        m_mixer->removeEffect("lowpass", 0.5f);
      if (m_mixer->hasEffect("echo"))
        m_mixer->removeEffect("echo", 0.5f);
    }

    float baseMaxDistance = assets->json("/sfx.config:baseMaxDistance").toFloat();
    Vec2F stereoAdjustmentRange = jsonToVec2F(assets->json("/sfx.config:stereoAdjustmentRange"));
    float attenuationGamma = assets->json("/sfx.config:attenuationGamma").toFloat();
    auto playerPos = m_universeClient->mainPlayer()->position();
    auto cameraPos = m_worldPainter->camera().centerWorldPosition();
    auto worldGeometry = currentWorld->geometry();

    Mixer::PositionalAttenuationFunction attenuationFunction = [&](unsigned channel, Vec2F pos, float rangeMultiplier) {
      Vec2F playerDiff = worldGeometry.diff(pos, playerPos);
      Vec2F cameraDiff = worldGeometry.diff(pos, cameraPos);
      float playerMagSq = playerDiff.magnitudeSquared();
      float cameraMagSq = cameraDiff.magnitudeSquared();

      Vec2F diff;
      float diffMagnitude;
      if (playerMagSq < cameraMagSq) {
        diff = playerDiff;
        diffMagnitude = sqrt(playerMagSq);
      }
      else {
        diff = cameraDiff;
        diffMagnitude = sqrt(cameraMagSq);
      }

      if (diffMagnitude == 0.0f)
        return 0.0f;

      Vec2F diffNorm = diff / diffMagnitude;

      float stereoIncidence = channel == 0 ? -diffNorm[0] : diffNorm[0];

      float maxDistance = baseMaxDistance * rangeMultiplier * lerp((stereoIncidence + 1.0f) / 2.0f, stereoAdjustmentRange[0], stereoAdjustmentRange[1]);

      return pow(clamp(diffMagnitude / maxDistance, 0.0f, 1.0f), 1.0f / attenuationGamma);
    };

    if (Voice* voice = Voice::singletonPtr())
      voice->update(attenuationFunction);

    m_mixer->update(attenuationFunction);

  } else {
    if (m_mixer->hasEffect("lowpass"))
      m_mixer->removeEffect("lowpass", 0);
    if (m_mixer->hasEffect("echo"))
      m_mixer->removeEffect("echo", 0);

    m_mixer->update();
  }
}

MixerPtr MainMixer::mixer() const {
  return m_mixer;
}

void MainMixer::setVolume(float volume, float rampTime) {
  m_mixer->setVolume(volume, rampTime);
}

void MainMixer::read(int16_t* sampleData, size_t frameCount, Mixer::ExtraMixFunction extraMixFunction) {
  m_mixer->read(sampleData, frameCount, extraMixFunction);
}

}
