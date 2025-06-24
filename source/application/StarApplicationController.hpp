#pragma once

#include "StarApplication.hpp"
#include "StarStatisticsService.hpp"
#include "StarP2PNetworkingService.hpp"
#include "StarUserGeneratedContentService.hpp"
#include "StarDesktopService.hpp"
#include "StarImage.hpp"
#include "StarRect.hpp"

namespace Star {

STAR_CLASS(ApplicationController);

// Audio format is always 16 bit signed integer samples
struct AudioFormat {
  unsigned sampleRate;
  unsigned channels;
};

// Window size defaults to 800x600, target update rate to 60hz, maximized and
// fullscreen are false, vsync is on, the cursor is visible, and audio and text
// input are disabled.
class ApplicationController {
public:
  virtual ~ApplicationController() = default;

  // Target hz at which update() will be called
  virtual void setTargetUpdateRate(float targetUpdateRate) = 0;
  // Window that controls how long the update rate will be increased or
  // decreased to make up for rate errors in the past.
  virtual void setUpdateTrackWindow(float updateTrackWindow) = 0;
  // Maximum number of calls to update() that can occur before we force
  // 'render()' to be called, even if we are still behind on our update rate.
  virtual void setMaxFrameSkip(unsigned maxFrameSkip) = 0;

  virtual void setApplicationTitle(String title) = 0;
  virtual void setFullscreenWindow(Vec2U fullScreenResolution) = 0;
  virtual void setNormalWindow(Vec2U windowSize) = 0;
  virtual void setMaximizedWindow() = 0;
  virtual void setBorderlessWindow() = 0;
  virtual void setVSyncEnabled(bool vSync) = 0;
  virtual void setCursorVisible(bool cursorVisible) = 0;
  virtual void setCursorPosition(Vec2I cursorPosition) = 0;
  virtual void setCursorHardware(bool cursorHardware) = 0;
  virtual bool setCursorImage(const String& id, const ImageConstPtr& image, unsigned scale, const Vec2I& offset) = 0;
  virtual void setAcceptingTextInput(bool acceptingTextInput) = 0;
  virtual void setTextArea(Maybe<pair<RectI, int>> area = {}) = 0;


  virtual AudioFormat enableAudio() = 0;
  virtual void disableAudio() = 0;

  typedef std::function<void(uint8_t*, int)> AudioCallback;
  virtual bool openAudioInputDevice(uint32_t deviceId, int freq, int channels, AudioCallback callback) = 0;
  virtual bool closeAudioInputDevice() = 0;

  virtual bool hasClipboard() = 0;
  virtual void setClipboard(String text) = 0;
  virtual Maybe<String> getClipboard() = 0;

  virtual bool isFocused() const = 0;

  // Returns the latest actual measured update and render rate, which may be
  // different than the target update rate.
  virtual float updateRate() const = 0;
  virtual float renderFps() const = 0;

  virtual StatisticsServicePtr statisticsService() const = 0;
  virtual P2PNetworkingServicePtr p2pNetworkingService() const = 0;
  virtual UserGeneratedContentServicePtr userGeneratedContentService() const = 0;
  virtual DesktopServicePtr desktopService() const = 0;

  // Signals the application to quit
  virtual void quit() = 0;
};

}
