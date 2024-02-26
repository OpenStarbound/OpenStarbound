#pragma once

#include "StarThread.hpp"
#include "StarJson.hpp"
#include "StarNetElementSystem.hpp"
#include "StarGameTypes.hpp"

namespace Star {

STAR_CLASS(Audio);
STAR_CLASS(AudioInstance);
STAR_CLASS(World);
STAR_CLASS(Songbook);
STAR_CLASS(RenderCallback);

class Songbook : public NetElementSyncGroup {
public:
  Songbook(String const& species);
  ~Songbook();

  void update(EntityMode mode, World* world);
  void render(RenderCallback* renderCallback);

  // instrument needs to tell the songbook what type it is, and needs to keep
  // calling it to signal
  // the instrument is still equiped
  void keepalive(String const& instrument, Vec2F const& position);

  void stop();
  void play(Json const& song, String const& timesource);
  bool active();
  bool instrumentPlaying();

private:
  struct Note {
    String instrument;
    String file;
    double timecode;
    double duration;
    double fadeout;
    double velocity;
  };

  struct HeldNote {
    AudioInstancePtr audio;
    double start;
    double end;
  };

  struct NoteMapping {
    List<String> files;
    double frequency;
    double velocity;
    double fadeout;
  };

  struct TimeSource {
    int64_t keepalive;
    int64_t epoch;
  };

  static double fundamentalFrequency(double p);
  static double fundamentalPitch(double p);

  void netElementsNeedLoad(bool full) override;
  void netElementsNeedStore() override;

  static StringMap<shared_ptr<TimeSource>> s_timeSources;
  static Mutex s_timeSourcesMutex;

  String m_species;
  Vec2F m_position;

  bool m_serverMode;

  int64_t m_globalNowDelta;
  int m_activeCooldown;
  bool m_dataUpdated;
  bool m_dataChanged;

  String m_timeSource;
  int64_t m_timeSourceEpoch;
  bool m_epochUpdated;
  String m_instrument;
  Json m_song;
  bool m_stopped;

  Deque<Note> m_track;

  List<HeldNote> m_heldNotes;
  shared_ptr<TimeSource> m_timeSourceInstance;

  List<AudioInstancePtr> m_pendingAudio;

  List<Note> parseABC(String const& abc);

  StringMap<Map<int, NoteMapping>> m_noteMapping;

  NoteMapping& noteMapping(String const& instrument, String const& species, int note);

  StringMap<AudioConstPtr> m_uncompressedSamples;

  void playback();

  NetElementData<Json> m_songNetState;
  NetElementInt m_timeSourceEpochNetState;
  NetElementString m_timeSourceNetState;
  NetElementBool m_activeNetState;
  NetElementString m_instrumentNetState;
};

}
