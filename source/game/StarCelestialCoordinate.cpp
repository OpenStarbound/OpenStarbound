#include "StarCelestialCoordinate.hpp"
#include "StarCelestialTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarLexicalCast.hpp"
#include "StarStaticRandom.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

CelestialCoordinate::CelestialCoordinate() : m_planetaryOrbitNumber(0), m_satelliteOrbitNumber(0) {}

CelestialCoordinate::CelestialCoordinate(Vec3I location, int planetaryOrbitNumber, int satelliteOrbitNumber)
  : m_location(std::move(location)),
    m_planetaryOrbitNumber(planetaryOrbitNumber),
    m_satelliteOrbitNumber(satelliteOrbitNumber) {}

CelestialCoordinate::CelestialCoordinate(Json const& variant) : CelestialCoordinate() {
  if (variant.isType(Json::Type::String)) {
    String id = variant.toString();
    if (!id.empty() && !id.equalsIgnoreCase("null")) {
      try {
        auto plist = id.splitAny(" _:");

        m_location[0] = lexicalCast<int>(plist.at(0));
        m_location[1] = lexicalCast<int>(plist.at(1));
        m_location[2] = lexicalCast<int>(plist.at(2));

        if (plist.size() > 3)
          m_planetaryOrbitNumber = lexicalCast<int>(plist.at(3));
        if (plist.size() > 4)
          m_satelliteOrbitNumber = lexicalCast<int>(plist.at(4));

        if (m_planetaryOrbitNumber <= 0)
          throw CelestialException(strf("Planetary body number out of range in '{}'", id));
        if (m_satelliteOrbitNumber < 0)
          throw CelestialException(strf("Satellite body number out of range in '{}'", id));
      } catch (StarException const& e) {
        throw CelestialException(strf("Error parsing CelestialCoordinate from '{}'", id), e);
      }
    }
  } else if (variant.isType(Json::Type::Object)) {
    m_location = jsonToVec3I(variant.get("location"));
    m_planetaryOrbitNumber = variant.getInt("planet", 0);
    m_satelliteOrbitNumber = variant.getInt("satellite", 0);
  } else if (!variant.isNull()) {
    throw CelestialException(
        strf("Improper variant type {} trying to convert to SystemCoordinate", variant.typeName()));
  }
}

bool CelestialCoordinate::isNull() const {
  return m_location == Vec3I() && m_planetaryOrbitNumber == 0 && m_satelliteOrbitNumber == 0;
}

bool CelestialCoordinate::isSystem() const {
  return !isNull() && m_planetaryOrbitNumber == 0;
}

bool CelestialCoordinate::isPlanetaryBody() const {
  return !isNull() && m_planetaryOrbitNumber != 0 && m_satelliteOrbitNumber == 0;
}

bool CelestialCoordinate::isSatelliteBody() const {
  return !isNull() && m_planetaryOrbitNumber != 0 && m_satelliteOrbitNumber != 0;
}

Vec3I CelestialCoordinate::location() const {
  return m_location;
}

CelestialCoordinate CelestialCoordinate::system() const {
  if (isNull())
    throw CelestialException("CelestialCoordinate::system() called on null coordinate");
  return CelestialCoordinate(m_location);
}

CelestialCoordinate CelestialCoordinate::planet() const {
  if (isPlanetaryBody())
    return *this;
  if (isSatelliteBody())
    return CelestialCoordinate(m_location, m_planetaryOrbitNumber);
  throw CelestialException("CelestialCoordinate::planet() called on null or system coordinate type");
}

int CelestialCoordinate::orbitNumber() const {
  if (isSatelliteBody())
    return m_satelliteOrbitNumber;
  if (isPlanetaryBody())
    return m_planetaryOrbitNumber;
  if (isSystem())
    return 0;
  throw CelestialException("CelestialCoordinate::orbitNumber() called on null coordinate");
}

CelestialCoordinate CelestialCoordinate::parent() const {
  if (isSatelliteBody())
    return CelestialCoordinate(m_location, m_planetaryOrbitNumber);
  if (isPlanetaryBody())
    return CelestialCoordinate(m_location);
  throw CelestialException("CelestialCoordinate::parent() called on null or system coordinate");
}

CelestialCoordinate CelestialCoordinate::child(int orbitNumber) const {
  if (isSystem())
    return CelestialCoordinate(m_location, orbitNumber);
  if (isPlanetaryBody())
    return CelestialCoordinate(m_location, m_planetaryOrbitNumber, orbitNumber);
  throw CelestialException("CelestialCoordinate::child called on null or satellite coordinate");
}

Json CelestialCoordinate::toJson() const {
  if (isNull()) {
    return Json();
  } else {
    return JsonObject{{"location", jsonFromVec3I(m_location)},
        {"planet", m_planetaryOrbitNumber},
        {"satellite", m_satelliteOrbitNumber}};
  }
}

String CelestialCoordinate::id() const {
  return toString(*this);
}

double CelestialCoordinate::distance(CelestialCoordinate const& rhs) const {
  return Vec2D(m_location[0] - rhs.m_location[0], m_location[1] - rhs.m_location[1]).magnitude();
}

String CelestialCoordinate::filename() const {
  return id().replace(":", "_");
}

CelestialCoordinate::operator bool() const {
  return !isNull();
}

bool CelestialCoordinate::operator<(CelestialCoordinate const& rhs) const {
  return tie(m_location, m_planetaryOrbitNumber, m_satelliteOrbitNumber)
      < tie(rhs.m_location, rhs.m_planetaryOrbitNumber, rhs.m_satelliteOrbitNumber);
}

bool CelestialCoordinate::operator==(CelestialCoordinate const& rhs) const {
  return tie(m_location, m_planetaryOrbitNumber, m_satelliteOrbitNumber)
      == tie(rhs.m_location, rhs.m_planetaryOrbitNumber, rhs.m_satelliteOrbitNumber);
}

bool CelestialCoordinate::operator!=(CelestialCoordinate const& rhs) const {
  return tie(m_location, m_planetaryOrbitNumber, m_satelliteOrbitNumber)
      != tie(rhs.m_location, rhs.m_planetaryOrbitNumber, rhs.m_satelliteOrbitNumber);
}

std::ostream& operator<<(std::ostream& os, CelestialCoordinate const& coord) {
  if (coord.isNull()) {
    os << "null";
  } else {
    format(os, "{}:{}:{}", coord.m_location[0], coord.m_location[1], coord.m_location[2]);

    if (coord.m_planetaryOrbitNumber) {
      format(os, ":{}", coord.m_planetaryOrbitNumber);
      if (coord.m_satelliteOrbitNumber)
        format(os, ":{}", coord.m_satelliteOrbitNumber);
    }
  }

  return os;
}

DataStream& operator>>(DataStream& ds, CelestialCoordinate& coordinate) {
  ds.read(coordinate.m_location);
  ds.read(coordinate.m_planetaryOrbitNumber);
  ds.read(coordinate.m_satelliteOrbitNumber);

  return ds;
}

DataStream& operator<<(DataStream& ds, CelestialCoordinate const& coordinate) {
  ds.write(coordinate.m_location);
  ds.write(coordinate.m_planetaryOrbitNumber);
  ds.write(coordinate.m_satelliteOrbitNumber);

  return ds;
}

}
