#include "StarBookmarkInterface.hpp"
#include "StarGuiReader.hpp"
#include "StarButtonWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

EditBookmarkDialog::EditBookmarkDialog(PlayerUniverseMapPtr playerUniverseMap) {
  m_playerUniverseMap = playerUniverseMap;

  GuiReader reader;
  auto assets = Root::singleton().assets();
  reader.registerCallback("ok", [this](Widget*) { ok(); });
  reader.registerCallback("remove", [this](Widget*) { remove(); });
  reader.registerCallback("close", [this](Widget*) { close(); });
  reader.registerCallback("name", [](Widget*) {});
  reader.construct(assets->json("/interface/windowconfig/editbookmark.config:paneLayout"), this);
  dismiss();
}

void EditBookmarkDialog::setBookmark(TeleportBookmark bookmark) {
  m_bookmark = bookmark;

  m_isNew = true;
  for (auto& existing : m_playerUniverseMap->teleportBookmarks()) {
    if (existing == bookmark) {
      m_bookmark.bookmarkName = existing.bookmarkName;
      m_isNew = false;
    }
  }
}

void EditBookmarkDialog::show() {
  Pane::show();

  if (m_isNew) {
    fetchChild<LabelWidget>("lblTitle")->setText("NEW BOOKMARK");
    fetchChild<ButtonWidget>("remove")->hide();
  } else {
    fetchChild<LabelWidget>("lblTitle")->setText("EDIT BOOKMARK");
    fetchChild<ButtonWidget>("remove")->show();
  }

  auto assets = Root::singleton().assets();
  fetchChild<ImageWidget>("imgIcon")->setImage(strf("/interface/bookmarks/icons/%s.png", m_bookmark.icon));

  fetchChild<LabelWidget>("lblPlanetName")->setText(m_bookmark.targetName);
  fetchChild<TextBoxWidget>("name")->setText(m_bookmark.bookmarkName, false);
  fetchChild<TextBoxWidget>("name")->focus();
}

void EditBookmarkDialog::ok() {
  m_bookmark.bookmarkName = fetchChild<TextBoxWidget>("name")->getText();
  if (m_bookmark.bookmarkName.empty())
    m_bookmark.bookmarkName = m_bookmark.targetName;
  if (!m_isNew)
    m_playerUniverseMap->removeTeleportBookmark(m_bookmark);
  m_playerUniverseMap->addTeleportBookmark(m_bookmark);
  dismiss();
}

void EditBookmarkDialog::remove() {
  m_playerUniverseMap->removeTeleportBookmark(m_bookmark);
  dismiss();
}

void EditBookmarkDialog::close() {
  dismiss();
}

void setupBookmarkEntry(WidgetPtr const& entry, TeleportBookmark const& bookmark) {
  entry->fetchChild<LabelWidget>("name")->setText(bookmark.bookmarkName);
  entry->fetchChild<LabelWidget>("planetName")->setText(bookmark.targetName);
  entry->fetchChild<ImageWidget>("icon")->setImage(strf("/interface/bookmarks/icons/%s.png", bookmark.icon));
}

}
