#include "StarVoice.hpp"

namespace Star {

STAR_EXCEPTION(VoiceException, StarException);

void Voice::mix(int16_t* buffer, size_t frames, unsigned channels) {

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

Voice::Voice() {
  if (s_singleton)
    throw VoiceException("Singleton Voice has been constructed twice");

  s_singleton = this;
}

Voice::~Voice() {
  s_singleton = nullptr;
}

}