#include "StarSongbookInterface.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarListWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"

namespace Star {

SongbookInterface::SongbookInterface(PlayerPtr player) {
  m_player = std::move(player);

  auto assets = Root::singleton().assets();

  GuiReader reader;

  reader.registerCallback("close", [=](Widget*) { dismiss(); });
  reader.registerCallback("btnPlay",
      [=](Widget*) {
        if (play())
          dismiss();
      });
  reader.registerCallback("group", [=](Widget*) {});

  reader.construct(assets->json("/interface/windowconfig/songbook.config:paneLayout"), this);

  auto songList = fetchChild<ListWidget>("songs.list");

  StringList files = assets->scan(".abc");
  sort(files, [](String const& a, String const& b) -> bool { return b.compare(a, String::CaseInsensitive) > 0; });
  for (auto s : files) {
    auto song = s.substr(7, s.length() - (7 + 4));
    auto widget = songList->addItem();
    widget->setData(s);
    auto songName = widget->fetchChild<LabelWidget>("songName");
    songName->setText(song);

    widget->show();
  }
}

bool SongbookInterface::play() {
  auto songList = fetchChild<ListWidget>("songs.list");
  auto songWidget = songList->selectedWidget();
  if (!songWidget)
    return false;
  auto songName = songWidget->data().toString();
  auto group = fetchChild<TextBoxWidget>("group")->getText();

  JsonObject song;
  song["resource"] = songName;
  auto buffer = Root::singleton().assets()->bytes(songName);
  song["abc"] = String(buffer->ptr(), buffer->size());

  m_player->songbook()->play(song, group);
  return true;
}

}
