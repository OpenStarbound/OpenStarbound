#include "StarSongbook.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarLexicalCast.hpp"
#include "StarRandom.hpp"
#include "StarWorld.hpp"
#include "StarLogging.hpp"
#include "StarEntityRendering.hpp"
#include "StarTime.hpp"

namespace Star {

Mutex Songbook::s_timeSourcesMutex;
StringMap<shared_ptr<Songbook::TimeSource>> Songbook::s_timeSources;

Songbook::Songbook(String const& species) {
  m_activeCooldown = 0;
  m_dataUpdated = false;
  m_dataChanged = false;
  m_timeSourceEpoch = 0;
  m_globalNowDelta = 0;
  m_species = species;
  m_stopped = true;
  m_serverMode = true;

  addNetElement(&m_songNetState);
  addNetElement(&m_timeSourceEpochNetState);
  addNetElement(&m_timeSourceNetState);
  addNetElement(&m_activeNetState);
  addNetElement(&m_instrumentNetState);
}

Songbook::~Songbook() {
  stop();
}

Songbook::NoteMapping& Songbook::noteMapping(String const& instrument, String const& species, int note) {
  if (!m_noteMapping.contains(instrument)) {
    Map<int, NoteMapping> notemap;
    auto tuning = Root::singleton().assets()->json(strf("/sfx/instruments/%s/tuning.config", instrument));
    for (auto e : tuning.get("mapping").iterateObject()) {
      int keyNumber = lexicalCast<int>(e.first);
      NoteMapping nm;
      if (e.second.contains("file")) {
        nm.files.append(e.second.getString("file", "").replace("$instrument$", instrument).replace("$species$", species));
      } else if (e.second.contains("files")) {
        for (auto entry : e.second.getArray("files"))
          nm.files.append(entry.toString().replace("$instrument$", instrument).replace("$species$", species));
      }
      nm.frequency = e.second.getDouble("f");
      nm.velocity = 1;
      nm.fadeout = e.second.getDouble("fadeOut", tuning.getDouble("fadeout"));
      notemap[keyNumber] = nm;
    }
    for (int key = 21; key <= 108; ++key) {
      NoteMapping& nm = notemap[key];
      if (nm.files.empty()) {
        auto prev = notemap[key - 1];
        nm.files = prev.files;
        nm.velocity = prev.velocity * nm.frequency / prev.frequency;
      }
    }
    m_noteMapping[instrument] = notemap;
  }
  return m_noteMapping[instrument][note];
}

void Songbook::update(EntityMode mode, World* world) {
  m_serverMode = world->isServer();

  if (m_serverMode)
    return;

  m_globalNowDelta = world->epochTime() * 1000 - Time::millisecondsSinceEpoch();
  if (m_dataUpdated) {
    m_dataUpdated = false;
    if (!m_song.isNull()) {
      try {
        {
          MutexLocker lock(s_timeSourcesMutex);
          if (!s_timeSources.contains(m_timeSource)) {
            m_timeSourceInstance = make_shared<TimeSource>();
            s_timeSources[m_timeSource] = m_timeSourceInstance;
            m_timeSourceInstance->epoch = m_timeSourceEpoch;
            m_timeSourceInstance->keepalive = m_timeSourceEpoch;
          } else
            m_timeSourceInstance = s_timeSources[m_timeSource];
        }
        m_track.clear();
        m_stopped = false;
        m_track.appendAll(parseABC(m_song.getString("abc")));
      } catch (StarException const& e) {
        Logger::error("Failed to handle abc: %s", outputException(e, true));
        m_stopped = true;
      }
    }
  }
  if (mode == EntityMode::Master) {
    if (active())
      m_activeCooldown--;
  }
  playback();
}

void Songbook::playback() {
  if (!active() || (m_track.empty() && m_heldNotes.empty())) {
    stop();
    return;
  }
  m_timeSourceInstance->keepalive = Time::millisecondsSinceEpoch();
  auto now = (Time::millisecondsSinceEpoch() - m_timeSourceInstance->epoch) / 1000.0;
  if (!m_heldNotes.empty()) {
    for (auto& note : m_heldNotes)
      note.audio->setPosition(m_position);
    eraseWhere(m_heldNotes, [&](HeldNote const& note) -> bool {
        return note.audio->finished();
      });
  }

  while (!m_track.empty() && (m_track.first().timecode <= (now + 0.5))) {
    auto note = m_track.takeFirst();
    auto delta = now - note.timecode;
    if (delta > 1)
      continue; // skip notes that are more than a second behind
    if (!m_uncompressedSamples.contains(note.file)) {
      auto sample = Root::singleton().assets()->audio(note.file);
      if (sample->compressed()) {
        auto copy = make_shared<Audio>(*sample);
        copy->uncompress();
        m_uncompressedSamples[note.file] = copy;
      } else
        m_uncompressedSamples[note.file] = sample;
    }

    AudioInstancePtr audioInstance = make_shared<AudioInstance>(*m_uncompressedSamples[note.file]);
    audioInstance->setPitchMultiplier(note.velocity);

    auto start = m_timeSourceInstance->epoch + (int64_t)(note.timecode * 1000.0);
    audioInstance->setClockStart(start);
    audioInstance->setClockStop(start + (int64_t)(note.duration * 1000.0), (int64_t)(note.fadeout * 1000.0));

    audioInstance->setPosition(m_position);

    m_pendingAudio.append(audioInstance);
    m_heldNotes.append(HeldNote{audioInstance, note.timecode, note.duration + note.timecode});
  }
}

void Songbook::render(RenderCallback* renderCallback) {
  for (auto& a : m_pendingAudio)
    renderCallback->addAudio(a);
  m_pendingAudio.clear();
}

void Songbook::keepalive(String const& instrument, Vec2F const& position) {
  if (instrument != m_instrument) {
    m_instrument = instrument;
    m_dataUpdated = true;
  }
  m_position = position;
  if (active())
    m_activeCooldown = 3;
}

List<Songbook::Note> Songbook::parseABC(String const& abc) {
  List<Songbook::Note> result;

  StringMap<String> fields;

  auto meter = [&]() -> Vec2I {
    auto m = fields.value("M", "C");
    if (m.equalsIgnoreCase("C"))
      m = "4/4";
    if (m.equalsIgnoreCase("C|"))
      m = "2/2";
    auto p = m.split("/", 1);
    return Vec2I{lexicalCast<int>(p[0]), lexicalCast<int>(p[1])};
  };

  auto noteLength = [&]() -> Vec2I {
    auto m = fields.value("L", "C");
    if (m.equalsIgnoreCase("C"))
      return {1, meter()[1]};
    auto p = m.split("/", 1);
    return Vec2I{lexicalCast<int>(p[0]), lexicalCast<int>(p[1])};
  };

  auto secondsPerBeat = [&]() -> double {
    auto m = fields.value("Q", "120");
    auto p = m.split("=", 1);
    double secondsPerBeat = 60.0 / lexicalCast<double>(p.last());
    if (p.size() > 1) {
      auto pp = p[0].split("/", 1);
      auto d = Vec2I{lexicalCast<int>(pp[0]), lexicalCast<int>(pp[1])};
      secondsPerBeat = d[1] * secondsPerBeat / d[0];
    } else
      secondsPerBeat = noteLength()[1] * secondsPerBeat / noteLength()[0];
    return secondsPerBeat;
  };

  auto transpose = [&]() -> int {
    auto t = fields.value("Transpose", "0");
    return lexicalCast<int>(t);
  };

  auto fetchKeySignatureMapping = [&]() -> List<int> {
    auto cleanupKey = [](String const& key) -> String {
      return key.toLower().replace(" ", "").replace("minor", "m").replace("min", "m").replace("major", "maj");
    };
    String key = cleanupKey(fields.value("K", "c"));
    auto keys = Root::singleton().assets()->json("/songbook.config:keys");
    while (true) {
      if (!keys.contains(key)) {
        Logger::info("Failed to find key %s, falling back to C", key);
        key = "c";
      }
      auto signature = keys.get(key);
      if (signature.isType(Json::Type::String)) {
        key = cleanupKey(signature.toString());
        continue;
      }
      List<int> keySignatureMapping;
      for (auto e : signature.toArray())
        keySignatureMapping.append(e.toInt());
      return keySignatureMapping;
    }
  };

  double now = 0;
  int accidentals = 0;
  bool accidentalSpecified = false;
  double lastBarNow = now;

  Map<int, int> impliedAccidentals;
  bool grouped = false;
  double groupDuration = 0;
  bool dirtyFlags = true;

  List<int> keySignatureMapping;
  List<int> tupleMapping;
  double fullNoteDuration = 0;
  double noteDuration = 0;
  double barDuration = 0;
  Map<int, size_t> pendingTies;
  int groupStartIndex = 0;

  int tupleCount = 0;
  double tupleDurationFactor = 0;
  bool staccato = false;

  for (auto l : abc.replace("\t", " ").splitLines()) {
    if (l[0] == '%') {
      // extended values support, outside of standard, semi standard seen in
      // some files
      // only used for Transpose
      if ((l.length() >= 3) && (l[1] == ' ') && (l[2] == ' ')) {
        auto s = l.substr(3).split(":", 1);
        if (s.size() > 1) {
          fields[s[0].trim()] = s[1].trim();
          dirtyFlags = true;
        }
      }
      continue;
    }

    if (l.contains("%")) {
      // truncate comments
      l = l.split("%", 1)[0];
    }

    if ((l.length() > 1) && (l[1] == ':') && (l[0] != '|')) {
      auto s = l.split(":", 1);
      if (s.size() > 1) {
        fields[s[0].trim()] = s[1].trim();
        dirtyFlags = true;
      }
    } else {
      if (dirtyFlags) {
        keySignatureMapping = fetchKeySignatureMapping();
        fullNoteDuration = secondsPerBeat();
        noteDuration = noteLength()[0] * fullNoteDuration / noteLength()[1];
        auto m = meter();
        barDuration = m[0] * fullNoteDuration / m[1];
        dirtyFlags = false;

        int tdt = 2;

        if ((m[1] == 8) && ((m[0] == 6) || (m[0] == 9) || (m[0] == 12)))
          tdt = 3;

        tupleMapping.clear();
        tupleMapping.append(0);
        tupleMapping.append(0);
        tupleMapping.append(3);
        tupleMapping.append(2);
        tupleMapping.append(3);
        tupleMapping.append(tdt);
        tupleMapping.append(2);
        tupleMapping.append(tdt);
        tupleMapping.append(3);
        tupleMapping.append(tdt);
      }

      Deque<String::Char> buffer(l.begin(), l.end());

      auto peek = [&]() -> String::Char {
        if (buffer.empty())
          return '\0';
        return buffer.first();
      };

      auto readDuration = [&]() -> double {
        double duration = 1;
        if (String::isAsciiNumber(peek())) {
          String s = "";
          while (String::isAsciiNumber(peek())) {
            s += buffer.takeFirst();
          }
          duration *= lexicalCast<int>(s);
        }
        if (peek() == '/') {
          buffer.takeFirst();
          double divisor = 2;
          if (String::isAsciiNumber(peek())) {
            String s = "";
            while (String::isAsciiNumber(peek())) {
              s += buffer.takeFirst();
            }
            divisor = lexicalCast<int>(s);
          }
          duration /= divisor;
        }
        return duration;
      };

      while (!buffer.empty()) {
        auto head = buffer.takeFirst();
        if (String::isSpace(head))
          continue;

        if (head == '|') {
          now = lastBarNow + barDuration;
          lastBarNow = now;
          // section/repetition artifact, nor supported
          if (peek() == ':')
            buffer.takeFirst();
          else if (peek() == ']')
            buffer.takeFirst();
          else {
            if (peek() == '[')
              buffer.takeFirst();
            while (String::isAsciiNumber(peek()))
              buffer.takeFirst();
          }
          accidentals = 0;
          accidentalSpecified = false;
          impliedAccidentals.clear();
          continue;
        }
        if (head == '~') {
          // ornament ?
          continue;
        }
        if (head == ':') {
          // section/repetition artifact, nor supported
          buffer.takeFirst();
          continue;
        }
        if (head == '^') {
          accidentals += 1;
          accidentalSpecified = true;
          continue;
        }
        if (head == '_') {
          accidentals -= 1;
          accidentalSpecified = true;
          continue;
        }
        if (head == '=') {
          accidentals = 0;
          accidentalSpecified = true;
          continue;
        }
        if (head == '[') {
          grouped = true;
          groupStartIndex = result.size();
          continue;
        }
        if (head == ']') {
          grouped = false;
          if (tupleCount > 0)
            tupleCount--;
          auto duration = readDuration();
          if (duration != 1) {
            for (int index = groupStartIndex; index < (int)result.size(); index++) {
              auto& entry = result[index];
              entry.duration *= duration;
            }
          }
          now += groupDuration * duration;
          groupDuration = 0;
          staccato = false;
          continue;
        }
        if (head == '(') {
          if (String::isAsciiNumber(peek())) {
            int p = 0;
            int q = 0;
            int r = 0;
            p = (int)buffer.takeFirst() - (int)'0';
            if (peek() == ':') {
              buffer.takeFirst();
              if (String::isAsciiNumber(peek()))
                q = (int)buffer.takeFirst() - (int)'0';
              if (peek() == ':') {
                buffer.takeFirst();
                if (String::isAsciiNumber(peek()))
                  r = (int)buffer.takeFirst() - (int)'0';
              }
            }

            if (r == 0)
              r = p;
            if (q == 0)
              q = tupleMapping[p];

            tupleCount = p;
            tupleDurationFactor = (float)q / (float)p;
          }

          continue;
        }

        if (head == '+') {
          while (!buffer.empty()) {
            if (buffer.takeFirst() == '+')
              break;
          }
          continue;
        }
        if (head == '!') {
          while (!buffer.empty()) {
            if (buffer.takeFirst() == '!')
              break;
          }
          continue;
        }
        if (head == '.') {
          staccato = true;
          continue;
        }

        if (String::isAsciiLetter(head)) {
          int note = (String::toLower(head) != head) ? 60 : 72;
          while (peek() == ',') {
            buffer.takeFirst();
            note -= 12;
          }
          while (peek() == '\'') {
            buffer.takeFirst();
            note += 12;
          }
          switch (String::toLower(head)) {
            case 'c': {
              note += 0;
              break;
            }
            case 'd': {
              note += 2;
              break;
            }
            case 'e': {
              note += 4;
              break;
            }
            case 'f': {
              note += 5;
              break;
            }
            case 'g': {
              note += 7;
              break;
            }
            case 'a': {
              note += 9;
              break;
            }
            case 'b': {
              note += 11;
              break;
            }
            case 'x': {
              note = 0;
              break;
            }
            case 'z': {
              note = 0;
              break;
            }
            default:
              throw StarException(strf("Unrecognized note %s", head));
          }
          if (note != 0) {
            bool accidentalActive = accidentalSpecified;
            if (!accidentalSpecified) {
              if (impliedAccidentals.contains(note)) {
                accidentals = impliedAccidentals.value(note);
                accidentalActive = true;
              }
            } else {
              impliedAccidentals[note] = accidentals;
            }
            note += accidentals;

            // int base = note;

            if (!accidentalActive) {
              switch (String::toLower(head)) {
                case 'c': {
                  note += keySignatureMapping[0];
                  break;
                }
                case 'd': {
                  note += keySignatureMapping[1];
                  break;
                }
                case 'e': {
                  note += keySignatureMapping[2];
                  break;
                }
                case 'f': {
                  note += keySignatureMapping[3];
                  break;
                }
                case 'g': {
                  note += keySignatureMapping[4];
                  break;
                }
                case 'a': {
                  note += keySignatureMapping[5];
                  break;
                }
                case 'b': {
                  note += keySignatureMapping[6];
                  break;
                }
                case 'x': {
                  break;
                }
                case 'z': {
                  break;
                }
                default:
                  throw StarException(strf("Unrecognized note %s", head));
              }
            }

            // std::cerr << ":" << note << ":" << (int)accidentalSpecified <<
            // ":" <<
            // (int)accidentalActive << ":" << accidentals << ":" << (note-base)
            // << ":" << head <<
            // "\n";
          }
          accidentals = 0;
          accidentalSpecified = false;
          double duration = readDuration();
          duration *= noteDuration;

          if (tupleCount > 0)
            duration *= tupleDurationFactor;

          double noteDuration = duration;
          if (staccato)
            noteDuration *= 0.5;

          if (peek() == '-') {
            if (note != 0) {
              note += transpose();
              if (pendingTies.contains(note)) {
                auto& noteInstance = result[pendingTies.get(note)];
                noteInstance.duration += noteDuration;
              } else {
                auto mapping = noteMapping(m_instrument, m_species, note);
                result.append(Note{
                    m_instrument,
                    Random::randFrom(mapping.files),
                    now,
                    noteDuration,
                    mapping.fadeout,
                    mapping.velocity
                  });
                pendingTies.add(note, result.size() - 1);
              }
            }
          } else {
            if (note != 0) {
              note += transpose();
              if (pendingTies.contains(note)) {
                auto& noteInstance = result[pendingTies.take(note)];
                noteInstance.duration += noteDuration;
              } else {
                auto mapping = noteMapping(m_instrument, m_species, note);
                result.append(Note{
                    m_instrument,
                    Random::randFrom(mapping.files),
                    now,
                    noteDuration,
                    mapping.fadeout,
                    mapping.velocity
                  });
              }
            }
          }
          if (!grouped) {
            if (tupleCount > 0)
              tupleCount--;
            now += duration;
            staccato = false;
          } else if (groupDuration == 0)
            groupDuration = duration;
          else
            groupDuration = std::min(groupDuration, duration);
        }
      }
    }
  }
  sortByComputedValue(result, [](Note const& note) { return note.timecode; });
  return result;
}

void Songbook::stop() {
  if (m_stopped)
    return;
  m_stopped = true;
  m_track.clear();
  m_heldNotes.clear();
  m_pendingAudio.clear();
  m_noteMapping.clear();
  m_uncompressedSamples.clear();
  m_activeCooldown = 0;
  m_song = {};
  m_dataUpdated = true;
  m_dataChanged = true;
}

void Songbook::play(Json const& song, String const& timeSource) {
  stop();

  m_song = song;
  m_timeSource = timeSource;
  if (m_timeSource.empty())
    m_timeSource = toString(Random::randu64());

  {
    m_timeSourceEpoch = Time::millisecondsSinceEpoch();
    MutexLocker lock(s_timeSourcesMutex);
    if (!s_timeSources.contains(m_timeSource)) {
      m_timeSourceInstance = make_shared<TimeSource>();
      s_timeSources[m_timeSource] = m_timeSourceInstance;
      m_timeSourceInstance->epoch = m_timeSourceEpoch;
      m_timeSourceInstance->keepalive = m_timeSourceEpoch;
    } else {
      m_timeSourceInstance = s_timeSources[m_timeSource];
      if ((Time::millisecondsSinceEpoch() - m_timeSourceInstance->keepalive) > 5000) {
        m_timeSourceInstance->epoch = m_timeSourceEpoch;
        m_timeSourceInstance->keepalive = m_timeSourceEpoch;
      }
    }
    m_timeSourceEpoch = m_timeSourceInstance->epoch;
  }

  m_dataUpdated = true;
  m_dataChanged = true;
  m_activeCooldown = 3;
}

bool Songbook::active() {
  return m_activeCooldown > 0;
}

bool Songbook::instrumentPlaying() {
  if (!active())
    return false;
  if (m_timeSourceInstance) {
    auto now = (Time::millisecondsSinceEpoch() - m_timeSourceInstance->epoch) / 1000.0;
    for (auto n : m_heldNotes) {
      if ((n.start <= now) && (now <= n.end))
        return true;
    }
  }
  return false;
}

double Songbook::fundamentalFrequency(double p) {
  return 55.0 * pow(2.0, (p - 69.0) / 12.0 + 3.0);
}

double Songbook::fundamentalPitch(double f) {
  return 69.0 + 12 * log2(f / 440.0);
}

void Songbook::netElementsNeedLoad(bool) {
  if (m_songNetState.pullUpdated()) {
    m_song = m_songNetState.get();
    m_timeSourceEpoch = m_timeSourceEpochNetState.get() - m_globalNowDelta;
    m_dataUpdated = true;
  }
  m_timeSource = m_timeSourceNetState.get();
  if (m_activeNetState.get())
    m_activeCooldown = 3;
  else
    m_activeCooldown = 0;
  m_instrument = m_instrumentNetState.get();
}

void Songbook::netElementsNeedStore() {
  if (m_serverMode)
    return;
  if (m_dataChanged) {
    m_songNetState.set(m_song);
    m_timeSourceEpochNetState.set(m_globalNowDelta + m_timeSourceEpoch);
    m_dataChanged = false;
  }
  m_activeNetState.set(active());
  m_instrumentNetState.set(m_instrument);
  m_timeSourceNetState.set(m_timeSource);
}

}
