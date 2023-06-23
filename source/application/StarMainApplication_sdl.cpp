#include "StarMainApplication.hpp"
#include "StarLogging.hpp"
#include "StarSignalHandler.hpp"
#include "StarTickRateMonitor.hpp"
#include "StarRenderer_opengl20.hpp"

#include "SDL.h"
#include "StarPlatformServices_pc.hpp"

namespace Star {

Maybe<Key> keyFromSdlKeyCode(SDL_Keycode sym) {
  static HashMap<int, Key> KeyCodeMap{
    {SDLK_BACKSPACE, Key::Backspace},
    {SDLK_TAB, Key::Tab},
    {SDLK_CLEAR, Key::Clear},
    {SDLK_RETURN, Key::Return},
    {SDLK_PAUSE, Key::Pause},
    {SDLK_ESCAPE, Key::Escape},
    {SDLK_SPACE, Key::Space},
    {SDLK_EXCLAIM, Key::Exclaim},
    {SDLK_QUOTEDBL, Key::QuotedBL},
    {SDLK_HASH, Key::Hash},
    {SDLK_DOLLAR, Key::Dollar},
    {SDLK_AMPERSAND, Key::Ampersand},
    {SDLK_QUOTE, Key::Quote},
    {SDLK_LEFTPAREN, Key::LeftParen},
    {SDLK_RIGHTPAREN, Key::RightParen},
    {SDLK_ASTERISK, Key::Asterisk},
    {SDLK_PLUS, Key::Plus},
    {SDLK_COMMA, Key::Comma},
    {SDLK_MINUS, Key::Minus},
    {SDLK_PERIOD, Key::Period},
    {SDLK_SLASH, Key::Slash},
    {SDLK_0, Key::Zero},
    {SDLK_1, Key::One},
    {SDLK_2, Key::Two},
    {SDLK_3, Key::Three},
    {SDLK_4, Key::Four},
    {SDLK_5, Key::Five},
    {SDLK_6, Key::Six},
    {SDLK_7, Key::Seven},
    {SDLK_8, Key::Eight},
    {SDLK_9, Key::Nine},
    {SDLK_COLON, Key::Colon},
    {SDLK_SEMICOLON, Key::Semicolon},
    {SDLK_LESS, Key::Less},
    {SDLK_EQUALS, Key::Equals},
    {SDLK_GREATER, Key::Greater},
    {SDLK_QUESTION, Key::Question},
    {SDLK_AT, Key::At},
    {SDLK_LEFTBRACKET, Key::LeftBracket},
    {SDLK_BACKSLASH, Key::Backslash},
    {SDLK_RIGHTBRACKET, Key::RightBracket},
    {SDLK_CARET, Key::Caret},
    {SDLK_UNDERSCORE, Key::Underscore},
    {SDLK_BACKQUOTE, Key::Backquote},
    {SDLK_a, Key::A},
    {SDLK_b, Key::B},
    {SDLK_c, Key::C},
    {SDLK_d, Key::D},
    {SDLK_e, Key::E},
    {SDLK_f, Key::F},
    {SDLK_g, Key::G},
    {SDLK_h, Key::H},
    {SDLK_i, Key::I},
    {SDLK_j, Key::J},
    {SDLK_k, Key::K},
    {SDLK_l, Key::L},
    {SDLK_m, Key::M},
    {SDLK_n, Key::N},
    {SDLK_o, Key::O},
    {SDLK_p, Key::P},
    {SDLK_q, Key::Q},
    {SDLK_r, Key::R},
    {SDLK_s, Key::S},
    {SDLK_t, Key::T},
    {SDLK_u, Key::U},
    {SDLK_v, Key::V},
    {SDLK_w, Key::W},
    {SDLK_x, Key::X},
    {SDLK_y, Key::Y},
    {SDLK_z, Key::Z},
    {SDLK_DELETE, Key::Delete},
    {SDLK_KP_0, Key::Kp0},
    {SDLK_KP_1, Key::Kp1},
    {SDLK_KP_2, Key::Kp2},
    {SDLK_KP_3, Key::Kp3},
    {SDLK_KP_4, Key::Kp4},
    {SDLK_KP_5, Key::Kp5},
    {SDLK_KP_6, Key::Kp6},
    {SDLK_KP_7, Key::Kp7},
    {SDLK_KP_8, Key::Kp8},
    {SDLK_KP_9, Key::Kp9},
    {SDLK_KP_PERIOD, Key::Kp_period},
    {SDLK_KP_DIVIDE, Key::Kp_divide},
    {SDLK_KP_MULTIPLY, Key::Kp_multiply},
    {SDLK_KP_MINUS, Key::Kp_minus},
    {SDLK_KP_PLUS, Key::Kp_plus},
    {SDLK_KP_ENTER, Key::Kp_enter},
    {SDLK_KP_EQUALS, Key::Kp_equals},
    {SDLK_UP, Key::Up},
    {SDLK_DOWN, Key::Down},
    {SDLK_RIGHT, Key::Right},
    {SDLK_LEFT, Key::Left},
    {SDLK_INSERT, Key::Insert},
    {SDLK_HOME, Key::Home},
    {SDLK_END, Key::End},
    {SDLK_PAGEUP, Key::PageUp},
    {SDLK_PAGEDOWN, Key::PageDown},
    {SDLK_F1, Key::F1},
    {SDLK_F2, Key::F2},
    {SDLK_F3, Key::F3},
    {SDLK_F4, Key::F4},
    {SDLK_F5, Key::F5},
    {SDLK_F6, Key::F6},
    {SDLK_F7, Key::F7},
    {SDLK_F8, Key::F8},
    {SDLK_F9, Key::F9},
    {SDLK_F10, Key::F10},
    {SDLK_F11, Key::F11},
    {SDLK_F12, Key::F12},
    {SDLK_F13, Key::F13},
    {SDLK_F14, Key::F14},
    {SDLK_F15, Key::F15},
    {SDLK_NUMLOCKCLEAR, Key::NumLock},
    {SDLK_CAPSLOCK, Key::CapsLock},
    {SDLK_SCROLLLOCK, Key::ScrollLock},
    {SDLK_RSHIFT, Key::RShift},
    {SDLK_LSHIFT, Key::LShift},
    {SDLK_RCTRL, Key::RCtrl},
    {SDLK_LCTRL, Key::LCtrl},
    {SDLK_RALT, Key::RAlt},
    {SDLK_LALT, Key::LAlt},
    {SDLK_RGUI, Key::RGui},
    {SDLK_LGUI, Key::LGui},
    {SDLK_MODE, Key::AltGr},
    {SDLK_APPLICATION, Key::Compose},
    {SDLK_HELP, Key::Help},
    {SDLK_PRINTSCREEN, Key::PrintScreen},
    {SDLK_SYSREQ, Key::SysReq},
    {SDLK_PAUSE, Key::Pause},
    {SDLK_MENU, Key::Menu},
    {SDLK_POWER, Key::Power}
  };

  return KeyCodeMap.maybe(sym);
}

KeyMod keyModsFromSdlKeyMods(uint16_t mod) {
  KeyMod keyMod = KeyMod::NoMod;

  if (mod & KMOD_LSHIFT)
    keyMod |= KeyMod::LShift;
  if (mod & KMOD_RSHIFT)
    keyMod |= KeyMod::RShift;
  if (mod & KMOD_LCTRL)
    keyMod |= KeyMod::LCtrl;
  if (mod & KMOD_RCTRL)
    keyMod |= KeyMod::RCtrl;
  if (mod & KMOD_LALT)
    keyMod |= KeyMod::LAlt;
  if (mod & KMOD_RALT)
    keyMod |= KeyMod::RAlt;
  if (mod & KMOD_LGUI)
    keyMod |= KeyMod::LGui;
  if (mod & KMOD_RGUI)
    keyMod |= KeyMod::RGui;
  if (mod & KMOD_NUM)
    keyMod |= KeyMod::Num;
  if (mod & KMOD_CAPS)
    keyMod |= KeyMod::Caps;
  if (mod & KMOD_MODE)
    keyMod |= KeyMod::AltGr;

  return keyMod;
}

MouseButton mouseButtonFromSdlMouseButton(uint8_t button) {
  if (button == SDL_BUTTON_LEFT)
    return MouseButton::Left;
  else if (button == SDL_BUTTON_MIDDLE)
    return MouseButton::Middle;
  else if (button == SDL_BUTTON_RIGHT)
    return MouseButton::Right;
  else if (button == SDL_BUTTON_X1)
    return MouseButton::FourthButton;
  else
    return MouseButton::FifthButton;
}

class SdlPlatform {
public:
  SdlPlatform(ApplicationUPtr application, StringList cmdLineArgs) {
    m_application = move(application);

    // extract application path from command line args
    String applicationPath = cmdLineArgs.first();
    cmdLineArgs = cmdLineArgs.slice(1);

    StringList platformArguments;
    eraseWhere(cmdLineArgs, [&platformArguments](String& argument) {
        if (argument.beginsWith("+platform")) {
          platformArguments.append(move(argument));
          return true;
        }
        return false;
      });

    Logger::info("Application: Initializing SDL");
    if (SDL_Init(0))
      throw ApplicationException(strf("Couldn't initialize SDL: %s", SDL_GetError()));

    if (char* basePath = SDL_GetBasePath()) {
      File::changeDirectory(basePath);
      SDL_free(basePath);
    }

    m_signalHandler.setHandleInterrupt(true);
    m_signalHandler.setHandleFatal(true);

    try {
      Logger::info("Application: startup...");
      m_application->startup(cmdLineArgs);
    } catch (std::exception const& e) {
      throw ApplicationException("Application threw exception during startup", e);
    }

    Logger::info("Application: Initializing SDL Video");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO))
      throw ApplicationException(strf("Couldn't initialize SDL Video: %s", SDL_GetError()));

