#pragma once

#include "StarPlayerUniverseMap.hpp"
#include "StarPane.hpp"

namespace Star {

STAR_CLASS(EditBookmarkDialog);

class EditBookmarkDialog : public Pane {
public:
  EditBookmarkDialog(PlayerUniverseMapPtr playerUniverseMap);

  virtual void show() override;

  void setBookmark(TeleportBookmark bookmark);

  void ok();
  void remove();
  void close();

private:
  PlayerUniverseMapPtr m_playerUniverseMap;
  TeleportBookmark m_bookmark;

  bool m_isNew;
};

void setupBookmarkEntry(WidgetPtr const& entry, TeleportBookmark const& bookmark);
}
