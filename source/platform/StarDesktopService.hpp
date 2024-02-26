#pragma once

namespace Star {

STAR_CLASS(DesktopService);

class DesktopService {
public:
  ~DesktopService() = default;

  virtual void openUrl(String const& url) = 0;
};

}
