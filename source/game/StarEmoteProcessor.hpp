#ifndef STAR_EMOTE_PROCESSOR_HPP
#define STAR_EMOTE_PROCESSOR_HPP

#include "StarHumanoid.hpp"

namespace Star {

STAR_CLASS(EmoteProcessor);

class EmoteProcessor {
public:
  EmoteProcessor();

  HumanoidEmote detectEmotes(String const& chatter) const;

private:
  struct EmoteBinding {
    EmoteBinding() : emote() {}
    String text;
    HumanoidEmote emote;
  };
  List<EmoteBinding> m_emoteBindings;
};

}

#endif
