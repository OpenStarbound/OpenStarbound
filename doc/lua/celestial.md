# celestial

The *celestial* table contains functions that relate to the client sky, flying the player ship, system positions for planets, system objects, and the celestial database. It is available in the following contexts:

* script pane

---

#### `bool` celestial.skyFlying()

Returns whether the client sky is currently flying.
---

#### `String` celestial.skyFlyingType()

Returns the type of flying the client sky is currently performing.
---

#### `String` celestial.skyWarpPhase()

Returns the current warp phase of the client sky, if warping.
---

#### `float` celestial.skyWarpProgress()

Returns a value between 0 and 1 for how far through warping the sky is currently.
---

#### `bool` celestial.skyInHyperspace()

Returns whether the sky is currently under hyperspace flight.
---

#### `void` celestial.flyShip(`Vec3I` system, `SystemLocation` destination)

Flies the player ship to the specified SystemLocation in the specified system.

SystemLocation is either of the following types: Null, CelestialCoordinate, Object, Vec2F

The locations are specified as a pair of type and value

```
local system = celestial.currentSystem().location
local location = nil -- Null
location = {"coordinate", {location = system, planet = 1, satellite = 0}} -- CelestialCoordinate
location = {"object", "11112222333344445555666677778888"} -- Object (UUID)
location = {0.0, 0.0} -- Vec2F (position in space)
celestial.flyShip(system, location)
```

---

#### `bool` celestial.flying()

Returns whether the player ship is flying
---

#### `Vec2F` celestial.shipSystemPosition()

Returns the current position of the ship in the system.
---

#### `SystemLocation` celestial.shipDestination()

Returns the current destination of the player ship.
---

#### `SystemLocation` celestial.shipLocation()

Returns the current system location of the player ship.
---

#### `CelestialCoordinate` celestial.currentSystem()

Returns the CelestialCoordinate for system the ship is currently in.
---

#### `float` celestial.planetSize(`CelestialCoordinate` planet)

Returns the diameter of the specified planet in system space.
---

#### `Vec2F` celestial.planetPosition(`CelestialCoordinate` planet)

Returns the position of the specified planet in system space.
---

#### `CelestialParameters` celestial.planetParameters(`CelestialCoordinate` planet)

Returns the celestial parameters for the specified planet.
---

#### `VisitableParameters` celestial.visitableParameters(`CelestialCoordinate` planet)

Returns the visitable parameters for the specified visitable planet. For unvisitable planets, returns nil.
---

#### `String` celestial.planetName(`CelestialCoordinate` planet)

Returns the name of the specified planet.
---

#### `uint64_t` celestial.planetSeed(`CelestialCoordinate` planet)

Returns the seed for the specified planet.
---

#### `float` celestial.clusterSize(`CelestialCoordinate` planet)

Returns the diameter of the specified planet and its orbiting moons.
---

#### `List<String>` celestial.planetOres(`CelestialCoordinate` planet)

Returns a list of ores available on the specified planet.
---

#### `Vec2F` celestial.systemPosition(`SystemLocation` location)

Returns the position of the specified location in the *current system*.
---

#### `Vec2F` celestial.orbitPosition(`Orbit` orbit)

Returns the calculated position of the provided orbit.

```
local orbit = {
  target = planet, -- the orbit target
  direction = 1, -- orbit direction
  enterTime = 0, -- time the orbit was entered, universe epoch time
  enterPosition = {1, 0} -- the position that the orbit was entered at, relative to the target
}
```
---

#### `List<Uuid>` celestial.systemObjects()

Returns a list of the Uuids for objects in the current system.
---

#### `String` celestial.objectType(`Uuid` uuid)

Returns the type of the specified object.
---

#### `Json` celestial.objectParameters(`Uuid` uuid)

Returns the parameters for the specified object.
---

#### `WorldId` celestial.objectWarpActionWorld(`Uuid` uuid)

Returns the warp action world ID for the specified object.
---

#### `Json` celestial.objectOrbit(`Uuid` uuid)

Returns the orbit of the specified object, if any.
---

#### `Maybe<Vec2F>` celestial.objectPosition(`Uuid` uuid)

Returns the position of the specified object, if any.
---

#### `Json` celestial.objectTypeConfig(`String` typeName)

Returns the configuration of the specified object type.
---

#### `Uuid` celestial.systemSpawnObject(`String` typeName, [`Vec2F` position], [`Uuid` uuid], [`Json` parameters])

Spawns an object of typeName at position. Optionally with the specified UUID and parameters.

If no position is specified, one is automatically chosen in a spawnable range.

Objects are limited to be spawned outside a distance of  `/systemworld.config:clientSpawnObjectPadding` from any planet surface (including moons), star surface, planetary orbit (including moons), or permanent objects orbits, and at most within `clientSpawnObjectPadding` from the outermost orbit.
---

#### `List<Uuid>` celestial.playerShips()

Returns a list of the player ships in the current system.
---

#### `playerShipPosition` celestial.playerShipPosition(`Uuid` uuid)

Returns the position of the specified player ship.
---

#### `Maybe<bool>` celestial.hasChildren(`CelestialCoordinate` coordinate)

Returns definitively whether the coordinate has orbiting children. `nil` return means the coordinate is not loaded.
---

#### `List<CelestialCoordinate>` celestial.children(`CelestialCoordinate` coordinate)

Returns the children for the specified celestial coordinate. For systems, return planets, for planets, return moons.
---

#### `List<int>` celestial.childOrbits(`CelestialCoordinate` coordinate)

Returns the child orbits for the specified celestial coordinate.
---

#### `List<CelestialCoordinate>` celestial.scanSystems(`RectI` region, [`Set<String>` includedTypes])

Returns a list of systems in the given region. If includedTypes is specified, this will return only systems whose typeName parameter is included in the set. This scans for systems asynchronously, meaning it may not return all systems if they have not been generated or sent to the client. Use `scanRegionFullyLoaded` to see if this is the case.
---

#### `List<pair<Vec2I, Vec2I>>` celestial.scanConstellationLines(`RectI` region)

Returns the constellation lines for the specified universe region.
---

#### `bool` celestial.scanRegionFullyLoaded(`RectI` region)

Returns whether the specified universe region has been fully loaded.
---

#### `List<pair<String, float>>` celestial.centralBodyImages(`CelestialCoordinate` system)

Returns the images with scales for the central body (star) for the specified system coordinate.
---

#### `List<pair<String, float>>` celestial.planetaryObjectImages(`CelestialCoordinate` coordinate)

Returns the smallImages with scales for the specified planet or moon.
---

#### `List<pair<String, float>>` celestial.worldImages(`CelestialCoordinate` coordinate)

Returns the generated world images with scales for the specified planet or moon.
---

#### `List<pair<String, float>>` celestial.starImages(`CelestialCoordinate` system, `float` twinkleTime)

Returns the star image for the specified system. Requires a twinkle time to provide the correct image frame.
