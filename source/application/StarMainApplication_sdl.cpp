#include "StarMainApplication.hpp"
#include "StarLogging.hpp"
#include "StarSignalHandler.hpp"
#include "StarTickRateMonitor.hpp"
#include "StarRenderer_opengl.hpp"
#include "StarTtlCache.hpp"
#include "StarImage.hpp"
#include "StarImageProcessing.hpp"

#include "SDL3/SDL.h"
#include "StarPlatformServices_pc.hpp"

#ifdef STAR_SYSTEM_WINDOWS
#include <dwmapi.h>
#include <objidl.h>
#include <ShlObj_core.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

namespace Star {

#ifdef STAR_SYSTEM_WINDOWS
static bool copyDibToClipboard(const uint8_t* buf, unsigned int width, unsigned int height) {
	size_t size = (size_t)(width * height * 4u);
	BITMAPV5HEADER hV5{};
	hV5.bV5Size = sizeof(hV5);
	hV5.bV5Width = width;
	hV5.bV5Height = height;
	hV5.bV5Planes = 1;
	hV5.bV5BitCount = 32;
	hV5.bV5Compression = BI_BITFIELDS;
	hV5.bV5RedMask = 0x00FF0000;
	hV5.bV5GreenMask = 0x0000FF00;
	hV5.bV5BlueMask = 0x000000FF;
	hV5.bV5AlphaMask = 0xFF000000;

	BITMAPINFOHEADER hV1{};
	hV1.biSize = sizeof(hV1);
	hV1.biWidth = width;
	hV1.biHeight = height;
	hV1.biPlanes = 1;
	hV1.biBitCount = 32;
	hV1.biCompression = BI_BITFIELDS;
	hV1.biSizeImage = size;
	DWORD colors[3] = { 0x00FF0000, 0x0000FF00, 0x000000FF };
	if (HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, sizeof(hV1) + sizeof(colors) + size)) {
		if (auto dst = (char*)GlobalLock(handle)) {
			memcpy(dst, &hV1, sizeof(hV1));
			memcpy(dst + sizeof(hV1), &colors, sizeof(colors));
			unsigned int sizeI = width * height;
			unsigned int* data = (unsigned int*)buf;
			unsigned int* rgba = (unsigned int*)(dst + sizeof(hV1) + sizeof(colors));
			for (unsigned int x = 0; x != sizeI; ++x) {
				unsigned int c = data[x];
				if ((c & 0xFF000000) != 0xFF000000) { //pre-multiply
					float a = (float)(c >> 24) / 255.0f;
					unsigned int v = c & 0xFF000000;
					v |= (unsigned int)((c & 0x000000FF) * a) << 16; // r
					v |= (unsigned int)(((c >> 8) & 0x000000FF) * a) << 8; // g
					v |= (unsigned int)(((c >> 16) & 0x000000FF) * a); // b
					rgba[x] = v;
				} else {
					unsigned int v = c & 0xFF00FF00; // a and g
					v |= c << 16 & 0x00FF0000; // r
					v |= c >> 16 & 0x000000FF; // b
					rgba[x] = v;
				}
			}
			GlobalUnlock(handle);
		}

		SetClipboardData(CF_DIB, handle);
    return true;
	}

	return false;
}

static bool copyPngToClipboard(void* buf, size_t size) {
  if (HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, size)) {
    if (auto dst = (char*)GlobalLock(handle)) {
      memcpy(dst, buf, size);
      GlobalUnlock(handle);
    }
    SetClipboardData(RegisterClipboardFormatA("PNG"), handle);
    return true;
  }
  return false;
}

