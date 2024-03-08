#pragma once

#include "StarSongbook.hpp"
#include "StarPane.hpp"

namespace Star {

STAR_CLASS(Player);

STAR_CLASS(SongbookInterface);

class SongbookInterface : public Pane {
public:
  SongbookInterface(PlayerPtr player);

  void update(float dt) override;

private:
  PlayerPtr m_player;
  StringList m_files;
  String m_searchValue;
  bool play();
};

}
