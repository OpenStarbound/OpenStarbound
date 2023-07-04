#ifndef STAR_MAIN_INTERFACE_HPP
#define STAR_MAIN_INTERFACE_HPP

#include "StarInventory.hpp"
#include "StarInteractionTypes.hpp"
#include "StarItemDescriptor.hpp"
#include "StarGameTypes.hpp"
#include "StarInterfaceCursor.hpp"
#include "StarMainInterfaceTypes.hpp"
#include "StarWarping.hpp"

namespace Star {

STAR_CLASS(UniverseClient);
STAR_CLASS(WorldPainter);
STAR_CLASS(Item);
STAR_CLASS(Chat);
STAR_CLASS(ClientCommandProcessor);
STAR_CLASS(OptionsMenu);
STAR_CLASS(WirePane);
STAR_CLASS(ActionBar);
STAR_CLASS(TeamBar);
STAR_CLASS(StatusPane);
STAR_CLASS(ContainerPane);
STAR_CLASS(CraftingPane);
STAR_CLASS(MerchantPane);
STAR_CLASS(CodexInterface);
STAR_CLASS(SongbookInterface);
STAR_CLASS(QuestLogInterface);
STAR_CLASS(AiInterface);
STAR_CLASS(PopupInterface);
STAR_CLASS(ConfirmationDialog);
STAR_CLASS(JoinRequestDialog);
STAR_CLASS(TeleportDialog);
STAR_CLASS(LabelWidget);
STAR_CLASS(Cinematic);
STAR_CLASS(NameplatePainter);
STAR_CLASS(QuestIndicatorPainter);
STAR_CLASS(RadioMessagePopup);
STAR_CLASS(Quest);
STAR_CLASS(QuestTrackerPane);
STAR_CLASS(ContainerInteractor);
STAR_CLASS(ScriptPane);
STAR_CLASS(ChatBubbleManager);
STAR_CLASS(CanvasWidget);

STAR_STRUCT(GuiMessage);
STAR_CLASS(MainInterface);

struct GuiMessage {
  GuiMessage();
  GuiMessage(String const& message, float cooldown);

  String message;
  float cooldown;
  float springState;
};

class MainInterface {
public:
  enum RunningState {
    Running,
    ReturnToTitle
  };

  MainInterface(UniverseClientPtr client, WorldPainterPtr painter, CinematicPtr cinematicOverlay);

  ~MainInterface();

  RunningState currentState() const;

  MainInterfacePaneManager* paneManager();

  bool escapeDialogOpen() const;

  void openCraftingWindow(Json const& config, EntityId sourceEntityId = NullEntityId);
  void openMerchantWindow(Json const& config, EntityId sourceEntityId = NullEntityId);
  void togglePlainCraftingWindow();

  bool windowsOpen() const;

  MerchantPanePtr activeMerchantPane() const;

  // Return true if this event was consumed or should be handled elsewhere.
  bool handleInputEvent(InputEvent const& event);
  // Return true if mouse / keyboard events are currently locked here
  bool inputFocus() const;
  // If input is focused, should MainInterface also accept text input events?
  bool textInputActive() const;

  void handleInteractAction(InteractAction interactAction);

  // Handles incoming client messages, aims main player, etc.
  void update();

  // Render things e.g. quest indicators that should be drawn in the world
  // behind interface e.g. chat bubbles
  void renderInWorldElements();
  void render();

  Vec2F cursorWorldPosition() const;

  void toggleDebugDisplay();
  bool isDebugDisplayed();

  void doChat(String const& chat, bool addToHistory);

  void queueMessage(String const& message);
  void queueItemPickupText(ItemPtr const& item);
  void queueJoinRequest(pair<String, RpcPromiseKeeper<P2PJoinRequestReply>> request);

  bool fixedCamera() const;

  void warpToOrbitedWorld(bool deploy = false);
  void warpToOwnShip();
  void warpTo(WarpAction const& warpAction);

  CanvasWidgetPtr fetchCanvas(String const& canvasName);

private:
  PanePtr createEscapeDialog();

  float interfaceScale() const;
  unsigned windowHeight() const;
  unsigned windowWidth() const;
  Vec2I mainBarPosition() const;

  void renderBreath();
  void renderMessages();
  void renderMonsterHealthBar();
  void renderSpecialDamageBar();
  void renderMainBar();
  void renderWindows();
  void renderDebug();

  void updateCursor();
  void renderCursor();

  bool overButton(PolyI buttonPoly, Vec2I const& mousePos) const;

  bool overlayClick(Vec2I const& mousePos, MouseButton mouseButton);

  GuiContext* m_guiContext;
  MainInterfaceConfigConstPtr m_config;
  InterfaceCursor m_cursor;

  RunningState m_state;

  UniverseClientPtr m_client;
  WorldPainterPtr m_worldPainter;
  CinematicPtr m_cinematicOverlay;

  MainInterfacePaneManager m_paneManager;

  QuestLogInterfacePtr m_questLogInterface;

  InventoryPanePtr m_inventoryWindow;
  CraftingPanePtr m_plainCraftingWindow;
  CraftingPanePtr m_craftingWindow;
  MerchantPanePtr m_merchantWindow;
  CodexInterfacePtr m_codexInterface;
  OptionsMenuPtr m_optionsMenu;
  ContainerPanePtr m_containerPane;
  PopupInterfacePtr m_popupInterface;
  ConfirmationDialogPtr m_confirmationDialog;
  JoinRequestDialogPtr m_joinRequestDialog;
  TeleportDialogPtr m_teleportDialog;
  QuestTrackerPanePtr m_questTracker;
  ScriptPanePtr m_mmUpgrade;
  ScriptPanePtr m_collections;
  Map<EntityId, PanePtr> m_interactionScriptPanes;

  StringMap<CanvasWidgetPtr> m_canvases;

  ChatPtr m_chat;
  ClientCommandProcessorPtr m_clientCommandProcessor;
  RadioMessagePopupPtr m_radioMessagePopup;
  WirePanePtr m_wireInterface;

  ActionBarPtr m_actionBar;
  Vec2I m_cursorScreenPos;
  ItemSlotWidgetPtr m_cursorItem;
  Maybe<String> m_cursorTooltip;

  LabelWidgetPtr m_planetText;
  GameTimer m_planetNameTimer;

  GameTimer m_debugSpatialClearTimer;
  GameTimer m_debugMapClearTimer;
  RectF m_debugTextRect;

  NameplatePainterPtr m_nameplatePainter;
  QuestIndicatorPainterPtr m_questIndicatorPainter;
  ChatBubbleManagerPtr m_chatBubbleManager;

  bool m_disableHud;

  String m_lastCommand;

  LinkedList<GuiMessagePtr> m_messages;
  HashMap<ItemDescriptor, std::pair<size_t, GuiMessagePtr>> m_itemDropMessages;
  unsigned m_messageOverflow;
  GuiMessagePtr m_overflowMessage;

  List<pair<String, RpcPromiseKeeper<P2PJoinRequestReply>>> m_queuedJoinRequests;

  EntityId m_lastMouseoverTarget;
  GameTimer m_stickyTargetingTimer;
  int m_portraitScale;

  EntityId m_specialDamageBarTarget;
  float m_specialDamageBarValue;

  ContainerInteractorPtr m_containerInteractor;
};

}

#endif