static bool duringClipboard(SDL_Window* window, std::function<void()> task) {
  auto props = SDL_GetWindowProperties(window);
  auto handle = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
  if (!handle || !OpenClipboard(handle))
    return false;
  EmptyClipboard();
  task();
  CloseClipboard();
  return true;
}
#endif

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
    {SDLK_DBLAPOSTROPHE, Key::QuotedBL},
    {SDLK_HASH, Key::Hash},
    {SDLK_DOLLAR, Key::Dollar},
    {SDLK_AMPERSAND, Key::Ampersand},
    {SDLK_APOSTROPHE, Key::Quote},
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
    {SDLK_GRAVE, Key::Backquote},
    {SDLK_A, Key::A},
    {SDLK_B, Key::B},
    {SDLK_C, Key::C},
    {SDLK_D, Key::D},
    {SDLK_E, Key::E},
    {SDLK_F, Key::F},
    {SDLK_G, Key::G},
    {SDLK_H, Key::H},
    {SDLK_I, Key::I},
    {SDLK_J, Key::J},
    {SDLK_K, Key::K},
    {SDLK_L, Key::L},
    {SDLK_M, Key::M},
    {SDLK_N, Key::N},
    {SDLK_O, Key::O},
    {SDLK_P, Key::P},
    {SDLK_Q, Key::Q},
    {SDLK_R, Key::R},
    {SDLK_S, Key::S},
    {SDLK_T, Key::T},
    {SDLK_U, Key::U},
    {SDLK_V, Key::V},
    {SDLK_W, Key::W},
    {SDLK_X, Key::X},
    {SDLK_Y, Key::Y},
    {SDLK_Z, Key::Z},
    {SDLK_DELETE, Key::Delete},
    {SDLK_KP_0, Key::Keypad0},
    {SDLK_KP_1, Key::Keypad1},
    {SDLK_KP_2, Key::Keypad2},
    {SDLK_KP_3, Key::Keypad3},
    {SDLK_KP_4, Key::Keypad4},
    {SDLK_KP_5, Key::Keypad5},
    {SDLK_KP_6, Key::Keypad6},
    {SDLK_KP_7, Key::Keypad7},
    {SDLK_KP_8, Key::Keypad8},
    {SDLK_KP_9, Key::Keypad9},
    {SDLK_KP_PERIOD, Key::KeypadPeriod},
    {SDLK_KP_DIVIDE, Key::KeypadDivide},
    {SDLK_KP_MULTIPLY, Key::KeypadMultiply},
    {SDLK_KP_MINUS, Key::KeypadMinus},
    {SDLK_KP_PLUS, Key::KeypadPlus},
    {SDLK_KP_ENTER, Key::KeypadEnter},
    {SDLK_KP_EQUALS, Key::KeypadEquals},
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
    {SDLK_F16, Key::F16},
    {SDLK_F17, Key::F17},
    {SDLK_F18, Key::F18},
    {SDLK_F19, Key::F19},
    {SDLK_F20, Key::F20},
    {SDLK_F21, Key::F21},
    {SDLK_F22, Key::F22},
    {SDLK_F23, Key::F23},
    {SDLK_F24, Key::F24},
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
  return static_cast<KeyMod>(mod);
}

MouseButton mouseButtonFromSdlMouseButton(uint8_t button) {
  switch (button) {
    case SDL_BUTTON_LEFT: return MouseButton::Left;
    case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
    case SDL_BUTTON_RIGHT: return MouseButton::Right;
    case SDL_BUTTON_X1: return MouseButton::FourthButton;
    default: return MouseButton::FifthButton;
  }
}

ControllerAxis controllerAxisFromSdlControllerAxis(uint8_t axis) {
  switch (axis) {
    case SDL_GAMEPAD_AXIS_LEFTX : return ControllerAxis::LeftX;
    case SDL_GAMEPAD_AXIS_LEFTY : return ControllerAxis::LeftY;
    case SDL_GAMEPAD_AXIS_RIGHTX : return ControllerAxis::RightX;
    case SDL_GAMEPAD_AXIS_RIGHTY : return ControllerAxis::RightY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER : return ControllerAxis::TriggerLeft;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER : return ControllerAxis::TriggerRight;
    default: return ControllerAxis::Invalid;
  }
}

ControllerButton controllerButtonFromSdlControllerButton(uint8_t button) {
  switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH : return ControllerButton::A;
    case SDL_GAMEPAD_BUTTON_EAST : return ControllerButton::B;
    case SDL_GAMEPAD_BUTTON_WEST : return ControllerButton::X;
    case SDL_GAMEPAD_BUTTON_NORTH : return ControllerButton::Y;
    case SDL_GAMEPAD_BUTTON_BACK : return ControllerButton::Back;
    case SDL_GAMEPAD_BUTTON_GUIDE : return ControllerButton::Guide;
    case SDL_GAMEPAD_BUTTON_START : return ControllerButton::Start;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK : return ControllerButton::LeftStick;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK : return ControllerButton::RightStick;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER : return ControllerButton::LeftShoulder;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER : return ControllerButton::RightShoulder;
    case SDL_GAMEPAD_BUTTON_DPAD_UP : return ControllerButton::DPadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN : return ControllerButton::DPadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT : return ControllerButton::DPadLeft;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT : return ControllerButton::DPadRight;
    case SDL_GAMEPAD_BUTTON_MISC1 : return ControllerButton::Misc1;
    case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1 : return ControllerButton::Paddle1;
    case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1 : return ControllerButton::Paddle2;
    case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2 : return ControllerButton::Paddle3;
    case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2 : return ControllerButton::Paddle4;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD : return ControllerButton::Touchpad;
    default: return ControllerButton::Invalid;
  }
}

