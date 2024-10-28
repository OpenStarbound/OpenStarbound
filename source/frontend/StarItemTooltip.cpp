#include "StarItemTooltip.hpp"
#include "StarGuiReader.hpp"
#include "StarPane.hpp"
#include "StarListWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarRoot.hpp"
#include "StarStoredFunctions.hpp"
#include "StarObjectItem.hpp"
#include "StarImageWidget.hpp"
#include "StarItemSlotWidget.hpp"
#include "StarPreviewableItem.hpp"
#include "StarFireableItem.hpp"
#include "StarStatusEffectItem.hpp"
#include "StarObject.hpp"
#include "StarLogging.hpp"
#include "StarAssets.hpp"
#include "StarObjectDatabase.hpp"
#include "StarStatusEffectDatabase.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

PanePtr ItemTooltipBuilder::buildItemTooltip(ItemPtr const& item, PlayerPtr const& viewer) {
  if (!item) {
    return {};
  } else {
    PanePtr tooltip = make_shared<Pane>();
    tooltip->removeAllChildren();

    String title;
    String subTitle;

    String tooltipKind = item->tooltipKind();

    if (tooltipKind.empty())
      tooltipKind = "base";
    if (!tooltipKind.endsWith(".tooltip"))
      tooltipKind = "/interface/tooltips/" + tooltipKind + ".tooltip";

    buildItemDescriptionInner(tooltip, item, tooltipKind, title, subTitle, viewer);

    auto titleIcon = make_shared<ItemSlotWidget>(item, "/interface/inventory/portrait.png");
    titleIcon->setBackingImageAffinity(true, true);
    titleIcon->showRarity(false);
    tooltip->setTitle(titleIcon, title, subTitle);

    return tooltip;
  }
}

void ItemTooltipBuilder::buildItemDescription(WidgetPtr const& container, ItemPtr const& item) {
  String tooltipKind = item->tooltipKind();

  if (tooltipKind.empty())
    tooltipKind = "base";
  if (!tooltipKind.endsWith(".itemdescription"))
    tooltipKind = "/interface/itemdescriptions/" + tooltipKind + ".itemdescription";

  String title;
  String subTitle;
  buildItemDescriptionInner(container, item, tooltipKind, title, subTitle);
}

String categoryDisplayName(String const& category) {
  Json categories = Root::singleton().assets()->json("/items/categories.config:labels");
  return categories.getString(category, category);
}

