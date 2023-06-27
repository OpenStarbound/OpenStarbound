#include "StarContainerInterface.hpp"
#include "StarCasting.hpp"
#include "StarContainerEntity.hpp"
#include "StarWorldClient.hpp"
#include "StarRoot.hpp"
#include "StarItemTooltip.hpp"
#include "StarItemGridWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarPaneManager.hpp"
#include "StarFuelWidget.hpp"
#include "StarPlayer.hpp"
#include "StarItemDatabase.hpp"
#include "StarObject.hpp"
#include "StarPlayerInventory.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarAugmentItem.hpp"

namespace Star {

ContainerPane::ContainerPane(WorldClientPtr worldClient, PlayerPtr player, ContainerInteractorPtr containerInteractor) {
  m_worldClient = worldClient;
  m_player = player;
  m_containerInteractor = move(containerInteractor);

  auto container = m_containerInteractor->openContainer();
  auto guiConfig = container->containerGuiConfig();

  if (auto scripts = guiConfig.opt("scripts").apply(jsonToStringList)) {
    if (!m_script) {
      m_script.emplace();
      m_script->setScripts(*scripts);
    }
    m_script->addCallbacks("widget", LuaBindings::makeWidgetCallbacks(this, &m_reader));
    m_script->addCallbacks("config", LuaBindings::makeConfigCallbacks( [guiConfig](String const& name, Json const& def) {
        return guiConfig.query(name, def);
      }));
    m_script->addCallbacks("player", LuaBindings::makePlayerCallbacks(m_player.get()));
    m_script->addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_player->statusController()));

    LuaCallbacks containerPaneCallbacks;
    containerPaneCallbacks.registerCallback("containerEntityId", [this]() -> Maybe<EntityId> {
        return m_containerInteractor->openContainerId();
      });
    containerPaneCallbacks.registerCallback("playerEntityId", [this]() { return m_player->entityId(); });
    containerPaneCallbacks.registerCallback("dismiss", [this]() { dismiss(); });
    m_script->addCallbacks("pane", containerPaneCallbacks);

    m_script->setUpdateDelta(guiConfig.getUInt("scriptDelta", 1));
  }

  auto rightClickCallback = [this](size_t index) {
    if (m_expectingSwap != ExpectingSwap::None)
      return;

    if (ItemPtr slotItem = m_containerInteractor->openContainer()->itemBag()->at(index)) {
      auto swapItem = m_player->inventory()->swapSlotItem();
      if (!swapItem || swapItem->empty() || swapItem->couldStack(slotItem)) {
        size_t count = swapItem ? swapItem->couldStack(slotItem) : slotItem->maxStack();
        if (context()->shiftHeld())
          count = max(1, min<int>(count, slotItem->count() / 2));
        else
          count = 1;

        m_containerInteractor->takeFromContainerSlot(index, count);
        m_expectingSwap = ExpectingSwap::SwapSlotStack;
      } else if (is<AugmentItem>(swapItem)) {
        m_containerInteractor->applyAugmentInContainer(index, swapItem);
        m_player->inventory()->setSwapSlotItem({});
        m_expectingSwap = ExpectingSwap::SwapSlot;
      }
    }
  };

  m_reader.registerCallback("close", [this](Widget*) { dismiss(); });
  m_reader.registerCallback("itemGrid", [=](Widget* paneObj) {
      if (auto itemGrid = as<ItemGridWidget>(paneObj))
        swapSlot(itemGrid);
      else
        throw GuiException("Invalid object type, expected ItemGridWidget.");
    });
  m_reader.registerCallback("itemGrid.right", [=](Widget* paneObj) {
      if (auto itemGrid = as<ItemGridWidget>(paneObj))
        rightClickCallback(itemGrid->selectedIndex());
      else
        throw GuiException("Invalid object type, expected ItemGridWidget.");
    });

  m_reader.registerCallback("itemGrid2", [=](Widget* paneObj) {
      if (auto itemGrid = as<ItemGridWidget>(paneObj))
        swapSlot(itemGrid);
      else
        throw GuiException("Invalid object type, expected ItemGridWidget.");
    });
  m_reader.registerCallback("itemGrid2.right", [=](Widget* paneObj) {
      if (auto itemGrid = as<ItemGridWidget>(paneObj))
        rightClickCallback(itemGrid->selectedIndex());
      else
        throw GuiException("Invalid object type, expected ItemGridWidget.");
    });

  m_reader.registerCallback("outputItemGrid", [=](Widget* paneObj) {
      if (auto itemGrid = as<ItemGridWidget>(paneObj))
        swapSlot(itemGrid);
      else
        throw GuiException("Invalid object type, expected ItemGridWidget.");
    });
  m_reader.registerCallback("outputItemGrid.right", [=](Widget* paneObj) {
      if (auto itemGrid = as<ItemGridWidget>(paneObj))
        rightClickCallback(itemGrid->selectedIndex());
      else
        throw GuiException("Invalid object type, expected ItemGridWidget.");
    });

  m_reader.registerCallback("toggleCrafting", [=](Widget*) { toggleCrafting(); });

  m_reader.registerCallback("clear", [=](Widget*) { clear(); });
  m_reader.registerCallback("burn", [=](Widget*) { burn(); });

  for (auto const& callbackName : jsonToStringList(guiConfig.get("scriptWidgetCallbacks", JsonArray{}))) {
    m_reader.registerCallback(callbackName, [this, callbackName](Widget* widget) {
      m_script->invoke(callbackName, widget->name(), widget->data());
    });
  }

  m_reader.construct(guiConfig.get("gui"), this);

  if (auto countWidget = fetchChild<LabelWidget>("count"))
    countWidget->setText(countWidget->text().replace("<slots>", strf("{}", container->containerSize())));

  m_itemBag = make_shared<ItemBag>(container->containerSize());
  auto items = container->containerItems();

  fetchChild<ItemGridWidget>("itemGrid")->setItemBag(m_itemBag);
  if (containsChild("itemGrid2"))
    fetchChild<ItemGridWidget>("itemGrid2")->setItemBag(m_itemBag);
  if (containsChild("outputItemGrid"))
    fetchChild<ItemGridWidget>("outputItemGrid")->setItemBag(m_itemBag);

  if (container->iconItem()) {
    auto itemDatabase = Root::singleton().itemDatabase();
    auto iconItem = itemDatabase->item(container->iconItem());
    auto icon = make_shared<ItemSlotWidget>(iconItem, "/interface/inventory/portrait.png");
    icon->showDurability(false);
    icon->showRarity(false);
    icon->setBackingImageAffinity(true, true);
    setTitle(icon, container->containerDescription(), container->containerSubTitle());
  }

  if (containsChild("objectImage"))
    if (auto containerObject = as<Object>(m_containerInteractor->openContainer()))
      fetchChild<ImageWidget>("objectImage")->setDrawables(containerObject->cursorHintDrawables());

  m_expectingSwap = ExpectingSwap::None;
}

