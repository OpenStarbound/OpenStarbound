#ifndef STAR_CONFIGURATION_HPP
#define STAR_CONFIGURATION_HPP

#include "StarJson.hpp"
#include "StarThread.hpp"
#include "StarVersion.hpp"

namespace Star {

STAR_CLASS(Configuration);

STAR_EXCEPTION(ConfigurationException, StarException);

class Configuration {
public:
  Configuration(Json defaultConfiguration, Json currentConfiguration);

  Json defaultConfiguration() const;
  Json currentConfiguration() const;
  String printConfiguration() const;

  Json get(String const& key) const;
  Json getPath(String const& path) const;

  Json getDefault(String const& key) const;
  Json getDefaultPath(String const& path) const;

  void set(String const& key, Json const& value);
  void setPath(String const& path, Json const& value);

private:
  mutable Mutex m_mutex;

  Json m_defaultConfig;
  Json m_currentConfig;
};

}

#endif
