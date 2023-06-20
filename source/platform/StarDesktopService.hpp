#ifndef STAR_DESKTOP_SERVICE_HPP
#define STAR_DESKTOP_SERVICE_HPP

namespace Star {

STAR_CLASS(DesktopService);

class DesktopService {
public:
  ~DesktopService() = default;

  virtual void openUrl(String const& url) = 0;
};

}

#endif
