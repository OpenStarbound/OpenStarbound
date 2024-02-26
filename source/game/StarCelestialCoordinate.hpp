#pragma once

#include "StarJson.hpp"
#include "StarVector.hpp"

namespace Star {

STAR_CLASS(CelestialCoordinate);

STAR_EXCEPTION(CelestialException, StarException);

// Specifies coordinates to either a planetary system, a planetary body, or a
// satellite around such a planetary body.  The terms here are meant to be very
// generic, a "planetary body" could be an asteroid field, or a ship, or
// anything in orbit around the center of mass of a specific planetary system.
// The terms are really simply meant as a hierarchy of orbits.
//
// No validity checking is done here, any coordinate to any body whether it
// exists in a specific universe or not can be expressed.  isNull() simply
// checks whether the coordinate is the result of the empty constructor, not
// whether the coordinate points to a valid object or not.
class CelestialCoordinate {
public:
  // Creates the null CelestialCoordinate
  CelestialCoordinate();
  CelestialCoordinate(Vec3I location, int planetaryOrbitNumber = 0, int satelliteOrbitNumber = 0);
  explicit CelestialCoordinate(Json const& json);

  // Is this coordanate the null coordinate?
  bool isNull() const;

  // Does this coordinate point to an entire planetary system?
  bool isSystem() const;
  // Is this world a body whose "designated gravity buddy" is the center of a
  // planetary system?
  bool isPlanetaryBody() const;
  // Is this world a body which orbits around a planetary body?
  bool isSatelliteBody() const;

  Vec3I location() const;

  // Returns just the system coordinate portion of this celestial coordinate.
  CelestialCoordinate system() const;

  // Returns just the planet portion of this celestial coordinate, throws
  // exception if this is a system coordinate.
  CelestialCoordinate planet() const;

  // Returns the orbit number for this body.  Returns 0 for system coordinates.
  int orbitNumber() const;

  // Returns the system for a planet or the planet for a satellite.  If this is
  // a system coordinate, throws an exception.
  CelestialCoordinate parent() const;

  // Returns a coordinate to a child object at the given orbit number.  If the
  // orbit number is 0, returns *this, otherwise if this is a satellite throws
  // an exception.
  CelestialCoordinate child(int orbitNumber) const;

  // Stores coordinate in json form that can be used to reconstruct it.
  Json toJson() const;

  // Returns coordinate in a parseable String format.
  String id() const;

  // Returns a fakey fake distance
  double distance(CelestialCoordinate const& rhs) const;

  // Returns a slightly different string format than id(), which is still in an
  // accepted format, but more appropriate for filenames.
  String filename() const;

  // Returns true if not null
  explicit operator bool() const;

  bool operator<(CelestialCoordinate const& rhs) const;
  bool operator==(CelestialCoordinate const& rhs) const;
  bool operator!=(CelestialCoordinate const& rhs) const;

  // Prints in the same format accepted by parser.  Each coordinate is unique.
  friend std::ostream& operator<<(std::ostream& os, CelestialCoordinate const& coord);

  friend DataStream& operator>>(DataStream& ds, CelestialCoordinate& coordinate);
  friend DataStream& operator<<(DataStream& ds, CelestialCoordinate const& coordinate);

private:
  Vec3I m_location;
  int m_planetaryOrbitNumber;
  int m_satelliteOrbitNumber;
};

}

template <> struct fmt::formatter<Star::CelestialCoordinate> : ostream_formatter {};