void ItemTooltipBuilder::buildItemDescriptionInner(
    WidgetPtr const& container, ItemPtr const& item, String const& tooltipKind, String& title, String& subTitle, PlayerPtr const& viewer) {
  GuiReader reader;
  auto& root = Root::singleton();

  title = item->friendlyName();
  subTitle = categoryDisplayName(item->category());
  String description = item->description();

  reader.construct(root.assets()->json(tooltipKind), container.get());

  if (container->containsChild("icon"))
    container->fetchChild<ItemSlotWidget>("icon")->setItem(item);

  container->setLabel("nameLabel", item->name());
  container->setLabel("countLabel", toString(item->count()));

  container->setLabel("rarityLabel", RarityNames.getRight(item->rarity()).titleCase());

  if (item->twoHanded())
    container->setLabel("handednessLabel", "2-Handed");
  else
    container->setLabel("handednessLabel", "1-Handed");

  container->setLabel("countLabel", toString(item->instanceValue("fuelAmount", 0).toUInt() * item->count()));
  container->setLabel("priceLabel", toString((int)item->price()));

  if (auto objectItem = as<ObjectItem>(item)) {
    try {
      auto object = Root::singleton().objectDatabase()->createObject(objectItem->objectName(), objectItem->objectParameters());

      if (container->containsChild("objectImage")) {
        auto drawables = object->cursorHintDrawables();
        container->fetchChild<ImageWidget>("objectImage")->setDrawables(drawables);
      }

      if (objectItem->tooltipKind() == "container")
        container->setLabel("slotCountLabel", strf("Holds {} Items", objectItem->instanceValue("slotCount")));

      title = object->shortDescription();
      subTitle = categoryDisplayName(object->category());
      description = object->description();
    } catch (StarException const& e) {
      Logger::error("Failed to instantiate object for object item tooltip. {}", outputException(e, false));
    }
  } else {
    if (container->containsChild("objectImage")) {
      if (auto previewable = as<PreviewableItem>(item)) {
        container->fetchChild<ImageWidget>("objectImage")->setDrawables(previewable->preview(viewer));
      } else {
        auto drawables = item->iconDrawables();
        container->fetchChild<ImageWidget>("objectImage")->setDrawables(drawables);
      }
    }
  }

  auto tooltipFields = item->instanceValue("tooltipFields", JsonObject());
  for (auto const& pair : tooltipFields.iterateObject()) {
    if (pair.first.equalsIgnoreCase("subtitle"))
      subTitle = pair.second.toString();
    if (pair.first.endsWith("Label"))
      container->setLabel(pair.first, pair.second.type() == Json::Type::String ? pair.second.toString() : toString(pair.second));
    if (pair.first.endsWith("Image") && container->containsChild(pair.first)) {
      if (pair.second.isType(Json::Type::String))
        container->fetchChild<ImageWidget>(pair.first)->setImage(pair.second.toString());
      else
        container->fetchChild<ImageWidget>(pair.first)->setDrawables(pair.second.toArray().transformed(construct<Drawable>()));
    }
  }

  if (auto fireable = as<FireableItem>(item)) {
    container->setLabel("cooldownTimeLabel", strf("{:.2f}", fireable->cooldownTime()));
    container->setLabel("windupTimeLabel", strf("{:.2f}", fireable->windupTime()));
    container->setLabel("speedLabel", strf("{:.2f}", 1.0f / (fireable->cooldownTime() + fireable->windupTime())));
  }

  if (container->containsChild("largeImage")) {
    container->fetchChild<ImageWidget>("largeImage")->setImage(item->largeImage());
  }

  container->setLabel("descriptionLabel", description);
  container->setLabel("friendlyNameLabel", title);

  if (container->containsChild("statusList")) {
    auto statusList = container->fetchChild<ListWidget>("statusList");
    if (auto statusEffects = as<StatusEffectItem>(item)) {
      for (auto effect : statusEffects->statusEffects())
        describePersistentEffect(statusList, effect);
    }
  }

  if (item->instanceValue("acceptsAugmentType", false)) {
    if (auto augmentLabel = container->fetchChild<LabelWidget>("augmentNameLabel")) {
      if (auto currentAugment = item->instanceValue("currentAugment")) {
        container->setLabel("augmentNameLabel", currentAugment.getString("displayName", "???"));
        if (auto augmentIcon = container->fetchChild<ImageWidget>("augmentIconImage"))
          augmentIcon->setImage(currentAugment.getString("displayIcon", ""));
        augmentLabel->setColor(Color::White);
      } else {
        container->setLabel("augmentNameLabel", "NO AUGMENT INSERTED");
        if (auto augmentIcon = container->fetchChild<ImageWidget>("augmentIconImage"))
          augmentIcon->setImage("");
        augmentLabel->setColor(Color::Gray);
      }
    }
  }

  container->setLabel("title", title);
  container->setLabel("subTitle", subTitle);
  if (container->containsChild("titleIcon")) {
    auto titleIcon = container->fetchChild<ItemSlotWidget>("titleIcon");
    titleIcon->setItem(item);
  }
}

void ItemTooltipBuilder::describePersistentEffect(
    ListWidgetPtr const& container, PersistentStatusEffect const& effect) {
  if (auto uniqueStatusEffect = effect.ptr<UniqueStatusEffect>()) {
    auto statusEffectDatabase = Root::singleton().statusEffectDatabase();
    auto effectConfig = statusEffectDatabase->uniqueEffectConfig(*uniqueStatusEffect);
    if (effectConfig.icon) {
      auto listItem = container->addItem();
      listItem->setLabel("statusLabel", effectConfig.label);
      listItem->fetchChild<ImageWidget>("statusImage")->setImage(*effectConfig.icon);
    }
  } else if (auto modifierEffect = effect.ptr<StatModifier>()) {
    auto statsConfig = Root::singleton().assets()->json("/interface/stats/stats.config");
    if (auto baseMultiplier = modifierEffect->ptr<StatBaseMultiplier>()) {
      if (statsConfig.contains(baseMultiplier->statName)) {
        auto listItem = container->addItem();
        listItem->fetchChild<ImageWidget>("statusImage")
            ->setImage(statsConfig.get(baseMultiplier->statName).getString("icon"));
        listItem->setLabel("statusLabel", strf("{:.1f}%", (baseMultiplier->baseMultiplier - 1) * 100));
      }
    } else if (auto valueModifier = modifierEffect->ptr<StatValueModifier>()) {
      if (statsConfig.contains(valueModifier->statName)) {
        auto listItem = container->addItem();
        listItem->fetchChild<ImageWidget>("statusImage")
            ->setImage(statsConfig.get(valueModifier->statName).getString("icon"));
        listItem->setLabel("statusLabel", strf("{}{:.2f}", valueModifier->value < 0 ? "-" : "", valueModifier->value));
      }
    } else if (auto effectiveMultiplier = modifierEffect->ptr<StatEffectiveMultiplier>()) {
      if (statsConfig.contains(effectiveMultiplier->statName)) {
        auto listItem = container->addItem();
        listItem->fetchChild<ImageWidget>("statusImage")
            ->setImage(statsConfig.get(effectiveMultiplier->statName).getString("icon"));
        listItem->setLabel("statusLabel", strf("{:.1f}%", (effectiveMultiplier->effectiveMultiplier - 1) * 100));
      }
    }
  }
}

}
