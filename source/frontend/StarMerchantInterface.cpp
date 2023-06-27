#include "StarMerchantInterface.hpp"
#include "StarJsonExtra.hpp"
#include "StarGuiReader.hpp"
#include "StarLexicalCast.hpp"
#include "StarRoot.hpp"
#include "StarItemTooltip.hpp"
#include "StarPlayer.hpp"
#include "StarWorldClient.hpp"
#include "StarButtonWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarItemGridWidget.hpp"
#include "StarListWidget.hpp"
#include "StarTabSet.hpp"
#include "StarAssets.hpp"
#include "StarItemDatabase.hpp"
#include "StarPlayerInventory.hpp"
#include "StarItemBag.hpp"
#include "StarQuestManager.hpp"

namespace Star {

MerchantPane::MerchantPane(
    WorldClientPtr worldClient, PlayerPtr player, Json const& settings, EntityId sourceEntityId) {
  m_worldClient = move(worldClient);
  m_player = move(player);
  m_sourceEntityId = sourceEntityId;

  auto assets = Root::singleton().assets();
  auto baseConfig = settings.get("config", "/interface/windowconfig/merchant.config");
  m_settings = jsonMerge(assets->fetchJson(baseConfig), settings);

  m_refreshTimer = GameTimer(assets->json("/merchant.config:autoRefreshRate").toFloat());

  m_buyFactor = m_settings.getFloat("buyFactor", assets->json("/merchant.config:defaultBuyFactor").toFloat());
  m_sellFactor = m_settings.getFloat("sellFactor", assets->json("/merchant.config:defaultSellFactor").toFloat());

  m_itemBag = make_shared<ItemBag>(m_settings.getUInt("sellContainerSize"));

  GuiReader reader;
  reader.registerCallback("spinCount.up", [=](Widget*) {
      if (m_selectedIndex != NPos) {
        if (m_buyCount < maxBuyCount())
          m_buyCount++;
        else
          m_buyCount = 1;
      } else {
        m_buyCount = 0;
      }
      countChanged();
    });

  reader.registerCallback("spinCount.down", [=](Widget*) {
      if (m_selectedIndex != NPos) {
        if (m_buyCount > 1)
          m_buyCount--;
        else
          m_buyCount = std::max(maxBuyCount(), 1);
      } else {
        m_buyCount = 0;
      }
      countChanged();
    });

  reader.registerCallback("countChanged", [=](Widget*) { countChanged(); });
  reader.registerCallback("parseCountText", [=](Widget*) { countTextChanged(); });

  reader.registerCallback("buy", [=](Widget*) { buy(); });

  reader.registerCallback("sell", [=](Widget*) { sell(); });

  reader.registerCallback("close", [=](Widget*) { dismiss(); });

  reader.registerCallback("itemGrid",
      [=](Widget*) {
        swapSlot();
        updateSellTotal();
      });

  Json paneLayout = m_settings.get("paneLayout");
  paneLayout = jsonMerge(paneLayout, m_settings.get("paneLayoutOverride", {}));
  reader.construct(paneLayout, this);

  m_tabSet = findChild<TabSetWidget>("buySellTabs");
  m_tabSet->setCallback([this](Widget*) {
    auto bgResult = getBG();
    if (m_tabSet->selectedTab() == 0)
      bgResult.body = m_settings.getString("buyBody");
    else
      bgResult.body = m_settings.getString("sellBody");
    setBG(bgResult);
  });
  m_itemGuiList = findChild<ListWidget>("itemList");
  m_countTextBox = findChild<TextBoxWidget>("tbCount");
  m_buyTotalLabel = findChild<LabelWidget>("lblBuyTotal");
  m_buyButton = findChild<ButtonWidget>("btnBuy");
  m_sellTotalLabel = findChild<LabelWidget>("lblSellTotal");
  m_sellButton = findChild<ButtonWidget>("btnSell");

  m_itemGrid = findChild<ItemGridWidget>("itemGrid");
  m_itemGrid->setItemBag(m_itemBag);

  buildItemList();

  updateSelection();

  updateSellTotal();
}

void MerchantPane::displayed() {
  Pane::displayed();
}

void MerchantPane::dismissed() {
  Pane::dismissed();

  for (auto unsold : m_itemBag->takeAll())
    m_player->giveItem(unsold);

  m_worldClient->sendEntityMessage(m_sourceEntityId, "onMerchantClosed");
}

PanePtr MerchantPane::createTooltip(Vec2I const& screenPosition) {
  if (m_tabSet->selectedTab() == 0) {
    for (size_t i = 0; i < m_itemGuiList->numChildren(); ++i) {
      auto entry = m_itemGuiList->itemAt(i);
      if (entry->getChildAt(screenPosition)) {
        auto itemConfig = m_itemList.get(i);
        ItemPtr item = Root::singleton().itemDatabase()->item(ItemDescriptor(itemConfig.get("item")));
        return ItemTooltipBuilder::buildItemTooltip(item, m_player);
      }
    }
  } else {
    if (auto item = m_itemGrid->itemAt(screenPosition))
      return ItemTooltipBuilder::buildItemTooltip(item, m_player);
  }
  return {};
}

void MerchantPane::update() {
  Pane::update();

  if (!m_worldClient->playerCanReachEntity(m_sourceEntityId))
    dismiss();

  if (m_refreshTimer.wrapTick()) {
    for (size_t i = 0; i < m_itemList.size(); ++i) {
      auto itemConfig = m_itemList.get(i);
      auto itemWidget = m_itemGuiList->itemAt(i);
      setupWidget(itemWidget, itemConfig);
    }
    updateBuyTotal();
  }

  updateSelection();

  m_itemGrid->updateAllItemSlots();
}

EntityId MerchantPane::sourceEntityId() const {
  return m_sourceEntityId;
}

ItemPtr MerchantPane::addItems(ItemPtr const& items) {
  if (m_tabSet->selectedTab() == 1) {
    auto remainder = m_itemBag->addItems(items);
    updateSellTotal();
    return remainder;
  } else {
    return items;
  }
}

void MerchantPane::swapSlot() {
  ItemPtr source = m_player->inventory()->swapSlotItem();
  auto inv = m_player->inventory();
  if (context()->shiftHeld()) {
    if (m_itemGrid->selectedItem()) {
      auto remainder = inv->addItems(m_itemBag->takeItems(m_itemGrid->selectedIndex()));
      if (remainder && !remainder->empty())
        m_itemBag->setItem(m_itemGrid->selectedIndex(), remainder);
    }
  } else {
    if (auto heldItem = m_player->inventory()->swapSlotItem())
      inv->setSwapSlotItem(m_itemBag->swapItems(m_itemGrid->selectedIndex(), heldItem));
    else
      inv->setSwapSlotItem(m_itemBag->takeItems(m_itemGrid->selectedIndex()));
  }
}

void MerchantPane::buildItemList() {
  m_itemGuiList->clear();
  m_itemList = m_settings.getArray("items");

  auto itemDatabase = Root::singleton().itemDatabase();
  filter(m_itemList, [&](Json const& itemConfig) {
      if (!itemDatabase->hasItem(ItemDescriptor(itemConfig.get("item")).name()))
        return false;

      if (auto prerequisite = itemConfig.optString("prerequisiteQuest")) {
        if (!m_player->questManager()->hasCompleted(*prerequisite))
          return false;
      }

      if (auto quests = itemConfig.optArray("exclusiveQuests")) {
        for (auto quest : *quests) {
          if (m_player->questManager()->hasQuest(quest.toString()))
            return false;
        }
      }

      if (auto prerequisite = itemConfig.optUInt("prerequisiteShipLevel")) {
        if (m_player->shipUpgrades().shipLevel < *prerequisite)
          return false;
      }

      if (auto maxLevel = itemConfig.optUInt("maxShipLevel")) {
        if (m_player->shipUpgrades().shipLevel > *maxLevel)
          return false;
      }

      return true;
    });

  for (auto itemConfig : m_itemList) {
    auto widget = m_itemGuiList->addItem();
    setupWidget(widget, itemConfig);
  }
}

void MerchantPane::setupWidget(WidgetPtr const& widget, Json const& itemConfig) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  ItemPtr item = root.itemDatabase()->item(ItemDescriptor(itemConfig.get("item")));

