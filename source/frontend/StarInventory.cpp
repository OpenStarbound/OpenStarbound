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
#include "StarObjectItem.hpp"
#include "StarInteractionTypes.hpp"

namespace Star {

InventoryPane::InventoryPane(MainInterface* parent, PlayerPtr player, ContainerInteractorPtr containerInteractor) {
  m_parent = parent;
  m_player = std::move(player);
  m_containerInteractor = std::move(containerInteractor);

  GuiReader invWindowReader;
  auto config = Root::singleton().assets()->json("/interface/windowconfig/playerinventory.config");

  auto leftClickCallback = [this](String const& bagType, Widget* widget) {
    auto itemGrid = convert<ItemGridWidget>(widget);
    InventorySlot inventorySlot = BagSlot(bagType, itemGrid->selectedIndex());

    auto inventory = m_player->inventory();
    if (context()->shiftHeld()) {
      if (auto sourceItem = itemGrid->selectedItem()) {
        if (auto activeMerchantPane = m_parent->activeMerchantPane()) {
          auto remainder = activeMerchantPane->addItems(inventory->takeSlot(inventorySlot));
          if (remainder && !remainder->empty())
            inventory->setItem(inventorySlot, remainder);
        } else if (m_containerInteractor->containerOpen()) {
          inventory->takeSlot(inventorySlot);
          m_containerInteractor->addToContainer(sourceItem);
          m_containerSource = inventorySlot;
          m_expectingSwap = true;
        } else {
          for (PanePtr& pane : m_parent->paneManager()->getAllPanes()) {
            auto remainder = pane->shiftItemFromInventory(inventory->itemsAt(inventorySlot));
            if (remainder.isValid()) {
              inventory->setItem(inventorySlot, remainder.value());
              break;
            }
          }
        }
      }
    } else {
      inventory->shiftSwap(inventorySlot);
    }
  };

  auto rightClickCallback = [this](InventorySlot slot) {
    auto inventory = m_player->inventory();
    if (ItemPtr slotItem = inventory->itemsAt(slot)) {
      auto swapItem = inventory->swapSlotItem();
      if (!swapItem || swapItem->empty() || swapItem->couldStack(slotItem)) {
        uint64_t count = swapItem ? swapItem->couldStack(slotItem) : slotItem->maxStack();
        if (context()->shiftHeld())
          count = max<uint64_t>(1, min<uint64_t>(count, slotItem->count() / 2));
        else
          count = 1;

        if (auto taken = slotItem->take(count)) {
          if (swapItem)
            swapItem->stackWith(taken);
          else
            inventory->setSwapSlotItem(taken);
        }
      } else if (auto augment = as<AugmentItem>(swapItem)) {
        if (auto augmented = augment->applyTo(slotItem))
          inventory->setItem(slot, augmented);
      }
    }
    else if (auto swapSlot = inventory->swapSlotItem()) {
      if (auto es = slot.ptr<EquipmentSlot>()) {
        if (inventory->itemAllowedAsEquipment(swapSlot, *es))
          inventory->setItem(slot, swapSlot->take(1));
      } else if (slot.is<TrashSlot>()) {
        inventory->setItem(slot, swapSlot->take(1));
      } else if (auto bs = slot.ptr<BagSlot>()) {
        if (inventory->itemAllowedInBag(swapSlot, bs->first))
          inventory->setItem(slot, swapSlot->take(1));
      }
    }
  };

  auto bagGridCallback = [rightClickCallback](String const& bagType, Widget* widget) {
    auto slot = BagSlot(bagType, convert<ItemGridWidget>(widget)->selectedIndex());
    rightClickCallback(slot);
  };

  auto middleClickCallback = [this](String const& bagType, Widget* widget) {
    if (!m_player->inWorld()) 
      return;

    auto itemGrid = convert<ItemGridWidget>(widget);
    InventorySlot inventorySlot = BagSlot(bagType, itemGrid->selectedIndex());
    auto inventory = m_player->inventory();
    if (auto sourceItem = as<ObjectItem>(itemGrid->selectedItem())) {
      if (auto actionTypeName = sourceItem->instanceValue("interactAction")) {
        auto actionType = InteractActionTypeNames.getLeft(actionTypeName.toString());
        if (actionType >= InteractActionType::OpenCraftingInterface && actionType <= InteractActionType::ScriptPane) {
          auto actionData = sourceItem->instanceValue("interactData", Json());
          if (actionData.isType(Json::Type::Object))
            actionData = actionData.set("openWithInventory", false);
          InteractAction action(actionType, NullEntityId, actionData);
          m_player->interact(action);
        }
      }
    }
  };

  Json itemBagConfig = config.get("bagConfig");
  auto bagOrder = itemBagConfig.toObject().keys().sorted([&itemBagConfig](String const& a, String const& b) {
    return itemBagConfig.get(a).getInt("order", 0) < itemBagConfig.get(b).getInt("order", 0);
  });
  for (auto name : bagOrder) {
    auto itemGrid = itemBagConfig.get(name).getString("itemGrid");
    invWindowReader.registerCallback(itemGrid, bind(leftClickCallback, name, _1));
    invWindowReader.registerCallback(itemGrid + ".right", bind(bagGridCallback, name, _1));
    invWindowReader.registerCallback(itemGrid + ".middle", bind(middleClickCallback, name, _1));
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

  for (auto const p : EquipmentSlotNames) {
    EquipmentSlot slot = p.first;
    registerSlotCallbacks(p.second, slot);
    invWindowReader.registerCallback(p.second + ".middle", [slot, this](Widget* paneObj) {
      auto inventory = m_player->inventory();
      inventory->setEquipmentVisibility(slot, !inventory->equipmentVisibility(slot));
    });
  }
  registerSlotCallbacks("trash", TrashSlot());

  invWindowReader.construct(config.get("paneLayout"), this);

  m_trashSlot = fetchChild<ItemSlotWidget>("trash");
  m_trashBurn = GameTimer(config.get("trashBurnTimeout").toFloat());

  m_disabledTechOverlays.append(fetchChild<ImageWidget>("techHeadDisabled"));
  m_disabledTechOverlays.append(fetchChild<ImageWidget>("techBodyDisabled"));
  m_disabledTechOverlays.append(fetchChild<ImageWidget>("techLegsDisabled"));

  for (auto const p : EquipmentSlotNames) {
    if (auto itemSlot = fetchChild<ItemSlotWidget>(p.second)) {
      itemSlot->setItem(m_player->inventory()->itemsAt(p.first));
      if (auto indicator = itemSlot->findChild<ImageWidget>("hidden"))
        indicator->setVisibility(!m_player->inventory()->equipmentVisibility(p.first));
      if (p.first >= EquipmentSlot::Cosmetic1)
        itemSlot->hide();
    }
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

  if ((m_displayingCosmetics = m_alwaysDisplayCosmetics = config.getBool("alwaysDisplayCosmetics", false))) {
    for (auto const& p : EquipmentSlotNames) {
      if (p.first >= EquipmentSlot::Cosmetic1) {
        if (auto itemSlot = fetchChild<ItemSlotWidget>(p.second))
          itemSlot->setVisibility(true);
      }
    }
  }

  m_expectingSwap = false;

  if (auto item = m_player->inventory()->swapSlotItem())
    m_currentSwapSlotItem = item->descriptor();
  m_pickUpSounds = jsonToStringList(config.get("sounds").get("pickup"));
  m_putDownSounds = jsonToStringList(config.get("sounds").get("putdown"));
  m_someUpSounds = jsonToStringList(config.get("sounds").get("someup"));
  m_someDownSounds = jsonToStringList(config.get("sounds").get("somedown"));
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
          if (techDatabase->contains(*techModule))
            return SimpleTooltipBuilder::buildTooltip(techDatabase->tech(*techModule).description);
      }
    }
  }

