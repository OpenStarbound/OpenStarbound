#include "StarVoiceLuaBindings.hpp"
#include "StarVoice.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeVoiceCallbacks(Voice* voice) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("speakers", [voice](Maybe<bool> onlyPlaying) -> List<Json> {
    List<Json> list;

    for (auto& speaker : voice->speakers(onlyPlaying.value(true))) {
      list.append(JsonObject{
        {"speakerId", speaker->speakerId },
        {"entityId",  speaker->entityId },
        {"name",      speaker->name },
        {"playing",   (bool)speaker->playing },
        {"muted",     (bool)speaker->muted },
        {"loudness",  (float)speaker->decibelLevel },
      });
    }

    return list;
  });

  return callbacks;
}

}
