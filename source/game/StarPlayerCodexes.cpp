#include "StarPlayerCodexes.hpp"
#include "StarCodex.hpp"
#include "StarCodexDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

PlayerCodexes::PlayerCodexes(Json const& variant) {
  if (variant) {
    auto assets = Root::singleton().assets();
    auto codexData = jsonToMapV<StringMap<bool>>(variant, mem_fn(&Json::toBool));
    for (auto pair : codexData) {
      if (auto codex = Root::singleton().codexDatabase()->codex(pair.first)) {
        m_codexes[pair.first] = CodexEntry{codex, pair.second};
      } else {
        Logger::debug("Failed to load missing codex '%s'", pair.first);
      }
    }
  }
}

Json PlayerCodexes::toJson() const {
  return jsonFromMapV<StringMap<CodexEntry>>(m_codexes, [](CodexEntry const& entry) { return entry.second; });
}

List<PlayerCodexes::CodexEntry> PlayerCodexes::codexes() const {
  List<CodexEntry> result;
  for (auto pair : m_codexes)
    result.append(pair.second);
  sort(result,
      [](CodexEntry const& left, CodexEntry const& right) -> bool {
        return make_tuple(left.second, left.first->title()) < make_tuple(right.second, right.first->title());
      });
  return result;
}

bool PlayerCodexes::codexKnown(String const& codexId) const {
  return m_codexes.contains(codexId);
}

CodexConstPtr PlayerCodexes::learnCodex(String const& codexId, bool markRead) {
  if (!codexKnown(codexId)) {
    if (auto codex = Root::singleton().codexDatabase()->codex(codexId)) {
      auto entry = CodexEntry{codex, markRead};
      m_codexes[codexId] = entry;
      return entry.first;
    }
  }
  return {};
}

bool PlayerCodexes::codexRead(String const& codexId) const {
  return m_codexes.contains(codexId) && m_codexes.get(codexId).second;
}

bool PlayerCodexes::markCodexRead(String const& codexId) {
  if (codexKnown(codexId) && !codexRead(codexId)) {
    m_codexes[codexId].second = true;
    return true;
  }
  return false;
}

bool PlayerCodexes::markCodexUnread(String const& codexId) {
  if (codexKnown(codexId) && codexRead(codexId)) {
    m_codexes[codexId].second = false;
    return true;
  }
  return false;
}

void PlayerCodexes::learnInitialCodexes(String const& playerSpecies) {
  for (auto codexId : jsonToStringList(Root::singleton().assets()->json(strf("/player.config:defaultCodexes.%s", playerSpecies))))
    learnCodex(codexId, true);
}

CodexConstPtr PlayerCodexes::firstNewCodex() const {
  for (auto pair : m_codexes) {
    if (!pair.second.second)
      return pair.second.first;
  }
  return {};
}

}
