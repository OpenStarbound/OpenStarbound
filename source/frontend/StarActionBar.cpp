#include "StarActionBar.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarGuiReader.hpp"
#include "StarItemTooltip.hpp"
#include "StarUniverseClient.hpp"
#include "StarItemGridWidget.hpp"
#include "StarItemSlotWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarPaneManager.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarAssets.hpp"
#include "StarItem.hpp"
#include "StarMerchantInterface.hpp"

namespace Star {

ActionBar::ActionBar(MainInterfacePaneManager* paneManager, PlayerPtr player) {
  m_paneManager = paneManager;
  m_player = move(player);

  auto assets = Root::singleton().assets();

  m_config = assets->json("/interface/windowconfig/actionbar.config");

  m_actionBarSelectOffset = jsonToVec2I(m_config.get("actionBarSelectOffset"));
  m_switchSounds = jsonToStringList(m_config.get("sounds").get("switch"));

  GuiReader reader;

  for (uint8_t i = 0; i < m_player->inventory()->customBarIndexes(); ++i) {
    reader.registerCallback(strf("customBar{}L", i + 1), bind(&ActionBar::customBarClick, this, i, true));
    reader.registerCallback(strf("customBar{}R", i + 1), bind(&ActionBar::customBarClick, this, i, false));

    reader.registerCallback(strf("customBar{}L.right", i + 1), bind(&ActionBar::customBarClickRight, this, i, true));
    reader.registerCallback(strf("customBar{}R.right", i + 1), bind(&ActionBar::customBarClickRight, this, i, false));
  }

  for (uint8_t i = 0; i < EssentialItemCount; ++i)
    reader.registerCallback(strf("essentialBar{}", i + 1), bind(&ActionBar::essentialBarClick, this, i));

  reader.registerCallback("pickupToActionBar", [=](Widget* widget) {
      auto button = as<ButtonWidget>(widget);
      Root::singleton().configuration()->setPath("inventory.pickupToActionBar", button->isChecked());
    });

  reader.registerCallback("swapCustomBar", [this](Widget*) {
      swapCustomBar();
    });

  reader.construct(m_config.get("paneLayout"), this);

  for (uint8_t i = 0; i < m_player->inventory()->customBarIndexes(); ++i) {
    auto customBarLeft = fetchChild<ItemSlotWidget>(strf("customBar{}L", i + 1));
    auto customBarRight = fetchChild<ItemSlotWidget>(strf("customBar{}R", i + 1));
    auto customBarLeftOverlay = fetchChild<ImageWidget>(strf("customBar{}LOverlay", i + 1));
    auto customBarRightOverlay = fetchChild<ImageWidget>(strf("customBar{}ROverlay", i + 1));

    TextPositioning countPosition = {jsonToVec2F(m_config.get("countMidAnchor")), HorizontalAnchor::HMidAnchor};
    customBarLeft->setCountPosition(countPosition);
    customBarLeft->setCountFontMode(FontMode::Shadow);
    customBarRight->setCountPosition(countPosition);
    customBarRight->setCountFontMode(FontMode::Shadow);

    m_customBarWidgets.append({customBarLeft, customBarRight, customBarLeftOverlay, customBarRightOverlay});
  }
  m_customSelectedWidget = fetchChild<ImageWidget>("customSelect");

  for (uint8_t i = 0; i < EssentialItemCount; ++i)
    m_essentialBarWidgets.append(fetchChild<ItemSlotWidget>(strf("essentialBar{}", i + 1)));
  m_essentialSelectedWidget = fetchChild<ImageWidget>("essentialSelect");
}

PanePtr ActionBar::createTooltip(Vec2I const& screenPosition) {
  ItemPtr item;
  auto tryItemWidget = [&](ItemSlotWidgetPtr const& isw) {
    if (isw->screenBoundRect().contains(screenPosition))
      item = isw->item();
  };

  for (auto const& p : m_customBarWidgets) {
    tryItemWidget(p.left);
    tryItemWidget(p.right);
  }

  for (auto const& w : m_essentialBarWidgets)
    tryItemWidget(w);

  if (!item)
    return {};

  return ItemTooltipBuilder::buildItemTooltip(item, m_player);
}

bool ActionBar::sendEvent(InputEvent const& event) {
  if (Pane::sendEvent(event))
    return true;

  auto inventory = m_player->inventory();

  auto customBarIndexes = inventory->customBarIndexes();
  if (auto mouseWheel = event.ptr<MouseWheelEvent>()) {
    auto abl = inventory->selectedActionBarLocation();

    int index = 0;
    if (!abl) {
      if (mouseWheel->mouseWheel == MouseWheel::Down)
        index = 0;
      else
        index = customBarIndexes + EssentialItemCount - 1;
    } else {
      if (auto cbi = abl.ptr<CustomBarIndex>()) {
        if (*cbi < customBarIndexes / 2)
          index = *cbi;
        else
          index = *cbi + EssentialItemCount;
      } else {
        index = customBarIndexes / 2 + (int)abl.get<EssentialItem>();
      }

      if (mouseWheel->mouseWheel == MouseWheel::Down)
        index = pmod(index + 1, customBarIndexes + EssentialItemCount);
      else
        index = pmod(index - 1, customBarIndexes + EssentialItemCount);
    }

    if (index < customBarIndexes / 2)
      abl = (CustomBarIndex)index;
    else if (index < customBarIndexes / 2 + EssentialItemCount)
      abl = (EssentialItem)(index - customBarIndexes / 2);
    else
      abl = (CustomBarIndex)(index - EssentialItemCount);

    inventory->selectActionBarLocation(abl);
    context()->playAudio(RandomSource().randFrom(m_switchSounds));

    return true;
  }

  if (event.is<MouseMoveEvent>()) {
    m_customBarHover.reset();
    Vec2I screenPosition = *GuiContext::singleton().mousePosition(event);
    for (uint8_t i = 0; i < customBarIndexes; ++i) {
      if (m_customBarWidgets[i].left->screenBoundRect().contains(screenPosition))
        m_customBarHover = make_pair((CustomBarIndex)i, false);
      else if (m_customBarWidgets[i].right->screenBoundRect().contains(screenPosition))
        m_customBarHover = make_pair((CustomBarIndex)i, true);
    }
  }

  for (auto action : context()->actions(event)) {
    if (action >= InterfaceAction::InterfaceBar1 && action <= InterfaceAction::InterfaceBar6)
      inventory->selectActionBarLocation((CustomBarIndex)((int)action - (int)InterfaceAction::InterfaceBar1));

    if (action >= InterfaceAction::EssentialBar1 && action <= InterfaceAction::EssentialBar4)
      inventory->selectActionBarLocation((EssentialItem)((int)action - (int)InterfaceAction::EssentialBar1));

    if (action == InterfaceAction::InterfaceDeselectHands) {
      if (auto previousSelectedLocation = inventory->selectedActionBarLocation()) {
        m_emptyHandsPreviousActionBarLocation = inventory->selectedActionBarLocation();
        inventory->selectActionBarLocation({});
      } else {
        inventory->selectActionBarLocation(take(m_emptyHandsPreviousActionBarLocation));
      }
    }

    if (action == InterfaceAction::InterfaceChangeBarGroup)
      swapCustomBar();
  }

  return false;
}

void ActionBar::update(float dt) {
  auto inventory = m_player->inventory();
  auto abl = inventory->selectedActionBarLocation();
  if (abl.is<CustomBarIndex>()) {
    auto overlayLoc = m_customBarWidgets.at(abl.get<CustomBarIndex>()).left->position();
    m_customSelectedWidget->setPosition(overlayLoc + m_actionBarSelectOffset);
    m_customSelectedWidget->show();
    m_essentialSelectedWidget->hide();
  } else if (abl.is<EssentialItem>()) {
    auto overlayLoc = m_essentialBarWidgets.at((size_t)abl.get<EssentialItem>())->position();
    m_essentialSelectedWidget->setPosition(overlayLoc + m_actionBarSelectOffset);
    m_essentialSelectedWidget->show();
    m_customSelectedWidget->hide();
  } else {
    m_essentialSelectedWidget->hide();
    m_customSelectedWidget->hide();
  }

  for (uint8_t i = 0; i < EssentialItemCount; ++i)
    m_essentialBarWidgets[i]->setItem(inventory->essentialItem((EssentialItem)i));

  for (uint8_t i = 0; i < inventory->customBarIndexes(); ++i) {
    // If there is no swap slot item being hovered over the custom bar, then
    // simply set the left and right item widgets in the custom bar to the
    // primary and secondary items.  If the primary item is two handed, the
    // secondary item will be null and the primary hand item is drawn dimmed in
    // the right hand slot.  If there IS a swap slot item being hovered over
    // this spot in the custom bar, things are more complex.  Instead of
    // showing what is currently in the custom bar, a preview of what WOULD
    // happen when linking is shown, except both the left and right item
    // widgets are always shown with no count and always dimmed to indicate
    // that it is just a preview.

    ItemPtr primaryItem;
    ItemPtr secondaryItem;

    if (auto slot = inventory->customBarPrimarySlot(i))
      primaryItem = inventory->itemsAt(*slot);

    if (auto slot = inventory->customBarSecondarySlot(i))
      secondaryItem = inventory->itemsAt(*slot);

    bool primaryPreview = false;
    bool secondaryPreview = false;

    ItemPtr swapSlotItem = inventory->swapSlotItem();
    if (swapSlotItem && m_customBarHover && m_customBarHover->first == i) {
      if (!m_customBarHover->second || swapSlotItem->twoHanded()) {
        if (!primaryItem && swapSlotItem == secondaryItem)
          secondaryItem = {};
        primaryItem = swapSlotItem;
        primaryPreview = true;
      } else {
        if (itemSafeTwoHanded(primaryItem))
          primaryItem = {};
        if (!secondaryItem && swapSlotItem == primaryItem)
          primaryItem = {};
        secondaryItem = swapSlotItem;
        secondaryPreview = true;
      }
    }

    auto& widgets = m_customBarWidgets[i];
    widgets.left->setItem(primaryItem);
    if (primaryPreview) {
      widgets.left->showDurability(false);
      widgets.left->showCount(false);
      widgets.leftOverlay->show();
    } else {
      widgets.left->showDurability(true);
      widgets.left->showCount(true);
      widgets.leftOverlay->hide();
    }

    if (itemSafeTwoHanded(primaryItem)) {
      widgets.right->setItem(primaryItem);
      widgets.right->showDurability(false);
      widgets.right->showCount(false);
      widgets.rightOverlay->show();
    } else {
      widgets.right->setItem(secondaryItem);
      if (secondaryPreview) {
        widgets.right->showDurability(false);
        widgets.right->showCount(false);
        widgets.rightOverlay->show();
      } else {
        widgets.right->showDurability(true);
        widgets.right->showCount(true);
        widgets.rightOverlay->hide();
      }
    }

    widgets.left->setHighlightEnabled(!widgets.left->item() && swapSlotItem);
    widgets.right->setHighlightEnabled(!widgets.right->item() && swapSlotItem);
  }

  fetchChild<ButtonWidget>("pickupToActionBar")->setChecked(Root::singleton().configuration()->getPath("inventory.pickupToActionBar").toBool());
  fetchChild<ButtonWidget>("swapCustomBar")->setChecked(m_player->inventory()->customBarGroup() != 0);
}

Maybe<String> ActionBar::cursorOverride(Vec2I const&) {
  if (m_customBarHover && m_player->inventory()->swapSlotItem())
    return m_config.getString("linkCursor");
  return {};
}

void ActionBar::customBarClick(uint8_t index, bool primary) {
  if (auto swapItem = m_player->inventory()->swapSlotItem()) {
    if (primary || itemSafeTwoHanded(swapItem))
      m_player->inventory()->setCustomBarPrimarySlot(index, InventorySlot(SwapSlot()));
    else
      m_player->inventory()->setCustomBarSecondarySlot(index, InventorySlot(SwapSlot()));

    m_player->inventory()->clearSwap();

  } else {
    m_player->inventory()->selectActionBarLocation(index);
  }
}

void ActionBar::customBarClickRight(uint8_t index, bool primary) {
  if (m_paneManager->registeredPaneIsDisplayed(MainInterfacePanes::Inventory)) {
    auto inventory = m_player->inventory();
    auto primarySlot = inventory->customBarPrimarySlot(index);
    auto secondarySlot = inventory->customBarSecondarySlot(index);

    if (primary || (primarySlot && itemSafeTwoHanded(inventory->itemsAt(*primarySlot))))
      inventory->setCustomBarPrimarySlot(index, {});
    else
      inventory->setCustomBarSecondarySlot(index, {});
  }
}

void ActionBar::essentialBarClick(uint8_t index) {
  m_player->inventory()->selectActionBarLocation((EssentialItem)index);
}

void ActionBar::swapCustomBar() {
  m_player->inventory()->setCustomBarGroup((m_player->inventory()->customBarGroup() + 1) % m_player->inventory()->customBarGroups());
}

}