    Logger::info("Application: Initializing SDL Joystick");
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK))
      throw ApplicationException(strf("Couldn't initialize SDL Joystick: %s", SDL_GetError()));

    Logger::info("Application: Initializing SDL Sound");
    if (SDL_InitSubSystem(SDL_INIT_AUDIO))
      throw ApplicationException(strf("Couldn't initialize SDL Sound: %s", SDL_GetError()));

    SDL_JoystickEventState(SDL_ENABLE);

    m_platformServices = PcPlatformServices::create(applicationPath, platformArguments);
    if (!m_platformServices)
      Logger::info("Application: No platform services available");

    Logger::info("Application: Creating SDL Window");
    m_sdlWindow = SDL_CreateWindow(m_windowTitle.utf8Ptr(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_windowSize[0], m_windowSize[1], SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_sdlWindow)
      throw ApplicationException::format("Application: Could not create SDL window: %s", SDL_GetError());

    SDL_ShowWindow(m_sdlWindow);
    SDL_RaiseWindow(m_sdlWindow);

    int width;
    int height;
    SDL_GetWindowSize(m_sdlWindow, &width, &height);
    m_windowSize = Vec2U(width, height);

    m_sdlGlContext = SDL_GL_CreateContext(m_sdlWindow);
    if (!m_sdlGlContext)
      throw ApplicationException::format("Application: Could not create OpenGL context: %s", SDL_GetError());

    SDL_GL_SwapWindow(m_sdlWindow);
    setVSyncEnabled(m_windowVSync);

    SDL_StopTextInput();

    SDL_AudioSpec desired = {};
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.samples = 2048;
    desired.channels = 2;
    desired.userdata = this;
    desired.callback = [](void* userdata, Uint8* stream, int len) {
      ((SdlPlatform*)(userdata))->getAudioData(stream, len);
    };

    SDL_AudioSpec obtained = {};
    m_audioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (!m_audioDevice) {
      Logger::error("Application: Could not open audio device, no sound available!");
    } else if (obtained.freq != desired.freq || obtained.channels != desired.channels || obtained.format != desired.format) {
      SDL_CloseAudioDevice(m_audioDevice);
      Logger::error("Application: Could not open 44.1khz / 16 bit stereo audio device, no sound available!");
    } else {
      Logger::info("Application: Opened default audio device with 44.1khz / 16 bit stereo audio, %s sample size buffer", obtained.samples);
      SDL_PauseAudioDevice(m_audioDevice, 0);
    }

    m_renderer = make_shared<OpenGl20Renderer>();
    m_renderer->setScreenSize(m_windowSize);
  }

  ~SdlPlatform() {
    SDL_CloseAudioDevice(m_audioDevice);

    m_renderer.reset();

    Logger::info("Application: Destroying SDL Window");
    SDL_DestroyWindow(m_sdlWindow);

    SDL_Quit();
  }

  void run() {
    try {
      Logger::info("Application: initialization...");
      m_application->applicationInit(make_shared<Controller>(this));

      Logger::info("Application: renderer initialization...");
      m_application->renderInit(m_renderer);

      Logger::info("Application: main update loop...");

      m_updateTicker.reset();
      m_renderTicker.reset();

      bool quit = false;
      while (true) {
        for (auto const& event : processEvents())
          m_application->processInput(event);

        if (m_platformServices)
          m_platformServices->update();

        if (m_platformServices->overlayActive())
          SDL_ShowCursor(1);
        else
          SDL_ShowCursor(m_cursorVisible ? 1 : 0);

        int updatesBehind = max<int>(round(m_updateTicker.ticksBehind()), 1);
        updatesBehind = min<int>(updatesBehind, m_maxFrameSkip + 1);
        for (int i = 0; i < updatesBehind; ++i) {
          m_application->update();
          m_updateRate = m_updateTicker.tick();
        }

        m_renderer->startFrame();
        m_application->render();
        m_renderer->finishFrame();
        SDL_GL_SwapWindow(m_sdlWindow);
        m_renderRate = m_renderTicker.tick();

        if (m_quitRequested) {
          Logger::info("Application: quit requested");
          quit = true;
        }

        if (m_signalHandler.interruptCaught()) {
          Logger::info("Application: Interrupt caught");
          quit = true;
        }

        if (quit) {
          Logger::info("Application: quitting...");
          break;
        }

        int64_t spareMilliseconds = round(m_updateTicker.spareTime() * 1000);
        if (spareMilliseconds > 0)
          Thread::sleepPrecise(spareMilliseconds);
      }
    } catch (std::exception const& e) {
      Logger::error("Application: exception thrown, shutting down: %s", outputException(e, true));
    }

    try {
      Logger::info("Application: shutdown...");
      m_application->shutdown();
    } catch (std::exception const& e) {
      Logger::error("Application: threw exception during shutdown: %s", outputException(e, true));
    }

    SDL_CloseAudioDevice(m_audioDevice);
    m_application.reset();
  }

