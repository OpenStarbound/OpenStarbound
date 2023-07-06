#include "StarCraftingInterface.hpp"
#include "StarJsonExtra.hpp"
#include "StarGuiReader.hpp"
#include "StarLexicalCast.hpp"
#include "StarRoot.hpp"
#include "StarItemTooltip.hpp"
#include "StarPlayer.hpp"
#include "StarContainerEntity.hpp"
#include "StarWorldClient.hpp"
#include "StarPlayerBlueprints.hpp"
#include "StarButtonWidget.hpp"
#include "StarPaneManager.hpp"
#include "StarPortraitWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarListWidget.hpp"
#include "StarImageStretchWidget.hpp"
#include "StarItemSlotWidget.hpp"
#include "StarConfiguration.hpp"
#include "StarObjectItem.hpp"
#include "StarAssets.hpp"
#include "StarItemDatabase.hpp"
#include "StarObjectDatabase.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerLog.hpp"
#include "StarMixer.hpp"

namespace Star {

CraftingPane::CraftingPane(WorldClientPtr worldClient, PlayerPtr player, Json const& settings, EntityId sourceEntityId) {
  m_worldClient = move(worldClient);
  m_player = move(player);
  m_blueprints = m_player->blueprints();
  m_recipeAutorefreshCooldown = 0;
  m_sourceEntityId = sourceEntityId;

  auto assets = Root::singleton().assets();
  // get the config data for this crafting pane, default to "bare hands" crafting
  auto baseConfig = settings.get("config", "/interface/windowconfig/crafting.config");
  m_settings = jsonMerge(assets->fetchJson(baseConfig), settings);
  m_filter = StringSet::from(jsonToStringList(m_settings.get("filter", JsonArray())));

  GuiReader reader;
  reader.registerCallback("spinCount.up", [=](Widget*) {
      if (m_count < maxCraft())
        m_count++;
      else
        m_count = 1;
      countChanged();
    });

  reader.registerCallback("spinCount.down", [=](Widget*) {
      if (m_count > 1)
        m_count--;
      else
        m_count = std::max(maxCraft(), 1);
      countChanged();
    });

  reader.registerCallback("tbSpinCount", [=](Widget*) { countTextChanged(); });

  reader.registerCallback("close", [=](Widget*) { dismiss(); });

  reader.registerCallback("btnCraft", [=](Widget*) { toggleCraft(); });
  reader.registerCallback("btnStopCraft", [=](Widget*) { toggleCraft(); });

  reader.registerCallback("btnFilterHaveMaterials", [=](Widget*) {
      Root::singleton().configuration()->setPath("crafting.filterHaveMaterials", m_filterHaveMaterials->isChecked());
      updateAvailableRecipes();
    });

  reader.registerCallback("filter", [=](Widget*) { updateAvailableRecipes(); });

  reader.registerCallback("categories", [=](Widget*) { updateAvailableRecipes(); });

  reader.registerCallback("rarities", [=](Widget*) { updateAvailableRecipes(); });

  reader.registerCallback("btnUpgrade", [=](Widget*) { upgradeTable(); });

  // this is where the GUI gets built and the buttons begin to have existence.
  // all possible callbacks must exist by this point

  Json paneLayout = m_settings.get("paneLayout");
  paneLayout = jsonMerge(paneLayout, m_settings.get("paneLayoutOverride", {}));
  reader.construct(paneLayout, this);

  if (auto upgradeButton = fetchChild<ButtonWidget>("btnUpgrade")) {
    upgradeButton->disable();
    Maybe<JsonArray> recipeData = m_settings.optArray("upgradeMaterials");

    // create a recipe out of the listed upgrade materials.
    // for ease of creating a tooltip later.
    if (recipeData) {
      m_upgradeRecipe = ItemRecipe();
      for (auto ingredient : *recipeData)
        m_upgradeRecipe->inputs.append(ItemDescriptor(ingredient.getString("item"), ingredient.getUInt("count"), {}));
      upgradeButton->setVisibility(true);
    } else {
      upgradeButton->setVisibility(false);
    }
  }

  m_guiList = fetchChild<ListWidget>("scrollArea.itemList");
  m_textBox = fetchChild<TextBoxWidget>("tbSpinCount");

  m_filterHaveMaterials = fetchChild<ButtonWidget>("btnFilterHaveMaterials");
  if (m_filterHaveMaterials)
    m_filterHaveMaterials->setChecked(Root::singleton().configuration()->getPath("crafting.filterHaveMaterials").toBool());

  fetchChild<ButtonWidget>("btnCraft")->disable();
  if (auto spinCountUp = fetchChild<ButtonWidget>("spinCount.up"))
    spinCountUp->disable();
  if (auto spinCountDown = fetchChild<ButtonWidget>("spinCount.down"))
    spinCountDown->disable();

  m_displayedRecipe = NPos;
  updateAvailableRecipes();

  m_crafting = false;
  m_count = 1;
  countChanged();

  if (m_settings.getBool("titleFromEntity", false) && sourceEntityId != NullEntityId) {
    auto entity = m_worldClient->entity(sourceEntityId);

    if (auto container = as<ContainerEntity>(entity)) {
      if (container->iconItem()) {
        auto itemDatabase = Root::singleton().itemDatabase();
        auto iconItem = itemDatabase->item(container->iconItem());
        auto icon = make_shared<ItemSlotWidget>(iconItem, "/interface/inventory/portrait.png");
        String title = this->title();
        if (title.empty())
          title = container->containerDescription();
        String subTitle = this->subTitle();
        if (subTitle.empty())
          subTitle = container->containerSubTitle();
        icon->showRarity(false);
        setTitle(icon, title, subTitle);
      }
    }
    if (auto portaitEntity = as<PortraitEntity>(entity)) {
      auto portrait = make_shared<PortraitWidget>(portaitEntity, PortraitMode::Bust);
      portrait->setIconMode();
      String title = this->title();
      if (title.empty())
        title = portaitEntity->name();
      String subTitle = this->subTitle();
      setTitle(portrait, title, subTitle);
    }
  }
}

void CraftingPane::displayed() {
  Pane::displayed();

  if (auto filterWidget = fetchChild<TextBoxWidget>("filter")) {
    filterWidget->setText("");
    filterWidget->blur();
  }

  updateAvailableRecipes();

  // unlock any recipes specified
  if (auto recipeUnlocks = m_settings.opt("initialRecipeUnlocks")) {
    for (String itemName : jsonToStringList(*recipeUnlocks))
      m_player->addBlueprint(ItemDescriptor(itemName));
  }
}

void CraftingPane::dismissed() {
  stopCrafting();
  Pane::dismissed();
  m_itemCache.clear();
}

PanePtr CraftingPane::createTooltip(Vec2I const& screenPosition) {
  for (size_t i = 0; i < m_guiList->numChildren(); ++i) {
    auto entry = m_guiList->itemAt(i);
    if (entry->getChildAt(screenPosition)) {
      auto& recipe = m_recipesWidgetMap.getLeft(entry);
      return setupTooltip(recipe);
    }
  }

  if (WidgetPtr child = getChildAt(screenPosition)) {
    if (child->name() == "btnUpgrade") {
      if (m_upgradeRecipe)
        return setupTooltip(*m_upgradeRecipe);
    }
  }

  return {};
}

EntityId CraftingPane::sourceEntityId() const {
  return m_sourceEntityId;
}

void CraftingPane::upgradeTable() {
  if (m_sourceEntityId != NullEntityId) {
    // Checks that the upgrade path exists
    if (m_upgradeRecipe) {
      if (m_player->isAdmin() || ItemDatabase::canMakeRecipe(*m_upgradeRecipe, m_player->inventory()->availableItems(), m_player->inventory()->availableCurrencies())) {
        if (!m_player->isAdmin())
          consumeIngredients(*m_upgradeRecipe, 1);

        // upgrade the old table
        m_worldClient->sendEntityMessage(m_sourceEntityId, "requestUpgrade");

        // unlock any recipes specified
        if (auto recipeUnlocks = m_settings.opt("upgradeRecipeUnlocks")) {
          for (String itemName : jsonToStringList(*recipeUnlocks))
            m_player->addBlueprint(ItemDescriptor(itemName));
        }

        // this closes the interface window
        dismiss();
      }
    }
  }
}

size_t CraftingPane::itemCount(List<ItemPtr> const& store, ItemDescriptor const& item) {
  auto itemDb = Root::singleton().itemDatabase();
  return itemDb->getCountOfItem(store, item);
}

void CraftingPane::update() {
  // shut down if we can't reach the table anymore.
  if (m_sourceEntityId != NullEntityId) {
    auto sourceEntity = as<TileEntity>(m_worldClient->entity(m_sourceEntityId));
    if (!sourceEntity || !m_worldClient->playerCanReachEntity(m_sourceEntityId) || !sourceEntity->isInteractive()) {
      dismiss();
      return;
    }
  }

  // similarly if the player is dead
  if (m_player->isDead()) {
    dismiss();
    return;
  }

  // has the selected recipe changed ?
  bool changedHighlight = (m_displayedRecipe != m_guiList->selectedItem());

  if (changedHighlight) {
    stopCrafting(); // TODO: allow viewing other recipes without interrupting crafting

    m_displayedRecipe = m_guiList->selectedItem();
    countTextChanged();

    auto recipe = recipeFromSelectedWidget();

    if (recipe.isNull()) {
      fetchChild<Widget>("description")->removeAllChildren();
    } else {
      auto description = fetchChild<Widget>("description");
      description->removeAllChildren();

      auto item = Root::singleton().itemDatabase()->item(recipe.output);
      ItemTooltipBuilder::buildItemDescription(description, item);
    }
  }

  // crafters gonna craft
  while (m_crafting && m_craftTimer.wrapTick()) {
    craft(1);
  }

  // update crafting icon, progress and buttons
  if (auto currentRecipeIcon = fetchChild<ItemSlotWidget>("currentRecipeIcon")) {
    auto recipe = recipeFromSelectedWidget();
    if (recipe.isNull()) {
      currentRecipeIcon->setItem(nullptr);
    } else {
      auto single = recipe.output.singular();
      ItemPtr item = m_itemCache[single];
      currentRecipeIcon->setItem(item);

      if (m_crafting)
        currentRecipeIcon->setProgress(1.0f - m_craftTimer.percent());
      else
        currentRecipeIcon->setProgress(1.0f);
    }
  }

  --m_recipeAutorefreshCooldown;

  // changed recipe or auto update time
  if (changedHighlight || (m_recipeAutorefreshCooldown <= 0)) {
    updateAvailableRecipes();
    updateCraftButtons();
  }

  setLabel("lblPlayerMoney", toString((int)m_player->currency("money")));

  Pane::update();
}

void CraftingPane::updateCraftButtons() {
  auto normalizedBag = m_player->inventory()->availableItems();
  auto availableCurrencies = m_player->inventory()->availableCurrencies();

  auto recipe = recipeFromSelectedWidget();
  bool recipeAvailable = !recipe.isNull() && (m_player->isAdmin() || ItemDatabase::canMakeRecipe(recipe, normalizedBag, availableCurrencies));

  fetchChild<ButtonWidget>("btnCraft")->setEnabled(recipeAvailable);
  if (auto spinCountUp = fetchChild<ButtonWidget>("spinCount.up"))
    spinCountUp->setEnabled(recipeAvailable);
  if (auto spinCountDown = fetchChild<ButtonWidget>("spinCount.down"))
    spinCountDown->setEnabled(recipeAvailable);

  if (auto stopCraftButton = fetchChild<ButtonWidget>("btnStopCraft")) {
    stopCraftButton->setVisibility(m_crafting);
    fetchChild<ButtonWidget>("btnCraft")->setVisibility(!m_crafting);
  }

  if (auto upgradeButton = fetchChild<ButtonWidget>("btnUpgrade")) {
    bool canUpgrade = (m_upgradeRecipe && (m_player->isAdmin() || ItemDatabase::canMakeRecipe(*m_upgradeRecipe, normalizedBag, availableCurrencies)));
    upgradeButton->setEnabled(canUpgrade);
  }
}

void CraftingPane::updateAvailableRecipes() {
  m_recipeAutorefreshCooldown = 30;

  StringSet categoryFilter;
  if (auto categoriesGroup = fetchChild<ButtonGroupWidget>("categories")) {
    if (auto selectedCategories = categoriesGroup->checkedButton()) {
      for (auto group : selectedCategories->data().getArray("filter"))
        categoryFilter.add(group.toString());
    }
  }

  HashSet<Rarity> rarityFilter;
  if (auto raritiesGroup = fetchChild<ButtonGroupWidget>("rarities")) {
    if (auto selectedRarities = raritiesGroup->checkedButton()) {
      for (auto entry : jsonToStringSet(selectedRarities->data().getArray("rarity")))
        rarityFilter.add(RarityNames.getLeft(entry));
    }
  }

  String filterText;
  if (auto filterWidget = fetchChild<TextBoxWidget>("filter"))
    filterText = filterWidget->getText();

  m_recipes = determineRecipes();

  size_t currentOffset = 0;

  ItemRecipe selectedRecipe;
  if (m_guiList->selectedWidget())
    selectedRecipe = m_recipesWidgetMap.getLeft(m_guiList->selectedWidget());

  HashMap<ItemDescriptor, uint64_t> normalizedBag = m_player->inventory()->availableItems();

  m_guiList->clear();

  for (auto const& recipe : m_recipes) {
    auto widget = m_recipesWidgetMap.valueRight(recipe);
    if (widget) {
      m_guiList->addItem(widget);
    } else {
      widget = m_guiList->addItem();
      m_recipesWidgetMap.add(recipe, widget);
    }

    setupWidget(widget, recipe, normalizedBag);

    if (selectedRecipe == recipe) {
      m_guiList->setSelected(currentOffset);
    }

    currentOffset++;
  }
}

void CraftingPane::setupWidget(WidgetPtr const& widget, ItemRecipe const& recipe, HashMap<ItemDescriptor, uint64_t> const& normalizedBag) {
  auto& root = Root::singleton();

  auto single = recipe.output.singular();
  ItemPtr item = m_itemCache[single];
  if (!item) {
    item = root.itemDatabase()->item(single);
    m_itemCache[single] = item;
  }

  bool unavailable = false;
  size_t price = recipe.currencyInputs.value("money", 0);

  if (!m_player->isAdmin()) {
    for (auto const& p : recipe.currencyInputs) {
      if (m_player->currency(p.first) < p.second)
        unavailable = true;
    }

    auto itemDb = Root::singleton().itemDatabase();
    for (auto const& input : recipe.inputs) {
      if (itemDb->getCountOfItem(normalizedBag, input, recipe.matchInputParameters) < input.count())
        unavailable = true;
    }
  }

  String name = item->friendlyName();
  if (recipe.output.count() > 1)
    name = strf("{} (x{})", name, recipe.output.count());

  auto itemName = widget->fetchChild<LabelWidget>("itemName");
  auto notcraftableoverlay = widget->fetchChild<ImageWidget>("notcraftableoverlay");

  itemName->setText(name);

  if (unavailable) {
    itemName->setColor(Color::Gray);
    notcraftableoverlay->show();
  } else {
    itemName->setColor(Color::White);
    notcraftableoverlay->hide();
  }

  if (price > 0) {
    widget->setLabel("priceLabel", toString(price));
    if (auto icon = widget->fetchChild<ImageWidget>("moneyIcon"))
      icon->setVisibility(true);
  } else {
    widget->setLabel("priceLabel", "");
    if (auto icon = widget->fetchChild<ImageWidget>("moneyIcon"))
      icon->setVisibility(false);
  }

  if (auto newIndicator = widget->fetchChild<ImageWidget>("newIcon")) {
    if (m_blueprints->isNew(recipe.output.singular())) {
      newIndicator->show();
      widget->setLabel("priceLabel", "");
      if (auto icon = widget->fetchChild<ImageWidget>("moneyIcon"))
        icon->setVisibility(false);
    } else {
      newIndicator->hide();
    }
  }

  widget->fetchChild<ItemSlotWidget>("itemIcon")->setItem(item);
  widget->show();
}

PanePtr CraftingPane::setupTooltip(ItemRecipe const& recipe) {
  auto& root = Root::singleton();

  auto tooltip = make_shared<Pane>();
  GuiReader reader;
  reader.construct(root.assets()->json("/interface/craftingtooltip/craftingtooltip.config"), tooltip.get());

  auto guiList = tooltip->fetchChild<ListWidget>("itemList");
  guiList->clear();

  auto normalizedBag = m_player->inventory()->availableItems();

  auto itemDb = root.itemDatabase();

  auto addIngredient = [guiList](ItemPtr const& item, size_t availableCount, size_t requiredCount) {
      auto widget = guiList->addItem();
      widget->fetchChild<LabelWidget>("itemName")->setText(item->friendlyName());
      auto countWidget = widget->fetchChild<LabelWidget>("count");
      countWidget->setText(strf("{}/{}", availableCount, requiredCount));
      if (availableCount < requiredCount)
        countWidget->setColor(Color::Red);
      else
        countWidget->setColor(Color::Green);
      widget->fetchChild<ItemSlotWidget>("itemIcon")->setItem(item);
      widget->show();
    };

  auto currenciesConfig = root.assets()->json("/currencies.config");
  for (auto const& p : recipe.currencyInputs) {
    if (p.second > 0) {
      auto currencyItem = root.itemDatabase()->item(ItemDescriptor(currenciesConfig.get(p.first).getString("representativeItem")));
      addIngredient(currencyItem, m_player->currency(p.first), p.second);
    }
  }

  for (auto const& input : recipe.inputs) {
    auto item = root.itemDatabase()->item(input.singular());
    size_t itemCount = itemDb->getCountOfItem(normalizedBag, input, recipe.matchInputParameters);
    addIngredient(item, itemCount, input.count());
  }

  auto background = tooltip->fetchChild<ImageStretchWidget>("background");
  background->setSize(background->size() + Vec2I(0, guiList->size()[1]));

  auto title = tooltip->fetchChild<LabelWidget>("title");
  title->setPosition(title->position() + Vec2I(0, guiList->size()[1]));

  tooltip->setSize(background->size());

  return tooltip;
}

bool CraftingPane::consumeIngredients(ItemRecipe& recipe, int count) {
  auto itemDb = Root::singleton().itemDatabase();

  auto normalizedBag = m_player->inventory()->availableItems();
  auto availableCurrencies = m_player->inventory()->availableCurrencies();

  // make sure we still have the currencies and items avaialable
  for (auto const& p : recipe.currencyInputs) {
    uint64_t countRequired = p.second * count;
    if (availableCurrencies.value(p.first) < countRequired) {
      updateAvailableRecipes();
      return false;
    }
  }
  for (auto input : recipe.inputs) {
    size_t countRequired = input.count() * count;
    if (itemDb->getCountOfItem(normalizedBag, input, recipe.matchInputParameters) < countRequired) {
      updateAvailableRecipes();
      return false;
    }
  }

  // actually consume the currencies and items
  for (auto const& p : recipe.currencyInputs) {
    if (p.second > 0)
      m_player->inventory()->consumeCurrency(p.first, p.second * count);
  }
  for (auto input : recipe.inputs) {
    if (count > 0)
      m_player->inventory()->consumeItems(ItemDescriptor(input.name(), input.count() * count, input.parameters()), recipe.matchInputParameters);
  }
  return true;
}

void CraftingPane::stopCrafting() {
  if (m_craftingSound)
    m_craftingSound->stop();
  m_crafting = false;
}

void CraftingPane::toggleCraft() {
  if (m_crafting) {
    stopCrafting();
  } else {
    auto recipe = recipeFromSelectedWidget();
    if (recipe.duration > 0 && !m_settings.getBool("disableTimer", false)) {
      m_crafting = true;
      m_craftTimer = GameTimer(recipe.duration);

      if (auto craftingSound = m_settings.optString("craftingSound")) {
        auto assets = Root::singleton().assets();
        m_craftingSound = make_shared<AudioInstance>(*assets->audio(*craftingSound));
        m_craftingSound->setLoops(-1);
        GuiContext::singleton().playAudio(m_craftingSound);
      }
    } else {
      craft(m_count);
    }
  }
}

void CraftingPane::craft(int count) {
  auto& root = Root::singleton();

  if (m_guiList->selectedItem() != NPos) {
    auto recipe = recipeFromSelectedWidget();

    if (!m_player->isAdmin() && !consumeIngredients(recipe, count)) {
      stopCrafting();
      return;
    }

    ItemDescriptor itemDescriptor = recipe.output;
    int remainingItemCount = itemDescriptor.count() * count;
    while (remainingItemCount > 0) {
      auto craftedItem = root.itemDatabase()->item(itemDescriptor.singular().multiply(remainingItemCount));
      remainingItemCount -= craftedItem->count();
      m_player->giveItem(craftedItem);

      for (auto collectable : recipe.collectables)
        m_player->addCollectable(collectable.first, collectable.second);
    }

    m_blueprints->markAsRead(recipe.output.singular());
  }

  updateAvailableRecipes();

  m_count -= count;
  if (m_count <= 0) {
    m_count = 1;
    stopCrafting();
  }
  countChanged();

  updateCraftButtons();
}

void CraftingPane::countTextChanged() {
  if (m_textBox) {
    int appropriateDefaultCount = 1;
    try {
      if (!m_textBox->getText().replace("x", "").size()) {
        m_count = appropriateDefaultCount;
      } else {
        m_count = clamp<int>(lexicalCast<int>(m_textBox->getText().replace("x", "")), appropriateDefaultCount, maxCraft());
        countChanged();
      }
    } catch (BadLexicalCast const&) {
      m_count = appropriateDefaultCount;
      countChanged();
    }
  } else {
    m_count = 1;
  }
}

void CraftingPane::countChanged() {
  if (m_textBox)
    m_textBox->setText(strf("x{}", m_count), false);
}

List<ItemRecipe> CraftingPane::determineRecipes() {
  HashSet<ItemRecipe> recipes;
  auto itemDb = Root::singleton().itemDatabase();

  StringSet categoryFilter;
  if (auto categoriesGroup = fetchChild<ButtonGroupWidget>("categories")) {
    if (auto selectedCategories = categoriesGroup->checkedButton()) {
      for (auto group : selectedCategories->data().getArray("filter"))
        categoryFilter.add(group.toString());
    }
  }

  HashSet<Rarity> rarityFilter;
  if (auto raritiesGroup = fetchChild<ButtonGroupWidget>("rarities")) {
    if (auto selectedRarities = raritiesGroup->checkedButton()) {
      for (auto entry : jsonToStringSet(selectedRarities->data().getArray("rarity")))
        rarityFilter.add(RarityNames.getLeft(entry));
    }
  }

  String filterText;
  if (auto filterWidget = fetchChild<TextBoxWidget>("filter"))
    filterText = filterWidget->getText();

  bool filterHaveMaterials = false;
  if (m_filterHaveMaterials)
    filterHaveMaterials = m_filterHaveMaterials->isChecked();

  if (m_settings.getBool("printer", false)) {
    auto objectDatabase = Root::singleton().objectDatabase();

    StringList itemList;
    if (m_player->isAdmin())
      itemList = objectDatabase->allObjects();
    else
      itemList = StringList::from(m_player->log()->scannedObjects());

    filter(itemList, [objectDatabase, itemDb](String const& itemName) {
        if (objectDatabase->isObject(itemName)) {
          if (auto objectConfig = objectDatabase->getConfig(itemName))
            return objectConfig->printable && itemDb->hasItem(itemName);
        }
        return false;
      });

    float printTime = m_settings.getFloat("printTime", 0);
    float printFactor = m_settings.getFloat("printCostFactor", 1.0);
    for (auto itemName : itemList) {
      ItemRecipe recipe;
      recipe.output = ItemDescriptor(itemName, 1);
      auto recipeItem = itemDb->item(recipe.output);
      int itemPrice = int(recipeItem->price() * printFactor);
      recipe.currencyInputs["money"] = itemPrice;
      recipe.outputRarity = recipeItem->rarity();
      recipe.duration = printTime;
      recipe.guiFilterString = ItemDatabase::guiFilterString(recipeItem);
      recipe.groups = StringSet{objectDatabase->getConfig(itemName)->category};
      recipes.add(recipe);
    }
  } else if (m_settings.contains("recipes")) {
    for (auto entry : m_settings.getArray("recipes")) {
      if (entry.type() == Json::Type::String)
        recipes.addAll(itemDb->recipesForOutputItem(entry.toString()));
      else
        recipes.add(itemDb->parseRecipe(entry));
    }

    if (filterHaveMaterials)
      recipes.addAll(itemDb->recipesFromSubset(m_player->inventory()->availableItems(), m_player->inventory()->availableCurrencies(), take(recipes), m_filter));
  } else {
    if (filterHaveMaterials)
      recipes.addAll(itemDb->recipesFromBagContents(m_player->inventory()->availableItems(), m_player->inventory()->availableCurrencies(), m_filter));
    else
      recipes.addAll(itemDb->allRecipes(m_filter));
  }

  if (!m_player->isAdmin() && m_settings.getBool("requiresBlueprint", true)) {
    auto tempRecipes = take(recipes);
    for (auto const& recipe : tempRecipes) {
      if (m_blueprints->isKnown(recipe.output))
        recipes.add(recipe);
    }
  }

  if (!categoryFilter.empty()) {
    auto temprecipes = take(recipes);
    for (auto const& recipe : temprecipes) {
      if (recipe.groups.hasIntersection(categoryFilter))
        recipes.add(recipe);
    }
  }

  if (!rarityFilter.empty()) {
    auto temprecipes = take(recipes);
    for (auto const& recipe : temprecipes) {
      if (recipe.output) {
        if (rarityFilter.contains(recipe.outputRarity))
          recipes.add(recipe);
      }
    }
  }

  if (!filterText.empty()) {
    auto bits = filterText.toLower().splitAny(" ,.?*\\+/|\t");
    auto temprecipes = take(recipes);
    for (auto const& recipe : temprecipes) {
      if (recipe.output) {
        bool match = true;
        auto guiFilterString = recipe.guiFilterString;
        for (auto const& bit : bits) {
          match &= guiFilterString.contains(bit);
          if (!match)
            break;
        }
        if (match)
          recipes.add(recipe);
      }
    }
  }

  List<ItemRecipe> sortedRecipes = recipes.values();
  auto itemDatabase = Root::singleton().itemDatabase();
  sortByComputedValue(sortedRecipes, [itemDatabase](ItemRecipe const& recipe) {
      return make_tuple(itemDatabase->itemFriendlyName(recipe.output.name()).trim().toLower(), recipe.output.name());
    });

  return sortedRecipes;
}

int CraftingPane::maxCraft() {
  if (m_player->isAdmin())
    return 1000;
  auto itemDb = Root::singleton().itemDatabase();
  int res = 0;
  if (m_guiList->selectedItem() != NPos && m_guiList->selectedItem() < m_recipes.size()) {
    HashMap<ItemDescriptor, uint64_t> normalizedBag = m_player->inventory()->availableItems();
    auto selectedRecipe = recipeFromSelectedWidget();
    res = itemDb->maxCraftableInBag(normalizedBag, m_player->inventory()->availableCurrencies(), selectedRecipe);
    res = std::min(res, 1000);
  }
  return res;
}

ItemRecipe CraftingPane::recipeFromSelectedWidget() const {
  auto pane = m_guiList->selectedWidget();
  if (pane && m_recipesWidgetMap.hasRightValue(pane)) {
    return m_recipesWidgetMap.getLeft(pane);
  }
  return ItemRecipe();
}

}