  String name = item->friendlyName();
  if (item->count() > 1)
    name = strf("{} (x{})", name, item->count());

  auto itemName = widget->fetchChild<LabelWidget>("itemName");
  itemName->setText(name);

  unsigned price = ceil(itemConfig.getInt("price", item->price()) * m_buyFactor);
  widget->setLabel("priceLabel", strf("{}", price));
  widget->setData(price);

  bool unavailable = price > m_player->currency("money");
  auto unavailableoverlay = widget->fetchChild<ImageWidget>("unavailableoverlay");
  if (unavailable) {
    itemName->setColor(Color::Gray);
    unavailableoverlay->show();
  } else {
    itemName->setColor(Color::White);
    unavailableoverlay->hide();
  }

  widget->fetchChild<ItemSlotWidget>("itemIcon")->setItem(item);
  widget->show();
}

void MerchantPane::updateSelection() {
  if (m_selectedIndex != m_itemGuiList->selectedItem()) {
    m_selectedIndex = m_itemGuiList->selectedItem();

    if (m_selectedIndex != NPos) {
      auto itemConfig = m_itemList.get(m_selectedIndex);
      m_selectedItem = Root::singleton().itemDatabase()->item(ItemDescriptor(itemConfig.get("item")));
      findChild<ButtonWidget>("spinCount.up")->enable();
      findChild<ButtonWidget>("spinCount.down")->enable();
      m_countTextBox->setColor(Color::White);
      m_buyCount = 1;
    } else {
      findChild<ButtonWidget>("spinCount.up")->disable();
      findChild<ButtonWidget>("spinCount.down")->disable();
      m_countTextBox->setColor(Color::Gray);
      m_buyCount = 0;
    }

    countChanged();
  }
}