void ContainerPane::displayed() {
  Pane::displayed();

  m_expectingSwap = ExpectingSwap::None;

  if (m_script) {
    if (m_worldClient && m_worldClient->inWorld())
      m_script->init(m_worldClient.get());

    m_script->invoke("displayed");
  }
}

void ContainerPane::dismissed() {
  Pane::dismissed();

  if (m_script) {
    m_script->invoke("dismissed");
    m_script->uninit();
  }
}

bool ContainerPane::giveContainerResult(ContainerResult result) {
  if (m_expectingSwap == ExpectingSwap::None)
    return false;

  for (auto item : result) {
    auto inv = m_player->inventory();
    m_player->triggerPickupEvents(item);

    if (m_expectingSwap == ExpectingSwap::SwapSlot) {
      m_player->clearSwap();
      inv->setSwapSlotItem(item);
    } else if (m_expectingSwap == ExpectingSwap::SwapSlotStack) {
      auto swapItem = inv->swapSlotItem();
      if (swapItem && swapItem->stackWith(item)) {
        continue;
      } else {
        inv->clearSwap();
        inv->setSwapSlotItem(item);
      }
    } else {
      m_containerInteractor->addToContainer(inv->addItems(item));
    }
  }

  m_expectingSwap = ExpectingSwap::None;
  return true;
}

PanePtr ContainerPane::createTooltip(Vec2I const& screenPosition) {
  ItemPtr item;
  if (auto child = getChildAt(screenPosition)) {
    if (auto itemSlot = as<ItemSlotWidget>(child))
      item = itemSlot->item();
    if (auto itemGrid = as<ItemGridWidget>(child))
      item = itemGrid->itemAt(screenPosition);
  }
  if (item)
    return ItemTooltipBuilder::buildItemTooltip(item, m_player);
  return {};
}

void ContainerPane::swapSlot(ItemGridWidget* grid) {
  auto inv = m_player->inventory();
  if (context()->shiftHeld()) {
    auto containerItem = grid->selectedItem();
    if (containerItem && inv->itemsCanFit(containerItem) >= containerItem->count()) {
      m_containerInteractor->swapInContainer(grid->selectedIndex(), {});
      m_expectingSwap = ExpectingSwap::Inventory;
    }
  } else {
    m_containerInteractor->swapInContainer(grid->selectedIndex(), inv->swapSlotItem());
    inv->setSwapSlotItem({});
    m_expectingSwap = ExpectingSwap::SwapSlot;
  }
}

void ContainerPane::startCrafting() {
  m_containerInteractor->startCraftingInContainer();
}

void ContainerPane::stopCrafting() {
  m_containerInteractor->stopCraftingInContainer();
}

void ContainerPane::toggleCrafting() {
  if (m_containerInteractor->openContainer()->isCrafting())
    stopCrafting();
  else
    startCrafting();
}

void ContainerPane::clear() {
  m_containerInteractor->clearContainer();
}

void ContainerPane::burn() {
  m_containerInteractor->burnContainer();
}

void ContainerPane::update() {
  Pane::update();

  if (m_script)
    m_script->update(m_script->updateDt());

  m_itemBag->clearItems();

  if (!m_containerInteractor->containerOpen()) {
    dismiss();

  } else {
    auto container = m_containerInteractor->openContainer();

    for (size_t i = 0; i < m_itemBag->size(); ++i)
      m_itemBag->putItems(i, container->containerItems()[i]);

    if (container->isInteractive()) {
      if (auto itemGrid = fetchChild<ItemGridWidget>("itemGrid")) {
        itemGrid->setProgress(container->craftingProgress());
        itemGrid->updateAllItemSlots();
      }
      if (auto itemGrid = fetchChild<ItemGridWidget>("itemGrid2")) {
        itemGrid->setProgress(container->craftingProgress());
        itemGrid->updateAllItemSlots();
      }

      if (auto fuelGauge = fetchChild<FuelWidget>("fuelGauge")) {
        fuelGauge->setCurrentFuelLevel(m_worldClient->getProperty("ship.fuel", 0).toUInt());
        fuelGauge->setMaxFuelLevel(m_worldClient->getProperty("ship.maxFuel", 0).toUInt());
        float totalFuelAmount = 0;
        for (auto& item : container->containerItems()) {
          if (item)
            totalFuelAmount += item->instanceValue("fuelAmount", 0).toUInt() * item->count();
        }
        fuelGauge->setPotentialFuelAmount(totalFuelAmount);
        fuelGauge->setRequestedFuelAmount(0);
      }
    }
  }
}

}