  return {};
}

bool InventoryPane::sendEvent(InputEvent const& event) {
  if (m_alwaysDisplayCosmetics)
    return Pane::sendEvent(event);

  if (auto mousePosition = Widget::context()->mousePosition(event)) {
    bool displayingCosmetics = false;
    for (auto const& p : EquipmentSlotNames) {
      if (auto itemSlot = fetchChild<ItemSlotWidget>(p.second)) {
        if ((displayingCosmetics = itemSlot->inMember(*mousePosition)))
          break;
      }
    }

    if (m_displayingCosmetics != displayingCosmetics) {
      for (auto const& p : EquipmentSlotNames) {
        if (p.first >= EquipmentSlot::Cosmetic1) {
          if (auto itemSlot = fetchChild<ItemSlotWidget>(p.second))
            itemSlot->setVisibility(displayingCosmetics);
        }
      }
      m_displayingCosmetics = displayingCosmetics;
    }
  }

  return Pane::sendEvent(event);
}

bool InventoryPane::giveContainerResult(ContainerResult result) {
  if (!m_expectingSwap)
    return false;

  for (auto& item : result) {
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
  for (auto& p : m_itemGrids)
    p.second->updateItemState();
}

bool InventoryPane::containsNewItems() const {
  for (auto& p : m_itemGrids) {
    if (p.second->slotsChanged())
      return true;
  }
  return false;
}

void InventoryPane::clearChangedSlots() {
  for (auto& p : m_itemGrids) {
    p.second->updateItemState();
    p.second->clearChangedSlots();
  }
}

void InventoryPane::update(float dt) {
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
    if (m_trashBurn.tick(dt) && trashItem->count() > 0) {
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
      if (auto indicator = itemSlot->findChild<ImageWidget>("hidden"))
        indicator->setVisibility(!inventory->equipmentVisibility(p.first));
    }
  }

  auto techDatabase = Root::singleton().techDatabase();
  for (auto const& p : TechTypeNames) {
    if (auto techIcon = fetchChild<ImageWidget>(strf("tech{}", p.second))) {
      if (auto techModule = m_player->techs()->equippedTechs().maybe(p.first)) {
        if (techDatabase->contains(*techModule)) {
          techIcon->setImage(techDatabase->tech(*techModule).icon);
          continue;
        }
      }
      techIcon->setImage("");
    }
  }

  if (ItemPtr swapSlot = inventory->swapSlotItem()) {
    if (!PlayerInventory::itemAllowedInBag(swapSlot, m_selectedTab)) {
      for (auto& pair : m_itemGrids) {
        if (pair.first != m_selectedTab && PlayerInventory::itemAllowedInBag(swapSlot, pair.first)) {
          selectTab(pair.first);
          break;
        }
      }
    }
  }

  for (auto p : m_itemGrids) {
    p.second->updateItemState();
    for (size_t i = 0; i < p.second->itemSlots(); ++i) {
      if (auto itemWidget = p.second->itemWidgetAt(i))
        itemWidget->showLinkIndicator(customBarItems.contains(itemWidget->item()));
      else
        Logger::warn("Could not find item widget {} in item grid {}!", i, p.first);
    }
  }

  m_itemGrids[m_selectedTab]->clearChangedSlots();

  for (auto& pair : m_newItemMarkers) {
    if (m_itemGrids[pair.first]->slotsChanged())
      pair.second->show();
    else
      pair.second->hide();
  }

  for (auto& techOverlay : m_disabledTechOverlays)
    techOverlay->setVisibility(m_player->techOverridden());

  auto healthLabel = fetchChild<LabelWidget>("healthtext");
  healthLabel->setText(strf("{:.1f}", m_player->maxHealth()));
  auto energyLabel = fetchChild<LabelWidget>("energytext");
  energyLabel->setText(strf("{:.1f}", m_player->maxEnergy()));
  auto weaponLabel = fetchChild<LabelWidget>("weapontext");
  weaponLabel->setText(strf("{:.1f}%", m_player->powerMultiplier() * 100));
  auto defenseLabel = fetchChild<LabelWidget>("defensetext");
  if (m_player->protection() == 0)
    defenseLabel->setText("--");
  else
    defenseLabel->setText(strf("{:.1f}", m_player->protection()));

  auto moneyLabel = fetchChild<LabelWidget>("lblMoney");
  moneyLabel->setText(toString(m_player->currency("money")));

  if (m_player->currency("essence") > 0) {
    fetchChild<ImageWidget>("imgEssenceIcon")->show();
    auto essenceLabel = fetchChild<LabelWidget>("lblEssence");
    essenceLabel->show();
    essenceLabel->setText(toString(m_player->currency("essence")));
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
      attackLabel->setText(strf("{:.1f}", *attack));
    } else {
      attackLabel->setText("");
    }

    auto defenseLabel = fetchChild<LabelWidget>("companionDefenseStat");
    if (auto defense = pet->stat("defense")) {
      defenseLabel->setText(strf("{:.1f}", *defense));
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
    float pitch = 1.f - ((float)item->count() / (float)item->maxStack()) * .2f;
    if (!m_currentSwapSlotItem || !item->matches(*m_currentSwapSlotItem, true))
      context->playAudio(RandomSource().randFrom(m_pickUpSounds), 0, 1.f, pitch);
    else if (item->count() > m_currentSwapSlotItem->count())
      context->playAudio(RandomSource().randFrom(m_someUpSounds), 0, 1.f, pitch);
    else if (item->count() < m_currentSwapSlotItem->count())
      context->playAudio(RandomSource().randFrom(m_someDownSounds), 0, 1.f, pitch);

    m_currentSwapSlotItem = item->descriptor();
  } else {
    if (m_currentSwapSlotItem)
      context->playAudio(RandomSource().randFrom(m_putDownSounds));
    m_currentSwapSlotItem = {};
  }

  m_title = m_player->name();

  Pane::update(dt);
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
