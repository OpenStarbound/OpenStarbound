#include "StarLuaConverters.hpp"
#include "StarVoiceLuaBindings.hpp"
#include "StarVoice.hpp"


namespace Star {

typedef Voice::SpeakerId SpeakerId;
LuaCallbacks LuaBindings::makeVoiceCallbacks() {
  LuaCallbacks callbacks;

  auto voice = Voice::singletonPtr();

  callbacks.registerCallbackWithSignature<StringList>("devices", bind(&Voice::availableDevices, voice));
  callbacks.registerCallback(  "getSettings", [voice]() -> Json      { return voice->saveJson();         });
  callbacks.registerCallback("mergeSettings", [voice](Json const& settings) { voice->loadJson(settings); });
  // i have an alignment addiction i'm so sorry
  callbacks.registerCallback("setSpeakerMuted",  [voice](SpeakerId speakerId, bool muted)  { voice->speaker(speakerId)->muted = muted; });
  callbacks.registerCallback(   "speakerMuted",  [voice](SpeakerId speakerId) { return (bool)voice->speaker(speakerId)->muted;         });
  // it just looks so neat to me!!
  callbacks.registerCallback("setSpeakerVolume", [voice](SpeakerId speakerId, float volume) { voice->speaker(speakerId)->volume = volume; });
  callbacks.registerCallback(   "speakerVolume", [voice](SpeakerId speakerId) { return (float)voice->speaker(speakerId)->volume;          });

  callbacks.registerCallback("speakerPosition", [voice](SpeakerId speakerId) { return voice->speaker(speakerId)->position; });

  callbacks.registerCallback("speaker",  [voice](Maybe<SpeakerId> speakerId) {
    if (speakerId)
      return voice->speaker(*speakerId)->toJson();
    else
      return voice->localSpeaker()->toJson();
  });

  callbacks.registerCallback("speakers", [voice](Maybe<bool> onlyPlaying) -> List<Json> {
    List<Json> list;

    for (auto& speaker : voice->sortedSpeakers(onlyPlaying.value(true)))
      list.append(speaker->toJson());

    return list;
  });

  return callbacks;
}

}
