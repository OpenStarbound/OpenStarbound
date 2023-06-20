#ifndef STAR_PLAYER_CODEXES_HPP
#define STAR_PLAYER_CODEXES_HPP

#include "StarUuid.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_CLASS(Codex);
STAR_STRUCT(CodexEntry);
STAR_CLASS(UniverseClient);

class PlayerCodexes {
public:
  typedef pair<CodexConstPtr, bool> CodexEntry;

  PlayerCodexes(Json const& json = {});

  Json toJson() const;

  List<CodexEntry> codexes() const;

  bool codexKnown(String const& codexId) const;
  CodexConstPtr learnCodex(String const& codexId, bool markRead = false);

  bool codexRead(String const& codexId) const;
  bool markCodexRead(String const& codexId);
  bool markCodexUnread(String const& codexId);

  void learnInitialCodexes(String const& playerSpecies);

  CodexConstPtr firstNewCodex() const;

private:
  StringMap<CodexEntry> m_codexes;
};

typedef shared_ptr<PlayerCodexes> PlayerCodexesPtr;
}

#endif
