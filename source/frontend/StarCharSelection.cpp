#include "StarCharSelection.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarLargeCharPlateWidget.hpp"
#include "StarAssets.hpp"
#include "StarRandom.hpp"
#include "StarInputEvent.hpp"

namespace Star {

CharSelectionPane::CharSelectionPane(PlayerStoragePtr playerStorage,
    CreateCharCallback createCallback,
    SelectCharacterCallback selectCallback,
    DeleteCharacterCallback deleteCallback)
  : m_playerStorage(playerStorage),
    m_downScroll(0),
    m_createCallback(createCallback),
    m_selectCallback(selectCallback),
    m_deleteCallback(deleteCallback) {
  auto& root = Root::singleton();

  GuiReader guiReader;

  guiReader.registerCallback("playerUpButton", [=](Widget*) { shiftCharacters(-1); });
  guiReader.registerCallback("playerDownButton", [=](Widget*) { shiftCharacters(1); });
  guiReader.registerCallback("charSelector1", [=](Widget*) { selectCharacter(0); });
  guiReader.registerCallback("charSelector2", [=](Widget*) { selectCharacter(1); });
  guiReader.registerCallback("charSelector3", [=](Widget*) { selectCharacter(2); });
  guiReader.registerCallback("charSelector4", [=](Widget*) { selectCharacter(3); });
  guiReader.registerCallback("createCharButton", [=](Widget*) { m_createCallback(); });

  guiReader.construct(root.assets()->json("/interface/windowconfig/charselection.config"), this);
}

bool CharSelectionPane::sendEvent(InputEvent const& event) {
  if (m_visible) {
    if (auto mouseWheel = event.ptr<MouseWheelEvent>()) {
      if (inMember(*context()->mousePosition(event))) {
        if (mouseWheel->mouseWheel == MouseWheel::Down)
          shiftCharacters(1);
        else if (mouseWheel->mouseWheel == MouseWheel::Up)
          shiftCharacters(-1);
        return true;
      }
    }
  }
  return Pane::sendEvent(event);
}

void CharSelectionPane::show() {
  Pane::show();

  m_downScroll = 0;
  updateCharacterPlates();
}

void CharSelectionPane::shiftCharacters(int shift) {
  m_downScroll = std::max<int>(std::min<int>(m_downScroll + shift, m_playerStorage->playerCount() - 3), 0);
  updateCharacterPlates();
}

void CharSelectionPane::selectCharacter(unsigned buttonIndex) {
  if (auto playerUuid = m_playerStorage->playerUuidAt(m_downScroll + buttonIndex)) {
    auto player = m_playerStorage->loadPlayer(*playerUuid);
    if (player->isPermaDead() && !player->isAdmin()) {
      auto sound = Random::randValueFrom(
                       Root::singleton().assets()->json("/interface.config:buttonClickFailSound").toArray(), "")
                       .toString();
      if (!sound.empty())
        context()->playAudio(sound);
    } else
      m_selectCallback(player);
  } else
    m_createCallback();
}

void CharSelectionPane::updateCharacterPlates() {
  auto updatePlayerLine = [this](String name, unsigned scrollPosition) {
    auto charSelector = fetchChild<LargeCharPlateWidget>(name);
    if (auto playerUuid = m_playerStorage->playerUuidAt(scrollPosition)) {
      if (auto player = m_playerStorage->loadPlayer(*playerUuid)) {
        player->humanoid()->setFacingDirection(Direction::Right);
        charSelector->setPlayer(player);
        charSelector->enableDelete([this, playerUuid](Widget*) { m_deleteCallback(*playerUuid); });
        return;
      }
    }
    charSelector->setPlayer(PlayerPtr());
    charSelector->disableDelete();
  };

  updatePlayerLine("charSelector1", m_downScroll + 0);
  updatePlayerLine("charSelector2", m_downScroll + 1);
  updatePlayerLine("charSelector3", m_downScroll + 2);
  updatePlayerLine("charSelector4", m_downScroll + 3);

  if (m_downScroll > 0)
    fetchChild("playerUpButton")->show();
  else
    fetchChild("playerUpButton")->hide();

  if (m_downScroll < m_playerStorage->playerCount() - 3)
    fetchChild("playerDownButton")->show();
  else
    fetchChild("playerDownButton")->hide();
}

}
