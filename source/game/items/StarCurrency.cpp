#include "StarCurrency.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

CurrencyItem::CurrencyItem(Json const& config, String const& directory) : Item(config, directory) {
  m_currency = config.getString("currency");
  m_value = config.getUInt("value");
}

ItemPtr CurrencyItem::clone() const {
  return make_shared<CurrencyItem>(*this);
}

String CurrencyItem::pickupSound() const {
  if (count() <= instanceValue("smallStackLimit", 100).toUInt()) {
    if (!instanceValue("pickupSoundsSmall", {}).isNull())
      return Random::randFrom(jsonToStringSet(instanceValue("pickupSoundsSmall")));
  } else if (count() <= instanceValue("mediumStackLimit", 10000).toUInt()) {
    if (!instanceValue("pickupSoundsMedium", {}).isNull())
      return Random::randFrom(jsonToStringSet(instanceValue("pickupSoundsMedium")));
  } else {
    if (!instanceValue("pickupSoundsLarge", {}).isNull())
      return Random::randFrom(jsonToStringSet(instanceValue("pickupSoundsLarge")));
  }
  return Item::pickupSound();
}

String CurrencyItem::currencyType() {
  return m_currency;
}

uint64_t CurrencyItem::currencyValue() {
  return m_value;
}

uint64_t CurrencyItem::totalValue() {
  return m_value * count();
}

}