class SdlPlatform {
public:
  SdlPlatform(ApplicationUPtr application, StringList cmdLineArgs) {
    m_application = std::move(application);

    // extract application path from command line args
    String applicationPath = cmdLineArgs.first();
    cmdLineArgs = cmdLineArgs.slice(1);

    StringList platformArguments;
    eraseWhere(cmdLineArgs, [&platformArguments](String& argument) {
        if (argument.beginsWith("+platform")) {
          platformArguments.append(std::move(argument));
          return true;
        }
        return false;
      });

    Logger::info("Application: Initializing SDL");
    if (!SDL_Init(0))
      throw ApplicationException(strf("Couldn't initialize SDL: {}", SDL_GetError()));

    if (auto basePath = SDL_GetBasePath())
      File::changeDirectory(basePath);

    m_signalHandler.setHandleInterrupt(true);
    m_signalHandler.setHandleFatal(true);

    try {
      Logger::info("Application: startup...");
      m_application->startup(cmdLineArgs);
    } catch (std::exception const& e) {
      throw ApplicationException("Application threw exception during startup", e);
    }

    //Sets Sdl metadata
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "Starbound");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
    //icon stuf
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_ICON_NAME, "steam_icon_211820");  // should be the default icon name steam has set for the icon

        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, "Test2");
	  
    
    Logger::info("Application: Initializing SDL Video");
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
      throw ApplicationException(strf("Couldn't initialize SDL Video: {}", SDL_GetError()));

    Logger::info("Application: using Video Driver '{}'", SDL_GetCurrentVideoDriver());

    Logger::info("Application: Initializing SDL Controller");
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
      throw ApplicationException(strf("Couldn't initialize SDL Controller: {}", SDL_GetError()));

#ifdef STAR_SYSTEM_WINDOWS // Newer SDL is defaulting to xaudio2, which does not support audio capture
  SDL_setenv_unsafe("SDL_AUDIODRIVER", "directsound", 1);
#endif

    Logger::info("Application: Initializing SDL Audio");
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
      throw ApplicationException(strf("Couldn't initialize SDL Audio: {}", SDL_GetError()));

    Logger::info("Application: using Audio Driver '{}'", SDL_GetCurrentAudioDriver());

    SDL_SetJoystickEventsEnabled(true);

    m_platformServices = PcPlatformServices::create(applicationPath, platformArguments);
    if (!m_platformServices)
      Logger::info("Application: No platform services available");

    Logger::info("Application: Creating SDL Window");
    m_sdlWindow = SDL_CreateWindow(m_windowTitle.utf8Ptr(), m_windowSize[0], m_windowSize[1], SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_sdlWindow)
      throw ApplicationException::format("Application: Could not create SDL Window: {}", SDL_GetError());
	  