void MerchantPane::updateBuyTotal() {
  if (auto selected = m_itemGuiList->selectedWidget())
    m_buyTotal = selected->data().toUInt() * m_buyCount;
  else
    m_buyTotal = 0;

  m_buyTotalLabel->setText(strf("{}", m_buyTotal));

  if (m_selectedIndex != NPos && m_buyCount > 0)
    m_buyButton->enable();
  else
    m_buyButton->disable();

  if (m_buyTotal > (int)m_player->inventory()->currency("money")) {
    m_buyTotalLabel->setColor(Color::Red);
    m_buyButton->disable();
  } else {
    m_buyTotalLabel->setColor(Color::White);
  }
}

void MerchantPane::buy() {
  if (m_buyTotal > 0 && m_player->inventory()->consumeCurrency("money", m_buyTotal)) {
    auto countRemaining = m_buyCount;
    while (countRemaining > 0) {
      auto buyItem = m_selectedItem->clone();
      buyItem->setCount(m_selectedItem->count() * countRemaining);
      countRemaining -= buyItem->count();
      m_player->giveItem(buyItem);
    }

    auto reportItem = m_selectedItem->clone();
    reportItem->setCount(reportItem->count() * m_buyCount, true);
    auto buySummary = JsonObject{{"item", reportItem->descriptor().toJson()}, {"total", m_buyTotal}};
    m_worldClient->sendEntityMessage(m_sourceEntityId, "onBuy", {buySummary});

    auto& guiContext = GuiContext::singleton();
    guiContext.playAudio(Root::singleton().assets()->json("/merchant.config:buySound").toString());

    buildItemList();

    updateBuyTotal();
  }
}

void MerchantPane::updateSellTotal() {
  m_sellTotal = 0;
  for (auto item : m_itemBag->items()) {
    if (item)
      m_sellTotal += round(item->price() * m_sellFactor);
  }
  m_sellTotalLabel->setText(strf("{}", m_sellTotal));
  if (m_sellTotal > 0)
    m_sellButton->enable();
  else
    m_sellButton->disable();
}

void MerchantPane::sell() {
  if (m_sellTotal > 0) {
    auto sellSummary = JsonObject{{"items", m_itemBag->toJson()}, {"total", m_sellTotal}};
    m_worldClient->sendEntityMessage(m_sourceEntityId, "onSell", {sellSummary});

    m_player->inventory()->addCurrency("money", m_sellTotal);
    m_itemBag->clearItems();
    updateSellTotal();

    auto& guiContext = GuiContext::singleton();
    guiContext.playAudio(Root::singleton().assets()->json("/merchant.config:sellSound").toString());
  }
}

int MerchantPane::maxBuyCount() {
  if (auto selected = m_itemGuiList->selectedWidget()) {
    auto assets = Root::singleton().assets();
    auto unitPrice = selected->data().toUInt();
    if (unitPrice == 0)
      return 1000;
    return min(1000, (int)floor(m_player->currency("money") / unitPrice));
  } else {
    return 0;
  }
}

void MerchantPane::countChanged() {
  m_countTextBox->setText(strf("x{}", m_buyCount));
  updateBuyTotal();
}

void MerchantPane::countTextChanged() {
  if (m_selectedIndex == NPos) {
    m_buyCount = 0;
    countChanged();
  } else {
    try {
      auto countString = m_countTextBox->getText().replace("x", "");
      if (countString.size()) {
        m_buyCount = clamp<int>(lexicalCast<int>(countString), 1, maxBuyCount());
        countChanged();
      }
    } catch (BadLexicalCast const&) {
      m_buyCount = 1;
      countChanged();
    }
  }
}

}
