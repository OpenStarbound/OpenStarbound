#include "StarInventory.hpp"
#include "StarGuiReader.hpp"
#include "StarItemTooltip.hpp"
#include "StarSimpleTooltip.hpp"
#include "StarRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarItemGridWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarPortraitWidget.hpp"
#include "StarPaneManager.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerCompanions.hpp"
#include "StarWorldClient.hpp"
#include "StarAssets.hpp"
#include "StarItem.hpp"
#include "StarMainInterface.hpp"
#include "StarMerchantInterface.hpp"
#include "StarJsonExtra.hpp"
#include "StarStatistics.hpp"
#include "StarAugmentItem.hpp"

namespace Star {

InventoryPane::InventoryPane(MainInterface* parent, PlayerPtr player, ContainerInteractorPtr containerInteractor) {
  m_parent = parent;
  m_player = move(player);
  m_containerInteractor = move(containerInteractor);

  GuiReader invWindowReader;
  auto config = Root::singleton().assets()->json("/interface/windowconfig/playerinventory.config");

  auto leftClickCallback = [this](String const& bagType, Widget* widget) {
    auto itemGrid = convert<ItemGridWidget>(widget);
    InventorySlot inventorySlot = BagSlot(bagType, itemGrid->selectedIndex());

    if (context()->shiftHeld()) {
      if (auto sourceItem = itemGrid->selectedItem()) {
        if (auto activeMerchantPane = m_parent->activeMerchantPane()) {
          auto remainder = activeMerchantPane->addItems(m_player->inventory()->takeSlot(inventorySlot));
          if (remainder && !remainder->empty())
            m_player->inventory()->setItem(inventorySlot, remainder);
        } else if (m_containerInteractor->containerOpen()) {
          m_player->inventory()->takeSlot(inventorySlot);
          m_containerInteractor->addToContainer(sourceItem);
          m_containerSource = inventorySlot;
          m_expectingSwap = true;
        }
      }
    } else {
      m_player->inventory()->shiftSwap(inventorySlot);
    }
  };

  auto rightClickCallback = [this](InventorySlot slot) {
    if (ItemPtr slotItem = m_player->inventory()->itemsAt(slot)) {
      auto swapItem = m_player->inventory()->swapSlotItem();
      if (!swapItem || swapItem->empty() || swapItem->couldStack(slotItem)) {
        uint64_t count = swapItem ? swapItem->couldStack(slotItem) : slotItem->maxStack();
        if (context()->shiftHeld())
          count = max(1, min<int>(count, slotItem->count() / 2));
        else
          count = 1;

        if (auto taken = slotItem->take(count)) {
          if (swapItem)
            swapItem->stackWith(taken);
          else
            m_player->inventory()->setSwapSlotItem(taken);
        }
      } else if (auto augment = as<AugmentItem>(swapItem)) {
        if (auto augmented = augment->applyTo(slotItem))
          m_player->inventory()->setItem(slot, augmented);
      }
    }
  };
  auto bagGridCallback = [rightClickCallback](String const& bagType, Widget* widget) {
    auto slot = BagSlot(bagType, convert<ItemGridWidget>(widget)->selectedIndex());
    rightClickCallback(slot);
  };

  Json itemBagConfig = config.get("bagConfig");
  auto bagOrder = itemBagConfig.toObject().keys().sorted([&itemBagConfig](String const& a, String const& b) {
    return itemBagConfig.get(a).getInt("order", 0) < itemBagConfig.get(b).getInt("order", 0);
  });
  for (auto name : bagOrder) {
    auto itemGrid = itemBagConfig.get(name).getString("itemGrid");
    invWindowReader.registerCallback(itemGrid, bind(leftClickCallback, name, _1));
    invWindowReader.registerCallback(strf("{}.right", itemGrid), bind(bagGridCallback, name, _1));
  }

  invWindowReader.registerCallback("close", [=](Widget*) {
      dismiss();
    });

  invWindowReader.registerCallback("sort", [=](Widget*) {
      m_player->inventory()->condenseBagStacks(m_selectedTab);
      m_player->inventory()->sortBag(m_selectedTab);
      // Don't show sorted items as new items
      m_itemGrids[m_selectedTab]->updateItemState();
      m_itemGrids[m_selectedTab]->clearChangedSlots();
    });

  invWindowReader.registerCallback("gridModeSelector", [=](Widget* widget) {
      auto selected = convert<ButtonWidget>(widget)->data().toString();
      selectTab(m_tabButtonData.keyOf(selected));
    });

  auto registerSlotCallbacks = [&](String name, InventorySlot slot) {
    invWindowReader.registerCallback(name, [=](Widget* paneObj) {
        if (as<ItemSlotWidget>(paneObj))
          m_player->inventory()->shiftSwap(slot);
        else
          throw GuiException("Invalid object type, expected ItemSlotWidget");
      });
    invWindowReader.registerCallback(name + ".right", [=](Widget* paneObj) {
        if (as<ItemSlotWidget>(paneObj))
          rightClickCallback(slot);
        else
          throw GuiException("Invalid object type, expected ItemSlotWidget");
      });
  };

  for (auto const p : EquipmentSlotNames)
    registerSlotCallbacks(p.second, p.first);
  registerSlotCallbacks("trash", TrashSlot());

  invWindowReader.construct(config.get("paneLayout"), this);

  m_trashSlot = fetchChild<ItemSlotWidget>("trash");
  m_trashBurn = GameTimer(config.get("trashBurnTimeout").toFloat());

  m_disabledTechOverlays.append(fetchChild<ImageWidget>("techHeadDisabled"));
  m_disabledTechOverlays.append(fetchChild<ImageWidget>("techBodyDisabled"));
  m_disabledTechOverlays.append(fetchChild<ImageWidget>("techLegsDisabled"));

  for (auto const p : EquipmentSlotNames) {
    if (auto itemSlot = fetchChild<ItemSlotWidget>(p.second))
      itemSlot->setItem(m_player->inventory()->itemsAt(p.first));
  }

  for (auto name : bagOrder) {
    auto itemTab = itemBagConfig.get(name);
    m_itemGrids[name] = fetchChild<ItemGridWidget>(itemTab.getString("itemGrid"));
    m_itemGrids[name]->setItemBag(m_player->inventory()->bagContents(name));
    m_itemGrids[name]->hide();
    m_newItemMarkers[name] = fetchChild<Widget>(itemTab.getString("newItemMarker"));
    m_tabButtonData[name] = itemTab.getString("tabButtonData");
  }
  selectTab(bagOrder[0]);

  auto centralPortrait = fetchChild<PortraitWidget>("portrait");
  centralPortrait->setEntity(m_player);

  auto portrait = make_shared<PortraitWidget>(m_player, PortraitMode::Bust);
  portrait->setIconMode();
  setTitle(portrait, m_player->name(), config.getString("subtitle"));

  m_expectingSwap = false;

  if (auto item = m_player->inventory()->swapSlotItem())
    m_currentSwapSlotItem = item->descriptor();
  m_pickUpSounds = jsonToStringList(config.get("sounds").get("pickup"));
  m_putDownSounds = jsonToStringList(config.get("sounds").get("putdown"));
}

void InventoryPane::displayed() {
  Pane::displayed();
  m_expectingSwap = false;

  for (auto grid : m_itemGrids)
    grid.second->updateItemState();
  m_itemGrids[m_selectedTab]->indicateChangedSlots();
}

PanePtr InventoryPane::createTooltip(Vec2I const& screenPosition) {
  ItemPtr item;
  if (auto child = getChildAt(screenPosition)) {
    if (auto itemSlot = as<ItemSlotWidget>(child)) {
      item = itemSlot->item();
      if (!item) {
        auto widgetData = itemSlot->data();
        if (widgetData && widgetData.type() == Json::Type::Object) {
          if (auto text = widgetData.optString("tooltipText"))
            return SimpleTooltipBuilder::buildTooltip(*text);
        }
      }
    }
    if (auto itemGrid = as<ItemGridWidget>(child))
      item = itemGrid->itemAt(screenPosition);
  }
  if (item)
    return ItemTooltipBuilder::buildItemTooltip(item, m_player);

  auto techDatabase = Root::singleton().techDatabase();
  for (auto const& p : TechTypeNames) {
    if (auto techIcon = fetchChild<ImageWidget>(strf("tech{}", p.second))) {
      if (techIcon->screenBoundRect().contains(screenPosition)) {
        if (auto techModule = m_player->techs()->equippedTechs().maybe(p.first))
          return SimpleTooltipBuilder::buildTooltip(techDatabase->tech(*techModule).description);
      }
    }
  }

  return {};
}

bool InventoryPane::giveContainerResult(ContainerResult result) {
  if (!m_expectingSwap)
    return false;

  for (auto item : result) {
    auto inv = m_player->inventory();
    m_player->triggerPickupEvents(item);

    auto remainder = inv->stackWith(m_containerSource, item);
    if (remainder && !remainder->empty())
      m_player->giveItem(remainder);
  }

  m_expectingSwap = false;
  return true;
}

void InventoryPane::updateItems() {
  for (auto p : m_itemGrids)
    p.second->updateItemState();
}

bool InventoryPane::containsNewItems() const {
  for (auto p : m_itemGrids) {
    if (p.second->slotsChanged())
      return true;
  }
  return false;
}

void InventoryPane::update() {
  auto inventory = m_player->inventory();
  auto context = Widget::context();

  HashSet<ItemPtr> customBarItems;
  for (uint8_t i = 0; i < inventory->customBarIndexes(); ++i) {
    if (auto primarySlot = inventory->customBarPrimarySlot(i)) {
      if (auto primaryItem = inventory->itemsAt(*primarySlot))
        customBarItems.add(primaryItem);
    }
    if (auto secondarySlot = inventory->customBarSecondarySlot(i)) {
      if (auto secondaryItem = inventory->itemsAt(*secondarySlot))
        customBarItems.add(secondaryItem);
    }
  }

  m_trashSlot->setItem(inventory->itemsAt(TrashSlot()));
  m_trashSlot->showLinkIndicator(customBarItems.contains(m_trashSlot->item()));
  if (auto trashItem = m_trashSlot->item()) {
    if (m_trashBurn.tick() && trashItem->count() > 0) {
      m_player->statistics()->recordEvent("trashItem", JsonObject{
          {"itemName", trashItem->name()},
          {"count", trashItem->count()},
          {"category", trashItem->category()}
        });
      trashItem->take(trashItem->count());
    }
  } else {
    m_trashBurn.reset();
  }
  m_trashSlot->setProgress(m_trashBurn.timer / m_trashBurn.time);

  for (auto const& p : EquipmentSlotNames) {
    if (auto itemSlot = fetchChild<ItemSlotWidget>(p.second)) {
      itemSlot->setItem(inventory->itemsAt(p.first));
      itemSlot->showLinkIndicator(customBarItems.contains(itemSlot->item()));
    }
  }

  auto techDatabase = Root::singleton().techDatabase();
  for (auto const& p : TechTypeNames) {
    if (auto techIcon = fetchChild<ImageWidget>(strf("tech{}", p.second))) {
      if (auto techModule = m_player->techs()->equippedTechs().maybe(p.first))
        techIcon->setImage(techDatabase->tech(*techModule).icon);
      else
        techIcon->setImage("");
    }
  }

  if (ItemPtr swapSlot = inventory->swapSlotItem()) {
    for (auto pair : m_itemGrids) {
      if (pair.first != m_selectedTab && PlayerInventory::itemAllowedInBag(swapSlot, pair.first)) {
        selectTab(pair.first);
        break;
      }
    }
  }

  for (auto p : m_itemGrids) {
    p.second->updateItemState();
    for (size_t i = 0; i < p.second->itemSlots(); ++i) {
      auto itemWidget = p.second->itemWidgetAt(i);
      itemWidget->showLinkIndicator(customBarItems.contains(itemWidget->item()));
    }
  }

  m_itemGrids[m_selectedTab]->clearChangedSlots();

  for (auto pair : m_newItemMarkers) {
    if (m_itemGrids[pair.first]->slotsChanged())
      pair.second->show();
    else
      pair.second->hide();
  }

  for (auto techOverlay : m_disabledTechOverlays)
    techOverlay->setVisibility(m_player->techOverridden());

  auto healthLabel = fetchChild<LabelWidget>("healthtext");
  healthLabel->setText(strf("{}", m_player->maxHealth()));
  auto energyLabel = fetchChild<LabelWidget>("energytext");
  energyLabel->setText(strf("{}", m_player->maxEnergy()));
  auto weaponLabel = fetchChild<LabelWidget>("weapontext");
  weaponLabel->setText(strf("{}%", ceil(m_player->powerMultiplier() * 100)));
  auto defenseLabel = fetchChild<LabelWidget>("defensetext");
  if (m_player->protection() == 0)
    defenseLabel->setText("--");
  else
    defenseLabel->setText(strf("{}", ceil(m_player->protection())));

  auto moneyLabel = fetchChild<LabelWidget>("lblMoney");
  moneyLabel->setText(strf("{}", m_player->currency("money")));

  if (m_player->currency("essence") > 0) {
    fetchChild<ImageWidget>("imgEssenceIcon")->show();
    auto essenceLabel = fetchChild<LabelWidget>("lblEssence");
    essenceLabel->show();
    essenceLabel->setText(strf("{}", m_player->currency("essence")));
  } else {
    fetchChild<ImageWidget>("imgEssenceIcon")->hide();
    fetchChild<LabelWidget>("lblEssence")->hide();
  }

  auto config = Root::singleton().assets()->json("/interface/windowconfig/playerinventory.config");

  auto pets = m_player->companions()->getCompanions("pets");
  if (pets.size() > 0) {
    auto pet = pets.first();
    auto companionImage = fetchChild<ImageWidget>("companionSlot");
    companionImage->setVisibility(true);

    companionImage->setDrawables(pet->portrait());
    auto nameLabel = fetchChild<LabelWidget>("companionName");
    if (auto name = pet->name()) {
      nameLabel->setText(pet->name()->toUpper());
    } else {
      nameLabel->setText(config.getString("defaultPetNameLabel"));
    }

    auto attackLabel = fetchChild<LabelWidget>("companionAttackStat");
    if (auto attack = pet->stat("attack")) {
      attackLabel->setText(strf("{:.0f}", *attack));
    } else {
      attackLabel->setText("");
    }

    auto defenseLabel = fetchChild<LabelWidget>("companionDefenseStat");
    if (auto defense = pet->stat("defense")) {
      defenseLabel->setText(strf("{:.0f}", *defense));
    } else {
      defenseLabel->setText("");
    }

    if (containsChild("companionHealthBar")) {
      auto healthBar = fetchChild<ProgressWidget>("companionHealthBar");
      Maybe<float> health = pet->resource("health");
      Maybe<float> healthMax = pet->resourceMax("health");
      if (health && healthMax) {
        healthBar->setCurrentProgressLevel(*health);
        healthBar->setMaxProgressLevel(*healthMax);
      } else {
        healthBar->setCurrentProgressLevel(0);
        healthBar->setMaxProgressLevel(1);
      }
    }
  } else {
    fetchChild<ImageWidget>("companionSlot")->setVisibility(false);

    fetchChild<LabelWidget>("companionName")->setText(config.getString("defaultPetNameLabel"));

    fetchChild<LabelWidget>("companionAttackStat")->setText("");
    fetchChild<LabelWidget>("companionDefenseStat")->setText("");

    if (containsChild("companionHealthBar")) {
      auto healthBar = fetchChild<ProgressWidget>("companionHealthBar");
      healthBar->setCurrentProgressLevel(0);
      healthBar->setMaxProgressLevel(1);
    }
  }

  if (auto item = inventory->swapSlotItem()) {
    if (!m_currentSwapSlotItem || !item->matches(*m_currentSwapSlotItem, true) || item->count() > m_currentSwapSlotItem->count())
      context->playAudio(RandomSource().randFrom(m_pickUpSounds));
    else if (item->count() < m_currentSwapSlotItem->count())
      context->playAudio(RandomSource().randFrom(m_putDownSounds));

    m_currentSwapSlotItem = item->descriptor();
  } else {
    if (m_currentSwapSlotItem)
      context->playAudio(RandomSource().randFrom(m_putDownSounds));
    m_currentSwapSlotItem = {};
  }
}

void InventoryPane::selectTab(String const& selected) {
  for (auto grid : m_itemGrids)
    grid.second->hide();
  m_selectedTab = selected;
  m_itemGrids[m_selectedTab]->show();
  m_itemGrids[m_selectedTab]->indicateChangedSlots();

  auto tabs = fetchChild<ButtonGroupWidget>("gridModeSelector");
  for (auto button : tabs->buttons())
    if (button->data().toString().equalsIgnoreCase(m_tabButtonData[selected]))
      tabs->select(tabs->id(button));
}

}
