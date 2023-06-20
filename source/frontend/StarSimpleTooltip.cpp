#include "StarSimpleTooltip.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarPane.hpp"

namespace Star {

PanePtr SimpleTooltipBuilder::buildTooltip(String const& text) {
  PanePtr tooltip = make_shared<Pane>();
  tooltip->removeAllChildren();
  GuiReader reader;
  reader.construct(Root::singleton().assets()->json("/interface/tooltips/simpletooltip.tooltip"), tooltip.get());
  tooltip->setLabel("contentLabel", text);

  auto stretchBackground = tooltip->fetchChild<Widget>("stretchBackground");
  stretchBackground->setSize(Vec2I{tooltip->fetchChild<Widget>("contentLabel")->size()[0] + 8, stretchBackground->size()[1]});
  tooltip->setSize(stretchBackground->size());

  return tooltip;
}

}
