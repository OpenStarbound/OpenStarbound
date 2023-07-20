#ifndef STAR_CONTAINER_INTERFACE_HPP
#define STAR_CONTAINER_INTERFACE_HPP

#include "StarPane.hpp"
#include "StarLuaComponents.hpp"
#include "StarContainerInteractor.hpp"
#include "StarGuiReader.hpp"

namespace Star {

STAR_CLASS(ContainerEntity);
STAR_CLASS(Player);
STAR_CLASS(WorldClient);
STAR_CLASS(Item);
STAR_CLASS(ItemGridWidget);
STAR_CLASS(ItemBag);
STAR_CLASS(ContainerPane);

class ContainerPane : public Pane {
public:
  ContainerPane(WorldClientPtr worldClient, PlayerPtr player, ContainerInteractorPtr containerInteractor);

  void displayed() override;
  void dismissed() override;
  PanePtr createTooltip(Vec2I const& screenPosition) override;

  bool giveContainerResult(ContainerResult result);

protected:
  void update(float dt) override;

private:
  enum class ExpectingSwap {
    None,
    Inventory,
    SwapSlot,
    SwapSlotStack
  };

  void swapSlot(ItemGridWidget* grid);
  void startCrafting();
  void stopCrafting();
  void toggleCrafting();
  void clear();
  void burn();

  WorldClientPtr m_worldClient;
  PlayerPtr m_player;
  ContainerInteractorPtr m_containerInteractor;
  ItemBagPtr m_itemBag;

  ExpectingSwap m_expectingSwap;

  GuiReader m_reader;

  Maybe<LuaWorldComponent<LuaUpdatableComponent<LuaBaseComponent>>> m_script;
};

}

#endif
