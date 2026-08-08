#pragma once

#include "StarUniverseServer.hpp"
#include "StarUniverseClient.hpp"
#include "StarWorldPainter.hpp"
#include "StarGameTypes.hpp"
#include "StarMainInterface.hpp"
#include "StarMainMixer.hpp"
#include "StarTitleScreen.hpp"
#include "StarErrorScreen.hpp"
#include "StarCinematic.hpp"
#include "StarKeyBindings.hpp"
#include "StarInventoryTypes.hpp"
#include "StarMainApplication.hpp"

namespace Star {

STAR_CLASS(Input);
STAR_CLASS(Voice);

class ClientApplication : public Application {
public:
  void setPostProcessLayerPasses(String const& layer, unsigned const& passes);
  void setPostProcessGroupEnabled(String const& group, bool const& enabled, Maybe<bool> const& save);
  bool postProcessGroupEnabled(String const& group);
  Json postProcessGroups();
  virtual unsigned framesSkipped() const override;

protected:
  virtual void startup(StringList const& cmdLineArgs) override;
  virtual void shutdown() override;

  virtual void applicationInit(ApplicationControllerPtr appController) override;
  virtual void renderInit(RendererPtr renderer) override;

  virtual void windowChanged(WindowMode windowMode, Vec2U screenSize) override;

  virtual void processInput(InputEvent const& event) override;

  virtual void update() override;
  virtual void render() override;

  virtual void getAudioData(int16_t* stream, size_t len) override;

private:
  enum class MainAppState {
    Quit,
    Startup,
    SteamFlatpakWarning,
    Mods,
    ModsWarning,
    Splash,
    Error,
    Title,
    SinglePlayer,
    MultiPlayer
  };

  struct PendingMultiPlayerConnection {
    Variant<P2PNetworkingPeerId, HostAddressWithPort> server;
    String account;
    String password;
    bool forceLegacy;
  };
  
  struct PostProcessGroup {
    bool enabled;
  };
  
  struct PostProcessLayer {
    List<String> effects;
    unsigned passes;
    PostProcessGroup* group;
  };

  void renderReload();

  void changeState(MainAppState newState);
  void setError(String const& error);
  void setError(String const& error, std::exception const& e);

  void loadMods();
  void updateSteamFlatpakWarning(float dt);
  void updateMods(float dt);
  void updateModsWarning(float dt);
  void updateSplash(float dt);
  void updateError(float dt);
  void updateTitle(float dt);
  void updateRunning(float dt);

  bool isActionTaken(InterfaceAction action) const;
  bool isActionTakenEdge(InterfaceAction action) const;

  void updateCamera(float dt);

  RootUPtr m_root;
  ThreadFunction<void> m_rootLoader;
  CallbackListenerPtr m_reloadListener;

  MainAppState m_state = MainAppState::Startup;

  // Valid after applicationInit is called
  MainMixerPtr m_mainMixer;
  GuiContextPtr m_guiContext;
  InputPtr m_input;
  VoicePtr m_voice;

  // Valid after renderInit is called the first time
  CinematicPtr m_cinematicOverlay;
  ErrorScreenPtr m_errorScreen;

  // Valid if main app state >= Title
  PlayerStoragePtr m_playerStorage;
  StatisticsPtr m_statistics;
  UniverseClientPtr m_universeClient;
  TitleScreenPtr m_titleScreen;

  // Valid if main app state > Title
  PlayerPtr m_player;
  WorldPainterPtr m_worldPainter;
  WorldRenderData m_renderData;
  MainInterfacePtr m_mainInterface;
  
  StringMap<PostProcessGroup> m_postProcessGroups;
  List<PostProcessLayer> m_postProcessLayers;
  StringMap<size_t> m_labelledPostProcessLayers;

  // Valid if main app state == SinglePlayer
  UniverseServerPtr m_universeServer;

  float m_cameraXOffset = 0.0f;
  float m_cameraYOffset = 0.0f;
  bool m_snapBackCameraOffset = false;
  float m_cameraOffsetDownTime = 0.f;
  Vec2F m_cameraPositionSmoother;
  Vec2F m_cameraSmoothDelta;
  int m_cameraZoomDirection = 0;

  unsigned m_framesSkipped = 0;
  float m_minInterfaceScale = 2;
  float m_maxInterfaceScale = 3;
  Vec2F m_crossoverRes;

  // Controller input mode: "off", "auto", "gamepad", "hybrid"
  // off = controller input completely disabled
  // auto = switch between gamepad/mouse based on last input
  // gamepad = right stick aims, virtual cursor for menus, mouse disabled
  // hybrid = right stick aims, mouse still active for UI
  enum class ControllerMode { Off, Auto, Gamepad, Hybrid };
  ControllerMode m_controllerMode = ControllerMode::Auto;
  // In Auto mode, tracks whether gamepad is currently the active input device
  bool m_gamepadActive = false;

  Vec2F m_controllerLeftStick;
  Vec2F m_controllerRightStick;
  float m_aimRadius = 8.0f; // world tiles
  float m_aimDeadzone = 0.15f;
  float m_verticalThreshold = 0.5f; // stick Y threshold for up/down movement
  Vec2F m_controllerAimPosition; // world-space aim from right stick
  Vec2F m_controllerAimOffset; // relative aim offset from player (preserved when stick centered)
  bool m_controllerAimActive = false; // true once right stick has been used at least once
  bool m_virtualCursorActive = false; // true when right stick controls screen cursor
  Vec2F m_virtualCursorPos; // screen-space position of virtual cursor
  float m_virtualCursorSpeed = 800.0f; // pixels per second at full tilt
  ControllerId m_activeController = (ControllerId)-1; // which controller to accept axis input from
  SelectedActionBarLocation m_prevActionBarLocation; // for MM toggle return
  bool m_triggerLeftPressed = false; // trigger axis → button edge detection
  bool m_triggerRightPressed = false;
  uint64_t m_activeControllerLastSeen = 0; // frame counter for controller disconnect detection
  uint64_t m_frameCounter = 0; // monotonic frame counter
  bool m_controllerShiftToggle = false; // persistent shift toggle for controller MM single-block mode

  List<KeyDownEvent> m_heldKeyEvents;
  List<KeyDownEvent> m_edgeKeyEvents;

  Maybe<PendingMultiPlayerConnection> m_pendingMultiPlayerConnection;
  Maybe<HostAddressWithPort> m_currentRemoteJoin;
  int64_t m_timeSinceJoin = 0;

  ByteArray m_immediateFont;

  bool m_loggedUGCCheck;
};

}