#if defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    m_sdlGlContext = SDL_GL_CreateContext(m_sdlWindow);
    if (!m_sdlGlContext)
      throw ApplicationException::format("Application: Could not create OpenGL context: {}", SDL_GetError());
    
    SDL_ShowWindow(m_sdlWindow);
    SDL_RaiseWindow(m_sdlWindow);

    int width;
    int height;
    SDL_GetWindowSize(m_sdlWindow, &width, &height);
    m_windowSize = Vec2U(width, height);

    SDL_GL_SwapWindow(m_sdlWindow);
    setVSyncEnabled(m_windowVSync);

    SDL_StopTextInput(m_sdlWindow);

    SDL_AudioSpec desired = {SDL_AUDIO_S16, 2, 44100};
    m_sdlAudioOutputStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired,
    [](void* userdata, SDL_AudioStream* stream, int len, int) {
      if (len > 0) {
        auto sdlPlatform = ((SdlPlatform*)(userdata));
        sdlPlatform->m_audioOutputData.resize(len);
        sdlPlatform->getAudioData(sdlPlatform->m_audioOutputData.data(), len);
        SDL_PutAudioStreamData(stream, sdlPlatform->m_audioOutputData.data(), len);
      }
    }, this);
    if (!m_sdlAudioOutputStream) {
      Logger::error("Application: Could not open audio device, no sound available!");
    } else {
      Logger::info("Application: Opened default audio device with 44.1khz / 16 bit stereo audio");
      SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdlAudioOutputStream));
    }

    m_renderer = make_shared<OpenGlRenderer>();
    m_renderer->setScreenSize(m_windowSize);

    m_cursorCache.setTimeToLive(30000);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    style.ScaleAllSizes(main_scale);

    ImGui_ImplSDL3_InitForOpenGL(m_sdlWindow, m_sdlGlContext);
    ImGui_ImplOpenGL3_Init(glsl_version);

  }

  ~SdlPlatform() {

    if (m_sdlAudioOutputStream)
      SDL_CloseAudioDevice(SDL_GetAudioStreamDevice(m_sdlAudioOutputStream));

    closeAudioInputDevice();

    m_renderer.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    Logger::info("Application: Destroying SDL Window");
    SDL_GL_DestroyContext(m_sdlGlContext);
    SDL_DestroyWindow(m_sdlWindow);

    SDL_Quit();
  }
  
  typedef std::function<void(uint8_t*, int)> AudioCallback;
  bool openAudioInputDevice(SDL_AudioDeviceID deviceId, int freq, int channels, AudioCallback callback) {
    closeAudioInputDevice();
    m_audioInputCallback = std::move(callback);
    SDL_AudioSpec desired = {SDL_AUDIO_S16, channels, freq};
    m_sdlAudioInputStream = SDL_OpenAudioDeviceStream(deviceId, &desired,
    [](void* userdata, SDL_AudioStream* stream, int len, int) {
      if (len > 0) {
        auto sdlPlatform = ((SdlPlatform*)(userdata));
        sdlPlatform->m_audioInputData.resize(len);
        SDL_GetAudioStreamData(stream, sdlPlatform->m_audioInputData.data(), len);
        sdlPlatform->m_audioInputCallback(sdlPlatform->m_audioInputData.data(), len);
      }
    }, this);

    if (m_sdlAudioInputStream) {
        Logger::info("Opened audio input device '{}'", SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(m_sdlAudioInputStream)));
      SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdlAudioInputStream));
    }
    else
      Logger::info("Failed to open audio input device: {}", SDL_GetError());

    return m_sdlAudioInputStream != 0;
  }

  bool closeAudioInputDevice() {
    if (m_sdlAudioInputStream) {
      Logger::info("Closing audio input device");
      SDL_CloseAudioDevice(SDL_GetAudioStreamDevice(m_sdlAudioInputStream));
      m_sdlAudioInputStream = 0;
      return true;
    }
    return false;
  }

  void cleanup() {
    m_cursorCache.ptr(m_currentCursor);
    m_cursorCache.cleanup();
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
        cleanup();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        for (auto const& event : processEvents())
          m_application->processInput(event);

        if (m_platformServices)
          m_platformServices->update();

        if (m_cursorVisible || m_platformServices->overlayActive())
          SDL_ShowCursor();
        else
          SDL_HideCursor();

        int updatesBehind = max<int>(round(m_updateTicker.ticksBehind()), 1);
        updatesBehind = min<int>(updatesBehind, m_maxFrameSkip + 1);
        for (int i = 0; i < updatesBehind; ++i) {
          m_application->update();
          m_updateRate = m_updateTicker.tick();
        }

        m_renderer->startFrame();
        m_application->render();
        m_renderer->finishFrame();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
      Logger::error("Application: exception thrown!");
      fatalException(e, true);
    }

    try {
      Logger::info("Application: shutdown...");
      m_application->shutdown();
    } catch (std::exception const& e) {
      Logger::error("Application: threw exception during shutdown: {}", outputException(e, true));
    }

    SDL_CloseAudioDevice(SDL_GetAudioStreamDevice(m_sdlAudioOutputStream));
    m_SdlControllers.clear();

    SDL_SetCursor(NULL);
    m_cursorCache.clear();

    m_application.reset();
  }

