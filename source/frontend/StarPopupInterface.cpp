#include "StarPopupInterface.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarLabelWidget.hpp"
#include "StarRandom.hpp"
#include "StarAssets.hpp"

namespace Star {

PopupInterface::PopupInterface() {
  auto assets = Root::singleton().assets();

  GuiReader reader;

  reader.registerCallback("close", [=](Widget*) { dismiss(); });
  reader.registerCallback("ok", [=](Widget*) { dismiss(); });

  reader.construct(assets->json("/interface/windowconfig/popup.config:paneLayout"), this);
}

void PopupInterface::displayMessage(String const& message, String const& title, String const& subtitle, Maybe<String> const& onShowSound) {
  setTitleString(title, subtitle);
  fetchChild<LabelWidget>("message")->setText(message);
  show();
  auto sound = onShowSound.value(Random::randValueFrom(Root::singleton().assets()->json("/interface/windowconfig/popup.config:onShowSound").toArray(), "").toString());
  if (!sound.empty())
    context()->playAudio(sound);
}

}
