#include "StarUnlockItem.hpp"
#include "StarPlayer.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarClientContext.hpp"
#include "StarPlayerBlueprints.hpp"

namespace Star {

UnlockItem::UnlockItem(Json const& config, String const& directory, Json const& itemParameters)
  : Item(config, directory, itemParameters), SwingableItem(config) {
  m_tierRecipesUnlock = instanceValue("tierRecipesUnlock").optString();
  m_shipUpgrade = instanceValue("shipUpgrade").optUInt();
  m_unlockMessage = instanceValue("unlockMessage").optString().value();
  auto image = AssetPath::relativeTo(directory, instanceValue("image").toString());
  m_drawables = {Drawable::makeImage(image, 1.0f / TilePixels, true, Vec2F())};
}

ItemPtr UnlockItem::clone() const {
  return make_shared<UnlockItem>(*this);
}

List<Drawable> UnlockItem::drawables() const {
  return m_drawables;
}

List<Drawable> UnlockItem::preview(PlayerPtr const& viewer) const {
  return iconDrawables();
}

void UnlockItem::fireTriggered() {
  if (!initialized())
    throw ItemException("Item not init'd properly, or user not recognized as Tool User.");

  // Only the player can use an unlock item, for any other entity it should do
  // nothing.
  if (auto player = as<Player>(owner())) {
    if (instanceValue("consume", true).toBool() && !consume(1))
      return;

    if (auto clientContext = player->clientContext()) {
      if (m_shipUpgrade)
        clientContext->rpcInterface()->invokeRemote("ship.applyShipUpgrades", JsonObject{{"shipLevel", *m_shipUpgrade}});
    }

    if (!m_unlockMessage.empty()) {
      JsonObject message;
      message["message"] = m_unlockMessage;
      owner()->interact(InteractAction(InteractActionType::ShowPopup, owner()->entityId(), message));
    }

    if (m_tierRecipesUnlock) {
      auto playerConfig = Root::singleton().assets()->json("/player.config");

      List<ItemDescriptor> blueprints;
      for (Json v : playerConfig.get("defaultBlueprints", JsonObject()).getArray(*m_tierRecipesUnlock, JsonArray()))
        blueprints.append(ItemDescriptor(v));

      auto speciesConfig = Root::singleton().assets()->json(strf("/species/%s.species", player->species()));
      for (Json v : speciesConfig.get("defaultBlueprints", JsonObject()).getArray(*m_tierRecipesUnlock, JsonArray()))
        blueprints.append(ItemDescriptor(v));

      for (auto b : blueprints)
        player->addBlueprint(b);
    }
  }
}

}
