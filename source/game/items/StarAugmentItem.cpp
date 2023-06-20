#include "StarAugmentItem.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarItemDatabase.hpp"
#include "StarLuaComponents.hpp"
#include "StarItemLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

AugmentItem::AugmentItem(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters) {}

AugmentItem::AugmentItem(AugmentItem const& rhs) : AugmentItem(rhs.config(), rhs.directory(), rhs.parameters()) {}

ItemPtr AugmentItem::clone() const {
  return make_shared<AugmentItem>(*this);
}

StringList AugmentItem::augmentScripts() const {
  return jsonToStringList(instanceValue("scripts")).transformed(bind(&AssetPath::relativeTo, directory(), _1));
}

ItemPtr AugmentItem::applyTo(ItemPtr const item) {
  return Root::singleton().itemDatabase()->applyAugment(item, this);
}

}