private:
  struct Controller : public ApplicationController {
    Controller(SdlPlatform* parent)
      : parent(parent) {}

    bool hasClipboard() override {
      return SDL_HasClipboardText();
    }

    Maybe<String> getClipboard() override {
      Maybe<String> string;
      if (SDL_HasClipboardText()) {
        if (auto text = SDL_GetClipboardText()) {
          if (*text != '\0')
            string.emplace(text);
          SDL_free(text);
        }
      }
      return string;
    }

    bool setClipboard(String text) override {
      return SDL_SetClipboardText(text.utf8Ptr());
    }

    bool setClipboardData(StringMap<ByteArray> data) override {
      return parent->setClipboardData(std::move(data));
    }

    bool setClipboardImage(Image const& image, ByteArray* png) override {
      return parent->setClipboardImage(image, png);
    }

    bool setClipboardFile(String const& path) override {
      return parent->setClipboardFile(path);
    }

    bool isFocused() const override {
      return (SDL_GetWindowFlags(parent->m_sdlWindow) & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

    void setTargetUpdateRate(float targetUpdateRate) override {
      parent->m_updateTicker.setTargetTickRate(targetUpdateRate);
    }

    void setUpdateTrackWindow(float updateTrackWindow) override {
      parent->m_updateTicker.setWindow(updateTrackWindow);
    }

    void setApplicationTitle(String title) override {
      parent->m_windowTitle = std::move(title);
      if (parent->m_sdlWindow)
        SDL_SetWindowTitle(parent->m_sdlWindow, parent->m_windowTitle.utf8Ptr());
    }

    void setFullscreenWindow(Vec2U fullScreenResolution) override {
      if (parent->m_windowMode != WindowMode::Fullscreen || parent->m_windowSize != fullScreenResolution) {
        int currentDisplayIndex = SDL_GetDisplayForWindow(parent->m_sdlWindow);

        SDL_DisplayMode closestDisplayMode;
        if (SDL_GetClosestFullscreenDisplayMode(currentDisplayIndex, (int)fullScreenResolution[0], (int)fullScreenResolution[1], 0.f, true, &closestDisplayMode)) {
          if (SDL_SetWindowFullscreenMode(parent->m_sdlWindow, &closestDisplayMode) == 0) {
            if (parent->m_windowMode == WindowMode::Fullscreen)
              SDL_SetWindowFullscreen(parent->m_sdlWindow, 0);
            else if (parent->m_windowMode == WindowMode::Borderless)
              SDL_SetWindowBordered(parent->m_sdlWindow, true);
            else if (parent->m_windowMode == WindowMode::Maximized)
              SDL_RestoreWindow(parent->m_sdlWindow);
            
            parent->m_windowMode = WindowMode::Fullscreen;
            SDL_SetWindowFullscreen(parent->m_sdlWindow, SDL_WINDOW_FULLSCREEN);
          } else {
            Logger::warn("Failed to set resolution {}, {}", (unsigned)closestDisplayMode.w, (unsigned)closestDisplayMode.h);
          }
        } else {
          Logger::warn("Unable to set requested display resolution {}, {}", (int)fullScreenResolution[0], (int)fullScreenResolution[1]);
        }

        if (auto displayMode = SDL_GetWindowFullscreenMode(parent->m_sdlWindow)) {
          parent->m_windowSize = {(unsigned)displayMode->w, (unsigned)displayMode->h};

          // call these manually since no SDL_WindowEvent is triggered when changing between fullscreen resolutions for some reason
          parent->m_renderer->setScreenSize(parent->m_windowSize);
          parent->m_application->windowChanged(parent->m_windowMode, parent->m_windowSize);
        } else {
          Logger::error("Couldn't get window display mode!");
        }
      }
    }

    void setNormalWindow(Vec2U windowSize) override {
      auto window = parent->m_sdlWindow;
      if (parent->m_windowMode != WindowMode::Normal || parent->m_windowSize != windowSize) {
        if (parent->m_windowMode == WindowMode::Fullscreen)
          SDL_SetWindowFullscreen(window, 0);
        else if (parent->m_windowMode == WindowMode::Borderless)
          SDL_SetWindowBordered(window, true);
        else if (parent->m_windowMode == WindowMode::Maximized)
          SDL_RestoreWindow(window);

        SDL_SetWindowBordered(window, true);
        SDL_SetWindowSize(window, windowSize[0], windowSize[1]);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        parent->m_windowMode = WindowMode::Normal;
        parent->m_windowSize = windowSize;
      }
    }

    void setMaximizedWindow() override {
      if (parent->m_windowMode != WindowMode::Maximized) {
        if (parent->m_windowMode == WindowMode::Fullscreen)
          SDL_SetWindowFullscreen(parent->m_sdlWindow, 0);
        else if (parent->m_windowMode == WindowMode::Borderless)
          SDL_SetWindowBordered(parent->m_sdlWindow, true);

        SDL_RestoreWindow(parent->m_sdlWindow);
        SDL_MaximizeWindow(parent->m_sdlWindow);
        parent->m_windowMode = WindowMode::Maximized;
      }
    }

    void setBorderlessWindow() override {
      if (parent->m_windowMode != WindowMode::Borderless) {
        if (parent->m_windowMode == WindowMode::Fullscreen)
          SDL_SetWindowFullscreen(parent->m_sdlWindow, 0);
        else if (parent->m_windowMode == WindowMode::Maximized)
          SDL_RestoreWindow(parent->m_sdlWindow);

        SDL_SetWindowBordered(parent->m_sdlWindow, false);
        parent->m_windowMode = WindowMode::Borderless;

        if (auto displayMode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(parent->m_sdlWindow))) {
          parent->m_windowSize = {(unsigned)displayMode->w, (unsigned)displayMode->h};

          SDL_SetWindowPosition(parent->m_sdlWindow, 0, 0);
          SDL_SetWindowSize(parent->m_sdlWindow, parent->m_windowSize[0], parent->m_windowSize[1]);
          parent->m_renderer->setScreenSize(parent->m_windowSize);
          parent->m_application->windowChanged(parent->m_windowMode, parent->m_windowSize);
        } else {
          Logger::error("Couldn't get desktop display mode!");
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

    void setCursorPosition(Vec2I cursorPosition) override {
      SDL_WarpMouseInWindow(parent->m_sdlWindow, cursorPosition[0], cursorPosition[1]);
    }

    void setCursorHardware(bool hardware) override {
      parent->m_cursorHardware = hardware;
    }

    bool setCursorImage(const String& id, const ImageConstPtr& image, unsigned scale, const Vec2I& offset) override {
      return parent->setCursorImage(id, image, scale, offset);
    }

    void setAcceptingTextInput(bool acceptingTextInput) override {
      if (acceptingTextInput != parent->m_acceptingTextInput) {
        if (acceptingTextInput)
          SDL_StartTextInput(parent->m_sdlWindow);
        else
          SDL_StopTextInput(parent->m_sdlWindow);

        parent->m_acceptingTextInput = acceptingTextInput;
      }
    }

    void setTextArea(Maybe<pair<RectI, int>> area) override {
      if (parent->m_textInputArea == area)
        return;
      parent->m_textInputArea = area;
      if (area) {
        RectI& r = area->first;
        SDL_Rect rect{
          r.xMin(), (int)parent->m_windowSize.y() - r.yMax(),
          r.width(), r.height()
        };
        SDL_SetTextInputArea(parent->m_sdlWindow, &rect, area->second);
      } else {
        SDL_SetTextInputArea(parent->m_sdlWindow, NULL, 0);
      }
    }

    AudioFormat enableAudio() override {
      parent->m_audioEnabled = true;
      SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(parent->m_sdlAudioOutputStream));
      return AudioFormat{44100, 2};
    }

    void disableAudio() override {
      parent->m_audioEnabled = false;
      SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(parent->m_sdlAudioOutputStream));
    }

    bool openAudioInputDevice(uint32_t deviceId, int freq, int channels, AudioCallback callback) override {
      return parent->openAudioInputDevice(deviceId, freq, channels, callback);
    };

    bool closeAudioInputDevice() override {
      return parent->closeAudioInputDevice();
    };

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

    ImGuiIO& io = ImGui::GetIO();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      Maybe<InputEvent> starEvent;
      if (event.type == SDL_EVENT_WINDOW_MAXIMIZED || event.type == SDL_EVENT_WINDOW_RESTORED) {
        auto windowFlags = SDL_GetWindowFlags(m_sdlWindow);

        if (windowFlags & SDL_WINDOW_MAXIMIZED)
          m_windowMode = WindowMode::Maximized;
        else if (windowFlags & SDL_WINDOW_FULLSCREEN)
          m_windowMode = WindowMode::Fullscreen;
        else if (windowFlags & SDL_WINDOW_BORDERLESS)
          m_windowMode = WindowMode::Borderless;
        else
          m_windowMode = WindowMode::Normal;

        m_application->windowChanged(m_windowMode, m_windowSize);
      } else if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        m_windowSize = Vec2U(event.window.data1, event.window.data2);
        m_renderer->setScreenSize(m_windowSize);
        m_application->windowChanged(m_windowMode, m_windowSize);
      }
      else if (event.type == SDL_EVENT_KEY_DOWN && !io.WantCaptureKeyboard) {
        if (!event.key.repeat) {
          if (auto key = keyFromSdlKeyCode(event.key.key))
            starEvent.set(KeyDownEvent{*key, keyModsFromSdlKeyMods(event.key.mod)});
        }
      } else if (event.type == SDL_EVENT_KEY_UP) {
        if (auto key = keyFromSdlKeyCode(event.key.key))
          starEvent.set(KeyUpEvent{*key});
      } else if (event.type == SDL_EVENT_TEXT_INPUT && !io.WantCaptureKeyboard) {
        starEvent.set(TextInputEvent{String(event.text.text)});
      } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        starEvent.set(MouseMoveEvent{
            {event.motion.xrel, -event.motion.yrel}, {event.motion.x, (int)m_windowSize[1] - event.motion.y}});
      } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !io.WantCaptureMouse) {
        starEvent.set(MouseButtonDownEvent{mouseButtonFromSdlMouseButton(event.button.button),
            {event.button.x, (int)m_windowSize[1] - event.button.y}});
      } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && !io.WantCaptureMouse) {
        starEvent.set(MouseButtonUpEvent{mouseButtonFromSdlMouseButton(event.button.button),
            {event.button.x, (int)m_windowSize[1] - event.button.y}});
      } else if (event.type == SDL_EVENT_MOUSE_WHEEL && !io.WantCaptureMouse) {
        starEvent.set(MouseWheelEvent{event.wheel.y < 0 ? MouseWheel::Down : MouseWheel::Up,
          {event.wheel.mouse_x, (int)m_windowSize[1] - event.wheel.mouse_y}});
      } else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        starEvent.set(ControllerAxisEvent{
          (ControllerId)event.gaxis.which,
          controllerAxisFromSdlControllerAxis(event.gaxis.axis),
          (float)event.gaxis.value / 32768.0f
        });
      } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        starEvent.set(ControllerButtonDownEvent{ (ControllerId)event.gbutton.which, controllerButtonFromSdlControllerButton(event.gbutton.button) });
      } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
        starEvent.set(ControllerButtonUpEvent{ (ControllerId)event.gbutton.which, controllerButtonFromSdlControllerButton(event.gbutton.button) });
      } else if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        auto insertion = m_SdlControllers.insert_or_assign(event.gdevice.which, SDLGameControllerUPtr(SDL_OpenGamepad(event.gdevice.which), SDL_CloseGamepad));
        if (SDL_Gamepad* controller = insertion.first->second.get())
          Logger::info("Controller device '{}' added", SDL_GetGamepadName(controller));
      } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        auto find = m_SdlControllers.find(event.gdevice.which);
        if (find != m_SdlControllers.end()) {
          if (SDL_Gamepad* controller = find->second.get())
            Logger::info("Controller device '{}' removed", SDL_GetGamepadName(controller));
          m_SdlControllers.erase(event.gdevice.which);
        }
      } else if (event.type == SDL_EVENT_QUIT) {
        m_quitRequested = true;
        starEvent.reset();
      }

      if (starEvent)
        inputEvents.append(starEvent.take());
    }

    return inputEvents;
  }

        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, "Test");

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
      Logger::info("Application: Enabling adaptive VSync");
      if (!SDL_GL_SetSwapInterval(-1)) {
        Logger::info("Application: Enabling adaptive VSync, falling back to full VSync");
        SDL_GL_SetSwapInterval(1);
      }
    } else {
      Logger::info("Application: Disabling VSync");
      SDL_GL_SetSwapInterval(0);
    }
  }

  inline static const size_t MaxCursorSize = 128;
  bool setCursorImage(const String& id, const ImageConstPtr& image, unsigned scale, const Vec2I& offset) {
    auto imageSize = image->size().piecewiseMultiply(Vec2U::filled(scale));
    if (!m_cursorHardware || !scale || imageSize.max() > MaxCursorSize || imageSize.product() > square(MaxCursorSize)) {
      if (auto defaultCursor = SDL_GetDefaultCursor()) {
        if (SDL_GetCursor() != defaultCursor)
          SDL_SetCursor(defaultCursor);
      }
      return m_cursorVisible = false;
    }

    auto& entry = m_cursorCache.get(m_currentCursor = { scale, offset, id }, [&](auto const&) {
      auto entry = std::make_shared<CursorEntry>();
      List<ImageOperation> operations;
      if (scale != 1)
        operations = {
          FlipImageOperation{ FlipImageOperation::Mode::FlipY }, // SDL wants an Australian cursor.
          BorderImageOperation{ 1, Vec4B(), Vec4B(), false, false }, // Nearest scaling fucks up and clips half off the edges, work around this with border+crop for now.
          ScaleImageOperation{ ScaleImageOperation::Mode::Nearest, Vec2F::filled(scale) },
          CropImageOperation{ RectI::withSize(Vec2I::filled(ceilf((float)scale / 2)), Vec2I(imageSize)) }
        };
      else
        operations = { FlipImageOperation{ FlipImageOperation::Mode::FlipY } };

      auto newImage = std::make_shared<Image>(processImageOperations(operations, *image));
      // Fix fully transparent pixels inverting the underlying display pixel on Windows (allowing this could be made configurable per cursor later!)
      newImage->forEachPixel([](unsigned /*x*/, unsigned /*y*/, Vec4B& pixel) { if (!pixel[3]) pixel[0] = pixel[1] = pixel[2] = 0; });
      entry->image = std::move(newImage);
      

      auto size = entry->image->size();
      SDL_PixelFormat pixelFormat;
      switch (entry->image->pixelFormat()) {
        case PixelFormat::RGB24: // I know this conversion looks wrong, but it's correct. I'm confused too.
          pixelFormat = SDL_PIXELFORMAT_XBGR8888;
          break;
        case PixelFormat::RGBA32:
          pixelFormat = SDL_PIXELFORMAT_ABGR8888;
          break;
        case PixelFormat::BGR24:
          pixelFormat = SDL_PIXELFORMAT_XRGB8888;
          break;
        case PixelFormat::BGRA32:
          pixelFormat = SDL_PIXELFORMAT_ARGB8888;
          break;
        default:
          pixelFormat = SDL_PIXELFORMAT_UNKNOWN;
      }

      entry->sdlSurface.reset(SDL_CreateSurfaceFrom(
        size[0], size[1],
        pixelFormat,
        (void*)entry->image->data(),
        entry->image->bytesPerPixel() * size[0])
      );
      entry->sdlCursor.reset(SDL_CreateColorCursor(entry->sdlSurface.get(), offset[0] * scale, offset[1] * scale));

      return entry;
    });

    SDL_SetCursor(entry->sdlCursor.get());
    return m_cursorVisible = true;
  }

  bool setClipboardData(StringMap<ByteArray> data) {
    auto heldData = new StringMap<ByteArray>(std::move(data));
    std::vector<const char*> types;
    for (auto& entry : *heldData)
      types.push_back(entry.first.utf8Ptr());
    auto request = [](void* userdata, const char* mime_type, size_t* size) -> const void* {
      if (auto entry = ((StringMap<ByteArray>*)userdata)->ptr(mime_type)) {
        *size = entry->size();
        return entry->ptr();
      }
      *size = 0;
      return NULL;
    };
    auto cleanup = [](void* userdata) { delete ((StringMap<ByteArray>*)userdata); };
    if (SDL_SetClipboardData(request, cleanup, heldData, types.data(), types.size()))
      return true;

    cleanup(heldData);
    return false;
  }

  bool setClipboardImage(Image const& image, ByteArray* png) {
    #ifdef STAR_SYSTEM_WINDOWS // wow, SDL3's implementation is so bad!!
    return duringClipboard(m_sdlWindow, [&]() {
      copyDibToClipboard(image.data(), image.width(), image.height());
      copyPngToClipboard(png->ptr(), png->size());
    });
    #else
    _unused(image);
    if (png) {
      StringMap<ByteArray> clipboardData = {{"image/png", std::move(*png)}};
      return setClipboardData(std::move(clipboardData));
    }
    return false;
    #endif
  }

  bool setClipboardFile(String const& path) {
    #ifdef STAR_SYSTEM_WINDOWS
    return duringClipboard(m_sdlWindow, [&]() {
      DROPFILES drop{};
      drop.pFiles = sizeof(DROPFILES);
      drop.pt = {0, 0};
      drop.fNC = false;
      drop.fWide = true;
      auto wide = path.wstring();
      wide.push_back(0);
      wide.push_back(0);
      size_t wideSize = wide.size() * sizeof(std::wstring::value_type);
      if (HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, sizeof(DROPFILES) + wideSize)) {
        if (auto dst = (char*)GlobalLock(handle)) {
          memcpy(dst, &drop, sizeof(drop));
          memcpy(dst + sizeof(DROPFILES), wide.data(), wideSize);
          GlobalUnlock(handle);
        }
        SetClipboardData(CF_HDROP, handle);
        return true;
      }
      return false;
    });
    #else
    _unused(path);
    return false;
    #endif
  }

  SignalHandler m_signalHandler;

  TickRateApproacher m_updateTicker = TickRateApproacher(60.0f, 1.0f);
  float m_updateRate = 0.0f;
  TickRateMonitor m_renderTicker = TickRateMonitor(1.0f);
  float m_renderRate = 0.0f;

  SDL_Window* m_sdlWindow = nullptr;
  SDL_GLContext m_sdlGlContext = nullptr;
  SDL_AudioStream* m_sdlAudioOutputStream = 0;
  SDL_AudioStream* m_sdlAudioInputStream = 0;
  AudioCallback m_audioInputCallback;
  std::vector<uint8_t> m_audioInputData;
  std::vector<uint8_t> m_audioOutputData;
  Maybe<pair<RectI, int>> m_textInputArea;

  typedef std::unique_ptr<SDL_Gamepad, decltype(&SDL_CloseGamepad)> SDLGameControllerUPtr;
  StableHashMap<int, SDLGameControllerUPtr> m_SdlControllers;

  typedef std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> SDLSurfaceUPtr;
  typedef std::unique_ptr<SDL_Cursor, decltype(&SDL_DestroyCursor)> SDLCursorUPtr;
  struct CursorEntry {
    ImageConstPtr image = nullptr;
    SDLSurfaceUPtr sdlSurface;
    SDLCursorUPtr sdlCursor;

    CursorEntry() : image(nullptr), sdlSurface(nullptr, SDL_DestroySurface), sdlCursor(nullptr, SDL_DestroyCursor) {};
  };

  typedef tuple<unsigned, Vec2I, String> CursorDescriptor;

  HashTtlCache<CursorDescriptor, std::shared_ptr<CursorEntry>> m_cursorCache;
  CursorDescriptor m_currentCursor;

  Vec2U m_windowSize = {800, 600};
  WindowMode m_windowMode = WindowMode::Normal;

  String m_windowTitle = "Starbound";
  bool m_windowVSync = true;
  unsigned m_maxFrameSkip = 5;
  bool m_cursorVisible = true;
  bool m_cursorHardware = true;
  bool m_acceptingTextInput = false;
  bool m_audioEnabled = false;
  bool m_quitRequested = false;

  OpenGlRendererPtr m_renderer;
  ApplicationUPtr m_application;
  PcPlatformServicesUPtr m_platformServices;
};

int runMainApplication(ApplicationUPtr application, StringList cmdLineArgs) {
  try {
    {
      SdlPlatform platform(std::move(application), std::move(cmdLineArgs));
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
