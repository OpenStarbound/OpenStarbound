#ifndef STAR_SONGBOOK_INTERFACE_HPP
#define STAR_SONGBOOK_INTERFACE_HPP

#include "StarSongbook.hpp"
#include "StarPane.hpp"

namespace Star {

STAR_CLASS(Player);

STAR_CLASS(SongbookInterface);

class SongbookInterface : public Pane {
public:
  SongbookInterface(PlayerPtr player);

private:
  PlayerPtr m_player;
  bool play();
};

}

#endif
