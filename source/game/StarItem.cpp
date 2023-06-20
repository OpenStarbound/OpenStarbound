#include "StarItem.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarLogging.hpp"
#include "StarWorldLuaBindings.hpp"

namespace Star {

Item::Item(Json config, String directory, Json parameters) {
  m_config = move(config);
  m_directory = move(directory);
  m_parameters = move(parameters);
  m_name = m_config.getString("itemName");
  m_count = 1;

  m_maxStack = instanceValue("maxStack", Root::singleton().assets()->json("/items/defaultParameters.config:defaultMaxStack").toInt()).toInt();
  m_shortDescription = instanceValue("shortdescription", "").toString();
  m_description = instanceValue("description", "").toString();

  m_rarity = RarityNames.getLeft(instanceValue("rarity").toString());

  auto inventoryIcon = instanceValue("inventoryIcon", Root::singleton().assets()->json("/items/defaultParameters.config:missingIcon"));
  if (inventoryIcon.type() == Json::Type::Array) {
    List<Drawable> iconDrawables;
    for (auto const& drawable : inventoryIcon.toArray()) {
      auto image = AssetPath::relativeTo(m_directory, drawable.getString("image"));
      Vec2F position = jsonToVec2F(drawable.getArray("position", {0, 0}));
      iconDrawables.append(Drawable::makeImage(image, 1.0f, true, position));
    }
    setIconDrawables(move(iconDrawables));
  } else {
    auto image = AssetPath::relativeTo(m_directory, inventoryIcon.toString());
    setIconDrawables({Drawable::makeImage(image, 1.0f, true, Vec2F())});
  }

  auto assets = Root::singleton().assets();
  m_twoHanded = instanceValue("twoHanded", false).toBool();
  m_price = instanceValue("price", assets->json("/items/defaultParameters.config:defaultPrice")).toInt();
  m_tooltipKind = instanceValue("tooltipKind", "").toString();
  auto largeImage = instanceValue("largeImage");
  if (!largeImage.isNull())
    m_largeImage = AssetPath::relativeTo(m_directory, largeImage.toString());

  m_category = instanceValue("category", "").toString();
  m_pickupSounds = jsonToStringSet(m_config.get("pickupSounds", JsonArray{}));
  if (!m_pickupSounds.size())
    m_pickupSounds = jsonToStringSet(assets->json("/items/defaultParameters.config:pickupSounds"));

  m_timeToLive = instanceValue("timeToLive", Root::singleton().assets()->json("/items/defaultParameters.config:defaultTimeToLive").toFloat()).toFloat();

  for (auto b : jsonToStringList(instanceValue("learnBlueprintsOnPickup", JsonArray{})))
    m_learnBlueprintsOnPickup.append(ItemDescriptor(b));

  for (auto pair : instanceValue("collectablesOnPickup", JsonObject{}).iterateObject())
    m_collectablesOnPickup[pair.first] = pair.second.toString();
}

Item::~Item() {}

String Item::name() const {
  return m_name;
}

uint64_t Item::count() const {
  return m_count;
}

uint64_t Item::setCount(uint64_t count, bool overfill) {
  if (overfill)
    m_count = count;
  else
    m_count = std::min(count, m_maxStack);
  return count - m_count;
}

bool Item::stackableWith(ItemConstPtr const& item) const {
  return item && name() == item->name() && parameters() == item->parameters();
}

uint64_t Item::maxStack() const {
  return m_maxStack;
}

uint64_t Item::couldStack(ItemConstPtr const& item) const {
  if (stackableWith(item) && m_count < m_maxStack) {
    uint64_t take = m_maxStack - m_count;
    return std::min(take, item->count());
  } else {
    return 0;
  }
}

bool Item::stackWith(ItemPtr const& item) {
  uint64_t take = couldStack(item);

  if (take > 0 && item->consume(take)) {
    m_count += take;
    return true;
  } else {
    return false;
  }
}

bool Item::matches(ItemDescriptor const& descriptor, bool exactMatch) const {
  return descriptor.name() == m_name && (!exactMatch || descriptor.parameters() == m_parameters);
}

bool Item::matches(ItemConstPtr const& other, bool exactMatch) const {
  return other->name() == m_name && (!exactMatch || other->parameters() == m_parameters);
}

bool Item::consume(uint64_t count) {
  if (m_count >= count) {
    m_count -= count;
    return true;
  } else {
    return false;
  }
}

ItemPtr Item::take(uint64_t max) {
  uint64_t takeCount = std::min(m_count, max);
  if (takeCount != 0) {
    if (auto newItems = clone()) {
      m_count -= takeCount;
      newItems->setCount(takeCount);
      return newItems;
    } else {
      Logger::warn(strf("Could not clone %s, not moving %d items as requested.", friendlyName(), takeCount).c_str());
    }
  }

  return {};
}

bool Item::empty() const {
  return m_count == 0;
}

ItemDescriptor Item::descriptor() const {
  return ItemDescriptor(m_name, m_count, m_parameters);
}

String Item::description() const {
  return m_description;
}

String Item::friendlyName() const {
  return m_shortDescription;
}

Rarity Item::rarity() const {
  return m_rarity;
}

List<Drawable> Item::iconDrawables() const {
  return m_iconDrawables;
}

List<Drawable> Item::dropDrawables() const {
  auto drawables = iconDrawables();
  Drawable::scaleAll(drawables, 1.0f / TilePixels);
  return drawables;
}

bool Item::twoHanded() const {
  return m_twoHanded;
}

float Item::timeToLive() const {
  return m_timeToLive;
}

uint64_t Item::price() const {
  return m_price * count();
}

String Item::tooltipKind() const {
  return m_tooltipKind;
}

String Item::largeImage() const {
  return m_largeImage;
}

String Item::category() const {
  return m_category;
}

String Item::pickupSound() const {
  return Random::randFrom(m_pickupSounds);
}

void Item::setMaxStack(uint64_t maxStack) {
  m_maxStack = maxStack;
}

void Item::setDescription(String const& description) {
  m_description = description;
}

void Item::setRarity(Rarity rarity) {
  m_rarity = rarity;
}

void Item::setPrice(uint64_t price) {
  m_price = price;
}

void Item::setIconDrawables(List<Drawable> drawables) {
  m_iconDrawables = move(drawables);
  auto boundBox = Drawable::boundBoxAll(m_iconDrawables, true);
  if (!boundBox.isEmpty()) {
    for (auto& drawable : m_iconDrawables)
      drawable.translate(-boundBox.center());
    // TODO: Why 16?  Is this the size of the icon container?  Shouldn't this
    // be configurable?
    float zoom = 16.0f / std::max(boundBox.width(), boundBox.height());
    if (zoom < 1) {
      for (auto& drawable : m_iconDrawables)
        drawable.scale(zoom);
    }
  }
}

void Item::setTwoHanded(bool twoHanded) {
  m_twoHanded = twoHanded;
}

void Item::setTimeToLive(float timeToLive) {
  m_timeToLive = timeToLive;
}

List<QuestArcDescriptor> Item::pickupQuestTemplates() const {
  return instanceValue("pickupQuestTemplates", JsonArray{}).toArray().transformed(&QuestArcDescriptor::fromJson);
}

StringSet Item::itemTags() const {
  return jsonToStringSet(m_config.get("itemTags", JsonArray{}));
}

bool Item::hasItemTag(String const& itemTag) const {
  return itemTags().contains(itemTag);
}

Json Item::instanceValue(String const& name, Json const& def) const {
  return jsonMergeQueryDef(name, def, m_config, m_parameters);
}

Json Item::instanceValues() const {
  return m_config.setAll(m_parameters.toObject());
}

Json Item::config() const {
  return m_config;
}

Json Item::parameters() const {
  return m_parameters;
}

void Item::setInstanceValue(String const& name, Json const& value) {
  if (m_parameters.get(name, {}) != value)
    m_parameters = m_parameters.setAll(JsonObject{{name, value}});
}

String const& Item::directory() const {
  return m_directory;
}

List<ItemDescriptor> Item::learnBlueprintsOnPickup() const {
  return m_learnBlueprintsOnPickup;
}

StringMap<String> Item::collectablesOnPickup() const {
  return m_collectablesOnPickup;
}

GenericItem::GenericItem(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters) {}

ItemPtr GenericItem::clone() const {
  return make_shared<GenericItem>(*this);
}

}
