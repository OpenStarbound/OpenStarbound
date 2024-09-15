#pragma once
#include "StarVersion.hpp"
#include "StarHash.hpp"

namespace Star {

extern VersionNumber const OpenProtocolVersion;

constexpr VersionNumber AnyVersion = 0xFFFFFFFF;
constexpr VersionNumber LegacyVersion = 0;

class NetCompatibilityRules {
public:
  NetCompatibilityRules();
  NetCompatibilityRules(uint64_t) = delete;
  NetCompatibilityRules(VersionNumber version);

  VersionNumber version() const;
  void setVersion(VersionNumber version);
  bool isLegacy() const;

  bool operator==(NetCompatibilityRules const& a) const;

private:
  VersionNumber m_version = OpenProtocolVersion;
};

inline NetCompatibilityRules::NetCompatibilityRules() : m_version(OpenProtocolVersion) {}

inline NetCompatibilityRules::NetCompatibilityRules(VersionNumber v) : m_version(v) {}

inline VersionNumber NetCompatibilityRules::version() const {
  return m_version;
}

inline void NetCompatibilityRules::setVersion(VersionNumber version) {
  m_version = version;
}

inline bool NetCompatibilityRules::isLegacy() const {
  return m_version == LegacyVersion;
}

inline bool NetCompatibilityRules::operator==(NetCompatibilityRules const& a) const {
  return m_version == a.m_version;
}

template <>
struct hash<NetCompatibilityRules> {
  size_t operator()(NetCompatibilityRules const& s) const {
    return s.version();
  }
};

}