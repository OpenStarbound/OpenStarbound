#include "StarCodexItem.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"
#include "StarClientContext.hpp"
#include "StarCodex.hpp"

namespace Star {

CodexItem::CodexItem(Json const& config, String const& directory, Json const& data)
  : Item(config, directory, data), SwingableItem(config) {
  setWindupTime(0.2f);
  setCooldownTime(0.5f);
  m_requireEdgeTrigger = true;
  m_codexId = instanceValue("codexId").toString();
  String iconPath = instanceValue("codexIcon").toString();
  m_iconDrawables = {Drawable::makeImage(iconPath, 1.0f, true, Vec2F())};
  m_worldDrawables = {Drawable::makeImage(iconPath, 1.0f / TilePixels, true, Vec2F())};
}

ItemPtr CodexItem::clone() const {
  return make_shared<CodexItem>(*this);
}

List<Drawable> CodexItem::drawables() const {
  return m_worldDrawables;
}

void CodexItem::fireTriggered() {
  if (auto player = as<Player>(owner())) {
    auto codexLearned = player->codexes()->learnCodex(m_codexId);
    if (codexLearned) {
      player->queueUIMessage(Root::singleton().assets()->json("/codex.config:messages.learned").toString());
    } else {
      player->queueUIMessage(Root::singleton().assets()->json("/codex.config:messages.alreadyKnown").toString());
    }
  }
}

List<Drawable> CodexItem::iconDrawables() const {
  return m_iconDrawables;
}

List<Drawable> CodexItem::dropDrawables() const {
  return m_worldDrawables;
}

}
