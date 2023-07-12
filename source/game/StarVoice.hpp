#ifndef STAR_VOICE_HPP
#define STAR_VOICE_HPP
#include "StarString.hpp"

namespace Star {

  STAR_CLASS(Voice);

  class Voice {
  public:
    void mix(int16_t* buffer, size_t frames, unsigned channels);

    // Get pointer to the singleton Voice instance, if it exists.  Otherwise,
    // returns nullptr.
    static Voice* singletonPtr();

    // Gets reference to Voice singleton, throws VoiceException if root
    // is not initialized.
    static Voice& singleton();

    Voice();
    ~Voice();

    Voice(Voice const&) = delete;
    Voice& operator=(Voice const&) = delete;
  private:
    static Voice* s_singleton;
  };
  
}

#endif