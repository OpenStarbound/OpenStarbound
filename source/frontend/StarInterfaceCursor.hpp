#pragma once

#include "StarJson.hpp"
#include "StarAnimation.hpp"

namespace Star {

class InterfaceCursor {
public:
  InterfaceCursor();

  // Sets the cursor to the default defined in interface.config
  void resetCursor();

  // Sets the cursor config to the given config IF the config is different than
  // the current one.  Expects a full asset path to the cursor config.
  void setCursor(String const& configFile);

  Drawable drawable() const;
  Vec2I size() const;
  Vec2I offset() const;
  unsigned int scale(unsigned int interfaceScale = 0) const;

  void update(float dt);

private:
  String m_configFile;
  Vec2I m_offset;
  Vec2I m_size;
  unsigned int m_scale;
  MVariant<String, Animation> m_drawable;
};

}
