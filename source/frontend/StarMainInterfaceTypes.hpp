#pragma once

#include "StarJson.hpp"
#include "StarPoly.hpp"
#include "StarBiMap.hpp"
#include "StarRegisteredPaneManager.hpp"
#include "StarAnimation.hpp"
#include "StarText.hpp"

namespace Star {

STAR_STRUCT(MainInterfaceConfig);

enum class MainInterfacePanes {
  EscapeDialog,
  Inventory,
  Codex,
  Cockpit,
  Tech,
  Songbook,
  Ai,
  Popup,
  Confirmation,
  HttpTrustDialog, // sorry i need it here
  JoinRequest,
  Options,
  QuestLog,
  ActionBar,
  TeamBar,
  StatusPane,
  Chat,
  WireInterface,
  PlanetText,
  RadioMessagePopup,
  CraftingPlain,
  QuestTracker,
  MmUpgrade,
  Collections,
  CharacterSwap
};

extern EnumMap<MainInterfacePanes> const MainInterfacePanesNames;

typedef RegisteredPaneManager<MainInterfacePanes> MainInterfacePaneManager;

struct MainInterfaceConfig {
  static MainInterfaceConfigPtr loadFromAssets();

  TextStyle textStyle;

  String inventoryImage;
  String inventoryImageHover;
  String inventoryImageGlow;
  String inventoryImageGlowHover;
  String inventoryImageOpen;
  String inventoryImageOpenHover;

  String beamDownImage;
  String beamDownImageHover;

  String deployImage;
  String deployImageHover;
  String deployImageDisabled;
  String beamUpImage;
  String beamUpImageHover;

  String craftImage;
  String craftImageHover;
  String craftImageOpen;
  String craftImageOpenHover;

  String codexImage;
  String codexImageHover;
  String codexImageOpen;
  String codexImageHoverOpen;

  String questLogImage;
  String questLogImageHover;
  String questLogImageOpen;
  String questLogImageHoverOpen;

  String mmUpgradeImage;
  String mmUpgradeImageHover;
  String mmUpgradeImageOpen;
  String mmUpgradeImageHoverOpen;
  String mmUpgradeImageDisabled;

  String collectionsImage;
  String collectionsImageHover;
  String collectionsImageOpen;
  String collectionsImageHoverOpen;
  String collectionsImageDisabled;

  Vec2I mainBarInventoryButtonOffset;
  Vec2I mainBarCraftButtonOffset;
  Vec2I mainBarCodexButtonOffset;
  Vec2I mainBarBeamButtonOffset;
  Vec2I mainBarDeployButtonOffset;
  Vec2I mainBarQuestLogButtonOffset;
  Vec2I mainBarMmUpgradeButtonOffset;
  Vec2I mainBarCollectionsButtonOffset;

  PolyI mainBarInventoryButtonPoly;
  PolyI mainBarCraftButtonPoly;
  PolyI mainBarCodexButtonPoly;
  PolyI mainBarBeamButtonPoly;
  PolyI mainBarDeployButtonPoly;
  PolyI mainBarQuestLogButtonPoly;
  PolyI mainBarMmUpgradeButtonPoly;
  PolyI mainBarCollectionsButtonPoly;

  PolyI mainBarPoly;
  Vec2I mainBarSize;

  Vec2I itemCountRightAnchor;
  Vec2I inventoryItemMouseOffset;

  unsigned maxMessageCount;
  String overflowMessageText;

  Vec2I messageBarPos;
  Vec2I messageItemOffset;

  String messageTextContainer;
  Vec2I messageTextContainerOffset;
  Vec2I messageTextOffset;

  float messageTime;
  float messageHideTime;
  Vec2I messageActiveOffset;
  Vec2I messageHiddenOffset;
  Vec2I messageHiddenOffsetBar;
  float messageWindowSpring;
  float monsterHealthBarTime;

  String hungerIcon;

  float planetNameTime;
  float planetNameFadeTime;
  String planetNameFormatString;
  TextStyle planetNameTextStyle;
  Vec2I planetNameOffset;

  bool renderVirtualCursor;
  Json cursorItemSlot;

  Vec2I debugOffset;
  TextStyle debugTextStyle;
  float debugSpatialClearTime;
  float debugMapClearTime;
  Color debugBackgroundColor;
  int debugBackgroundPad;

  StringMap<StringList> macroCommands;
};

}
