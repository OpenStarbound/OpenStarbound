#ifndef STAR_CELESTIAL_GENERATION_HPP
#define STAR_CELESTIAL_GENERATION_HPP

#include "StarCelestialDatabase.hpp"

namespace Star {

STAR_CLASS(CelestialDatabase);

// Functions for generating and drawing worlds from a celestial database.
// Guards against drawing unloaded celestial objects, will return empty if no
// information is returned from the celestial database.
//
// Drawing methods return the stack of images to draw and the scale to draw
// them at.
class CelestialGraphics {
public:
  // Some static versions of drawing functions are given that do not require an
  // active CelestialDatabasePtr to draw.

  static List<pair<String, float>> drawSystemPlanetaryObject(CelestialParameters const& celestialParameters);
  static List<pair<String, float>> drawSystemCentralBody(CelestialParameters const& celestialParameters);

  // Specify the shadowing parameters in order to use the shadowing
  // information from that body instead of the primary one.
  static List<pair<String, float>> drawWorld(
      CelestialParameters const& celestialParameters, Maybe<CelestialParameters> const& shadowingParameters = {});
  static List<pair<String, String>> worldHorizonImages(CelestialParameters const& celestialParameters);
  static int worldRadialPosition(CelestialParameters const& celestialParameters);

  // Each orbiting body will occupy a unique orbital slot, but to give
  // graphical diversity, will also fit into exactly one radial slot for
  // display purposes.  The range of radial numbers is [0, RadialPosiitons)
  static int planetRadialPositions();
  static int satelliteRadialPositions();

  static List<pair<String, float>> drawSystemTwinkle(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& system, double twinkleTime);

  // Returns the small graphic for the given planetary object appropriate for a
  // system-level view.
  static List<pair<String, float>> drawSystemPlanetaryObject(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate);
  static List<pair<String, float>> drawSystemCentralBody(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate);

  // Returns the graphics appropriate to draw an entire world (planetary object
  // or satellite object) in a map view.  Shadows the satellite the same as
  // its parent planetary object.
  static List<pair<String, float>> drawWorld(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate);

  // Draw all of the left and right image pairs for all the layers for the
  // world horizon.
  static List<pair<String, String>> worldHorizonImages(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate);

  static int worldRadialPosition(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate);

private:
};

}

#endif
