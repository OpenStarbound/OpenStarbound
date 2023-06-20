#include "StarMainInterfaceTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarImageMetadataDatabase.hpp"

namespace Star {

MainInterfaceConfigPtr MainInterfaceConfig::loadFromAssets() {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto imageMetadata = root.imageMetadataDatabase();

  auto config = make_shared<MainInterfaceConfig>();

  config->fontSize = assets->json("/interface.config:font.baseSize").toInt();

  config->inventoryImage = assets->json("/interface.config:mainBar.inventory.base").toString();
  config->inventoryImageHover = assets->json("/interface.config:mainBar.inventory.hover").toString();
  config->inventoryImageGlow = assets->json("/interface.config:mainBar.inventory.glow").toString();
  config->inventoryImageGlowHover = assets->json("/interface.config:mainBar.inventory.glowHover").toString();
  config->inventoryImageOpen = assets->json("/interface.config:mainBar.inventory.open").toString();
  config->inventoryImageOpenHover = assets->json("/interface.config:mainBar.inventory.openHover").toString();

  config->beamDownImage = assets->json("/interface.config:mainBar.beam.base").toString();
  config->beamDownImageHover = assets->json("/interface.config:mainBar.beam.hover").toString();

  config->deployImage = assets->json("/interface.config:mainBar.deploy.base").toString();
  config->deployImageHover = assets->json("/interface.config:mainBar.deploy.hover").toString();
  config->deployImageDisabled = assets->json("/interface.config:mainBar.deploy.disabled").toString();
  config->beamUpImage = assets->json("/interface.config:mainBar.beamUp.base").toString();
  config->beamUpImageHover = assets->json("/interface.config:mainBar.beamUp.hover").toString();

  config->craftImage = assets->json("/interface.config:mainBar.craft.base").toString();
  config->craftImageHover = assets->json("/interface.config:mainBar.craft.hover").toString();
  config->craftImageOpen = assets->json("/interface.config:mainBar.craft.open").toString();
  config->craftImageOpenHover = assets->json("/interface.config:mainBar.craft.openHover").toString();

  config->codexImage = assets->json("/interface.config:mainBar.codex.base").toString();
  config->codexImageHover = assets->json("/interface.config:mainBar.codex.hover").toString();
  config->codexImageOpen = assets->json("/interface.config:mainBar.codex.open").toString();
  config->codexImageHoverOpen = assets->json("/interface.config:mainBar.codex.openHover").toString();

  config->questLogImage = assets->json("/interface.config:mainBar.questLog.base").toString();
  config->questLogImageHover = assets->json("/interface.config:mainBar.questLog.hover").toString();
  config->questLogImageOpen = assets->json("/interface.config:mainBar.questLog.open").toString();
  config->questLogImageHoverOpen = assets->json("/interface.config:mainBar.questLog.openHover").toString();

  config->mmUpgradeImage = assets->json("/interface.config:mainBar.mmUpgrade.base").toString();
  config->mmUpgradeImageHover = assets->json("/interface.config:mainBar.mmUpgrade.hover").toString();
  config->mmUpgradeImageOpen = assets->json("/interface.config:mainBar.mmUpgrade.open").toString();
  config->mmUpgradeImageHoverOpen = assets->json("/interface.config:mainBar.mmUpgrade.openHover").toString();
  config->mmUpgradeImageDisabled = assets->json("/interface.config:mainBar.mmUpgrade.disabled").toString();

  config->collectionsImage = assets->json("/interface.config:mainBar.collections.base").toString();
  config->collectionsImageHover = assets->json("/interface.config:mainBar.collections.hover").toString();
  config->collectionsImageOpen = assets->json("/interface.config:mainBar.collections.open").toString();
  config->collectionsImageHoverOpen = assets->json("/interface.config:mainBar.collections.openHover").toString();

  config->mainBarInventoryButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.inventory.pos"));
  config->mainBarCraftButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.craft.pos"));
  config->mainBarBeamButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.beam.pos"));
  config->mainBarDeployButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.deploy.pos"));
  config->mainBarCodexButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.codex.pos"));
  config->mainBarQuestLogButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.questLog.pos"));
  config->mainBarMmUpgradeButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.mmUpgrade.pos"));
  config->mainBarCollectionsButtonOffset = jsonToVec2I(assets->json("/interface.config:mainBar.collections.pos"));

  config->mainBarInventoryButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.inventory.poly"));
  config->mainBarCraftButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.craft.poly"));
  config->mainBarBeamButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.beam.poly"));
  config->mainBarDeployButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.deploy.poly"));
  config->mainBarCodexButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.codex.poly"));
  config->mainBarQuestLogButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.questLog.poly"));
  config->mainBarMmUpgradeButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.mmUpgrade.poly"));
  config->mainBarCollectionsButtonPoly = jsonToPolyI(assets->json("/interface.config:mainBar.collections.poly"));

  config->mainBarSize = jsonToVec2I(assets->json("/interface.config:mainBar.size"));

  config->itemCountRightAnchor = jsonToVec2I(assets->json("/interface.config:itemCountRightAnchor"));
  config->inventoryItemMouseOffset = jsonToVec2I(assets->json("/interface.config:inventoryItemMouseOffset"));

  config->maxMessageCount = assets->json("/interface.config:maxMessageCount").toUInt();
  config->overflowMessageText = assets->json("/interface.config:overflowMessageText").toString();

  config->messageBarPos = jsonToVec2I(assets->json("/interface.config:message.barPos"));
  config->messageItemOffset = jsonToVec2I(assets->json("/interface.config:message.itemOffset"));

  config->messageTextContainer = assets->json("/interface.config:message.textContainer").toString();
  config->messageTextContainerOffset = jsonToVec2I(assets->json("/interface.config:message.textContainerOffset"));
  config->messageTextOffset = jsonToVec2I(assets->json("/interface.config:message.textOffset"));

  config->messageTime = assets->json("/interface.config:message.showTime").toFloat();
  config->messageHideTime = assets->json("/interface.config:message.hideTime").toFloat();
  config->messageActiveOffset = jsonToVec2I(assets->json("/interface.config:message.offset"));
  config->messageHiddenOffset = jsonToVec2I(assets->json("/interface.config:message.offsetHidden"));
  config->messageHiddenOffsetBar = jsonToVec2I(assets->json("/interface.config:message.offsetHiddenBar"));
  config->messageWindowSpring = assets->json("/interface.config:message.windowSpring").toFloat();

  config->monsterHealthBarTime = assets->json("/interface.config:monsterHealth.showTime").toFloat();

  config->hungerIcon = assets->json("/interface.config:hungerIcon").toString();

  config->planetNameTime = assets->json("/interface.config:planetNameTime").toFloat();
  config->planetNameFadeTime = assets->json("/interface.config:planetNameFadeTime").toFloat();
  config->planetNameFormatString = assets->json("/interface.config:planetNameFormatString").toString();
  config->planetNameFontSize = assets->json("/interface.config:font.planetSize").toInt();
  config->planetNameDirectives = assets->json("/interface.config:planetNameDirectives").toString();
  config->planetNameOffset = jsonToVec2I(assets->json("/interface.config:planetTextOffset"));

  config->renderVirtualCursor = assets->json("/interface.config:renderVirtualCursor").toBool();
  config->cursorItemSlot = assets->json("/interface.config:cursorItemSlot");

  config->debugOffset = jsonToVec2I(assets->json("/interface.config:debugOffset"));
  config->debugFontSize = assets->json("/interface.config:debugFontSize").toUInt();
  config->debugSpatialClearTime = assets->json("/interface.config:debugSpatialClearTime").toFloat();
  config->debugMapClearTime = assets->json("/interface.config:debugMapClearTime").toFloat();
  config->debugBackgroundColor = jsonToColor(assets->json("/interface.config:debugBackgroundColor"));
  config->debugBackgroundPad = assets->json("/interface.config:debugBackgroundPad").toUInt();

  for (auto const& path : assets->scanExtension("macros")) {
    for (auto const& pair : assets->json(path).iterateObject())
      config->macroCommands.add(pair.first, jsonToStringList(pair.second));
  }

  return config;
}

}