private:
  struct Controller : public ApplicationController {
    Controller(SdlPlatform* parent)
      : parent(parent) {}

    Maybe<String> getClipboard() override {
      if (SDL_HasClipboardText())
        return String(SDL_GetClipboardText());
      return {};
    }

    void setClipboard(String text) override {
      SDL_SetClipboardText(text.utf8Ptr());
    }

    void setTargetUpdateRate(float targetUpdateRate) override {
      parent->m_updateTicker.setTargetTickRate(targetUpdateRate);
    }

    void setUpdateTrackWindow(float updateTrackWindow) override {
      parent->m_updateTicker.setWindow(updateTrackWindow);
    }

    void setApplicationTitle(String title) override {
      parent->m_windowTitle = move(title);
      if (parent->m_sdlWindow)
        SDL_SetWindowTitle(parent->m_sdlWindow, parent->m_windowTitle.utf8Ptr());
    }

    void setFullscreenWindow(Vec2U fullScreenResolution) override {
      if (parent->m_windowMode != WindowMode::Fullscreen || parent->m_windowSize != fullScreenResolution) {
        SDL_DisplayMode requestedDisplayMode = {SDL_PIXELFORMAT_RGB888, (int)fullScreenResolution[0], (int)fullScreenResolution[1], 0, 0};
        int currentDisplayIndex = SDL_GetWindowDisplayIndex(parent->m_sdlWindow);

        SDL_DisplayMode targetDisplayMode;
        if (SDL_GetClosestDisplayMode(currentDisplayIndex, &requestedDisplayMode, &targetDisplayMode) != NULL) {
          if (SDL_SetWindowDisplayMode(parent->m_sdlWindow, &requestedDisplayMode) == 0) {
            if (parent->m_windowMode == WindowMode::Fullscreen)
              SDL_SetWindowFullscreen(parent->m_sdlWindow, 0);
            else
              parent->m_windowMode = WindowMode::Fullscreen;
            SDL_SetWindowFullscreen(parent->m_sdlWindow, SDL_WINDOW_FULLSCREEN);
          } else {
            Logger::warn("Failed to set resolution %s, %s", (unsigned)requestedDisplayMode.w, (unsigned)requestedDisplayMode.h);
          }
        } else {
          Logger::warn("Unable to set requested display resolution %s, %s", (int)fullScreenResolution[0], (int)fullScreenResolution[1]);
        }

        SDL_DisplayMode actualDisplayMode;
        if (SDL_GetWindowDisplayMode(parent->m_sdlWindow, &actualDisplayMode) == 0) {
          parent->m_windowSize = {(unsigned)actualDisplayMode.w, (unsigned)actualDisplayMode.h};

          // call these manually since no SDL_WindowEvent is triggered when changing between fullscreen resolutions for some reason
          parent->m_renderer->setScreenSize(parent->m_windowSize);
          parent->m_application->windowChanged(parent->m_windowMode, parent->m_windowSize);
        } else {
          Logger::error("Couldn't get window display mode!");
        }
      }
    }

    void setNormalWindow(Vec2U windowSize) override {
      if (parent->m_windowMode != WindowMode::Normal || parent->m_windowSize != windowSize) {
        if (parent->m_windowMode == WindowMode::Fullscreen || parent->m_windowMode == WindowMode::Borderless) {
          SDL_SetWindowFullscreen(parent->m_sdlWindow, 0);
          SDL_SetWindowBordered(parent->m_sdlWindow, SDL_TRUE);
        }
        else if (parent->m_windowMode == WindowMode::Maximized)
          SDL_RestoreWindow(parent->m_sdlWindow);

        SDL_SetWindowSize(parent->m_sdlWindow, windowSize[0], windowSize[1]);
        SDL_SetWindowPosition(parent->m_sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        parent->m_windowMode = WindowMode::Normal;
        parent->m_windowSize = windowSize;
      }
    }

    void setMaximizedWindow() override {
      if (parent->m_windowMode != WindowMode::Maximized) {
        if (parent->m_windowMode == WindowMode::Fullscreen || parent->m_windowMode == WindowMode::Borderless) {
          SDL_SetWindowFullscreen(parent->m_sdlWindow, 0);
          SDL_SetWindowBordered(parent->m_sdlWindow, SDL_TRUE);
        }

        SDL_MaximizeWindow(parent->m_sdlWindow);
        parent->m_windowMode = WindowMode::Maximized;
      }
    }

    void setBorderlessWindow() override {
      if (parent->m_windowMode != WindowMode::Borderless) {
        SDL_SetWindowFullscreen(parent->m_sdlWindow, 0);
        SDL_SetWindowBordered(parent->m_sdlWindow, SDL_FALSE);
        SDL_MaximizeWindow(parent->m_sdlWindow);
        parent->m_windowMode = WindowMode::Borderless;

        SDL_DisplayMode actualDisplayMode;
        if (SDL_GetWindowDisplayMode(parent->m_sdlWindow, &actualDisplayMode) == 0) {
          parent->m_windowSize = {(unsigned)actualDisplayMode.w, (unsigned)actualDisplayMode.h};

          parent->m_renderer->setScreenSize(parent->m_windowSize);
          parent->m_application->windowChanged(parent->m_windowMode, parent->m_windowSize);
        } else {
          Logger::error("Couldn't get window display mode!");
        }
      }
    }

    void setVSyncEnabled(bool vSync) override {
      if (parent->m_windowVSync != vSync) {
        parent->setVSyncEnabled(vSync);
        parent->m_windowVSync = vSync;
      }
    }

    void setMaxFrameSkip(unsigned maxFrameSkip) override {
      parent->m_maxFrameSkip = maxFrameSkip;
    }

    void setCursorVisible(bool cursorVisible) override {
      parent->m_cursorVisible = cursorVisible;
    }

    void setAcceptingTextInput(bool acceptingTextInput) override {
      if (acceptingTextInput != parent->m_acceptingTextInput) {
        if (acceptingTextInput)
          SDL_StartTextInput();
        else
          SDL_StopTextInput();

        parent->m_acceptingTextInput = acceptingTextInput;
      }
    }

    AudioFormat enableAudio() override {
      parent->m_audioEnabled = true;
      SDL_PauseAudio(false);
      return AudioFormat{44100, 2};
    }

    void disableAudio() override {
      parent->m_audioEnabled = false;
      SDL_PauseAudio(true);
    }

    float updateRate() const override {
      return parent->m_updateRate;
    }

    float renderFps() const override {
      return parent->m_renderRate;
    }

    StatisticsServicePtr statisticsService() const override {
      if (parent->m_platformServices)
        return parent->m_platformServices->statisticsService();
      return {};
    }

    P2PNetworkingServicePtr p2pNetworkingService() const override {
      if (parent->m_platformServices)
        return parent->m_platformServices->p2pNetworkingService();
      return {};
    }

    UserGeneratedContentServicePtr userGeneratedContentService() const override {
      if (parent->m_platformServices)
        return parent->m_platformServices->userGeneratedContentService();
      return {};
    }

    DesktopServicePtr desktopService() const override {
      if (parent->m_platformServices)
        return parent->m_platformServices->desktopService();
      return {};
    }

    void quit() override {
      parent->m_quitRequested = true;
    }

    SdlPlatform* parent;
  };

  List<InputEvent> processEvents() {
    List<InputEvent> inputEvents;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      Maybe<InputEvent> starEvent;
      if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED || event.window.event == SDL_WINDOWEVENT_RESTORED) {
          auto windowFlags = SDL_GetWindowFlags(m_sdlWindow);

          if (windowFlags & SDL_WINDOW_MAXIMIZED) {
            m_windowMode = WindowMode::Maximized;
          } else if (windowFlags & SDL_WINDOW_FULLSCREEN || windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
            if (m_windowMode != WindowMode::Fullscreen && m_windowMode != WindowMode::Borderless)
              m_windowMode = WindowMode::Fullscreen;
          } else {
            m_windowMode = WindowMode::Normal;
          }

          m_application->windowChanged(m_windowMode, m_windowSize);

        } else if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          m_windowSize = Vec2U(event.window.data1, event.window.data2);
          m_renderer->setScreenSize(m_windowSize);
          m_application->windowChanged(m_windowMode, m_windowSize);
        }

      } else if (event.type == SDL_KEYDOWN) {
        if (!event.key.repeat) {
          if (auto key = keyFromSdlKeyCode(event.key.keysym.sym))
            starEvent.set(KeyDownEvent{*key, keyModsFromSdlKeyMods(event.key.keysym.mod)});
        }

      } else if (event.type == SDL_KEYUP) {
        if (auto key = keyFromSdlKeyCode(event.key.keysym.sym))
          starEvent.set(KeyUpEvent{*key});

      } else if (event.type == SDL_TEXTINPUT) {
        starEvent.set(TextInputEvent{String(event.text.text)});

      } else if (event.type == SDL_MOUSEMOTION) {
        starEvent.set(MouseMoveEvent{
            {event.motion.xrel, -event.motion.yrel}, {event.motion.x, (int)m_windowSize[1] - event.motion.y}});

      } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        starEvent.set(MouseButtonDownEvent{mouseButtonFromSdlMouseButton(event.button.button),
            {event.button.x, (int)m_windowSize[1] - event.button.y}});

      } else if (event.type == SDL_MOUSEBUTTONUP) {
        starEvent.set(MouseButtonUpEvent{mouseButtonFromSdlMouseButton(event.button.button),
            {event.button.x, (int)m_windowSize[1] - event.button.y}});

      } else if (event.type == SDL_MOUSEWHEEL) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        starEvent.set(MouseWheelEvent{event.wheel.y < 0 ? MouseWheel::Down : MouseWheel::Up, {x, (int)m_windowSize[1] - y}});

      } else if (event.type == SDL_QUIT) {
        m_quitRequested = true;
        starEvent.reset();
      }

      if (starEvent)
        inputEvents.append(starEvent.take());
    }

    return inputEvents;
  }

  void getAudioData(Uint8* stream, int len) {
    if (m_audioEnabled) {
      m_application->getAudioData((int16_t*)stream, len / 4);
    } else {
      for (int i = 0; i < len; ++i)
        stream[i] = 0;
    }
  }

  void setVSyncEnabled(bool vsyncEnabled) {
    if (vsyncEnabled) {
      // If VSync is requested, try for late swap tearing first, then fall back
      // to regular VSync
      Logger::info("Application: Enabling VSync with late swap tearing");
      if (SDL_GL_SetSwapInterval(-1) < 0) {
        Logger::info("Application: Enabling VSync late swap tearing failed, falling back to full VSync");
        SDL_GL_SetSwapInterval(1);
      }
    } else {
      Logger::info("Application: Disabling VSync");
      SDL_GL_SetSwapInterval(0);
    }
  }

  SignalHandler m_signalHandler;

  TickRateApproacher m_updateTicker = TickRateApproacher(60.0f, 1.0f);
  float m_updateRate = 0.0f;
  TickRateMonitor m_renderTicker = TickRateMonitor(1.0f);
  float m_renderRate = 0.0f;

  SDL_Window* m_sdlWindow = nullptr;
  SDL_GLContext m_sdlGlContext = nullptr;
  SDL_AudioDeviceID m_audioDevice = 0;

  Vec2U m_windowSize = {800, 600};
  WindowMode m_windowMode = WindowMode::Normal;

  String m_windowTitle = "Starbound";
  bool m_windowVSync = true;
  unsigned m_maxFrameSkip = 5;
  bool m_cursorVisible = true;
  bool m_acceptingTextInput = false;
  bool m_audioEnabled = false;
  bool m_quitRequested = false;

  OpenGl20RendererPtr m_renderer;
  ApplicationUPtr m_application;
  PcPlatformServicesUPtr m_platformServices;
};

int runMainApplication(ApplicationUPtr application, StringList cmdLineArgs) {
  try {
    {
      SdlPlatform platform(move(application), move(cmdLineArgs));
      platform.run();
    }
    Logger::info("Application: stopped gracefully");
    return 0;
  } catch (std::exception const& e) {
    fatalException(e, true);
  } catch (...) {
    fatalError("Unknown Exception", true);
  }
  return 1;
}

}
