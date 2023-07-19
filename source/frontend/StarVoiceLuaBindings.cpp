#include "StarVoiceLuaBindings.hpp"
#include "StarVoice.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeVoiceCallbacks(Voice* voice) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("getSettings",   [voice]() -> Json      { return voice->saveJson();         });
  callbacks.registerCallback("mergeSettings", [voice](Json const& settings) { voice->loadJson(settings); });

  callbacks.registerCallback("speakers", [voice](Maybe<bool> onlyPlaying) -> List<Json> {
    List<Json> list;

    for (auto& speaker : voice->speakers(onlyPlaying.value(true))) {
      list.append(JsonObject{
        {"speakerId",      speaker->speakerId           },
        {"entityId",       speaker->entityId            },
        {"name",           speaker->name                },
        {"playing",        (bool)speaker->playing       },
        {"muted",          (bool)speaker->muted         },
        {"decibels",       (float)speaker->decibelLevel },
        {"smoothDecibels", (float)speaker->smoothDb     },
      });
    }

    return list;
  });

  return callbacks;
}

}
