---@meta

--- celestial API
---@class celestial
celestial = {}

--- Returns whether the client sky is currently flying. ---
---@return boolean
function celestial.skyFlying() end

--- Returns the type of flying the client sky is currently performing. ---
---@return string
function celestial.skyFlyingType() end

--- Returns the current warp phase of the client sky, if warping. ---
---@return string
function celestial.skyWarpPhase() end

--- Returns a value between 0 and 1 for how far through warping the sky is currently. ---
---@return number
function celestial.skyWarpProgress() end

--- Returns whether the sky is currently under hyperspace flight. ---
---@return boolean
function celestial.skyInHyperspace() end

--- Flies the player ship to the specified SystemLocation in the specified system. SystemLocation is either of the following types: Null, CelestialCoordinate, Object, Vec2F The locations are specified as a pair of type and value ``` local system = celestial.currentSystem().location local location = nil -- Null location = {"coordinate", {location = system, planet = 1, satellite = 0}} -- CelestialCoordinate location = {"object", "11112222333344445555666677778888"} -- Object (UUID) location = {0.0, 0.0} -- Vec2F (position in space) celestial.flyShip(system, location) ``` ---
---@param system Vec3I
---@param destination SystemLocation
---@return void
function celestial.flyShip(system, destination) end

--- Returns whether the player ship is flying ---
---@return boolean
function celestial.flying() end

--- Returns the current position of the ship in the system. ---
---@return Vec2F
function celestial.shipSystemPosition() end

--- Returns the current destination of the player ship. ---
---@return SystemLocation
function celestial.shipDestination() end

--- Returns the current system location of the player ship. ---
---@return SystemLocation
function celestial.shipLocation() end

--- Returns the CelestialCoordinate for system the ship is currently in. ---
---@return CelestialCoordinate
function celestial.currentSystem() end

--- Returns the diameter of the specified planet in system space. ---
---@param planet CelestialCoordinate
---@return number
function celestial.planetSize(planet) end

--- Returns the position of the specified planet in system space. ---
---@param planet CelestialCoordinate
---@return Vec2F
function celestial.planetPosition(planet) end

--- Returns the celestial parameters for the specified planet. ---
---@param planet CelestialCoordinate
---@return CelestialParameters
function celestial.planetParameters(planet) end

--- Returns the visitable parameters for the specified visitable planet. For unvisitable planets, returns nil. ---
---@param planet CelestialCoordinate
---@return VisitableParameters
function celestial.visitableParameters(planet) end

--- Returns the name of the specified planet. ---
---@param planet CelestialCoordinate
---@return string
function celestial.planetName(planet) end

--- Returns the seed for the specified planet. ---
---@param planet CelestialCoordinate
---@return uint64_t
function celestial.planetSeed(planet) end

--- Returns the diameter of the specified planet and its orbiting moons. ---
---@param planet CelestialCoordinate
---@return number
function celestial.clusterSize(planet) end

--- Returns a list of ores available on the specified planet. ---
---@param planet CelestialCoordinate
---@return List<String>
function celestial.planetOres(planet) end

--- Returns the position of the specified location in the *current system*. ---
---@param location SystemLocation
---@return Vec2F
function celestial.systemPosition(location) end

--- Returns the calculated position of the provided orbit. ``` local orbit = { target = planet, -- the orbit target direction = 1, -- orbit direction enterTime = 0, -- time the orbit was entered, universe epoch time enterPosition = {1, 0} -- the position that the orbit was entered at, relative to the target } ``` ---
---@param orbit Orbit
---@return Vec2F
function celestial.orbitPosition(orbit) end

--- Returns a list of the Uuids for objects in the current system. ---
---@return List<Uuid>
function celestial.systemObjects() end

--- Returns the type of the specified object. ---
---@param uuid Uuid
---@return string
function celestial.objectType(uuid) end

--- Returns the parameters for the specified object. ---
---@param uuid Uuid
---@return Json
function celestial.objectParameters(uuid) end

--- Returns the warp action world ID for the specified object. ---
---@param uuid Uuid
---@return WorldId
function celestial.objectWarpActionWorld(uuid) end

--- Returns the orbit of the specified object, if any. ---
---@param uuid Uuid
---@return Json
function celestial.objectOrbit(uuid) end

--- Returns the position of the specified object, if any. ---
---@param uuid Uuid
---@return Maybe<Vec2F>
function celestial.objectPosition(uuid) end

--- Returns the configuration of the specified object type. ---
---@param typeName string
---@return Json
function celestial.objectTypeConfig(typeName) end

--- Spawns an object of typeName at position. Optionally with the specified UUID and parameters. If no position is specified, one is automatically chosen in a spawnable range. Objects are limited to be spawned outside a distance of  `/systemworld.config:clientSpawnObjectPadding` from any planet surface (including moons), star surface, planetary orbit (including moons), or permanent objects orbits, and at most within `clientSpawnObjectPadding` from the outermost orbit. ---
---@param typeName string
---@param position Vec2F
---@param uuid Uuid
---@param parameters Json
---@return Uuid
function celestial.systemSpawnObject(typeName, position, uuid, parameters) end

--- Returns a list of the player ships in the current system. ---
---@return List<Uuid>
function celestial.playerShips() end

--- Returns the position of the specified player ship. ---
---@param uuid Uuid
---@return playerShipPosition
function celestial.playerShipPosition(uuid) end

--- Returns definitively whether the coordinate has orbiting children. `nil` return means the coordinate is not loaded. ---
---@param coordinate CelestialCoordinate
---@return Maybe<bool>
function celestial.hasChildren(coordinate) end

--- Returns the children for the specified celestial coordinate. For systems, return planets, for planets, return moons. ---
---@param coordinate CelestialCoordinate
---@return List<CelestialCoordinate>
function celestial.children(coordinate) end

--- Returns the child orbits for the specified celestial coordinate. ---
---@param coordinate CelestialCoordinate
---@return List<int>
function celestial.childOrbits(coordinate) end

--- Returns a list of systems in the given region. If includedTypes is specified, this will return only systems whose typeName parameter is included in the set. This scans for systems asynchronously, meaning it may not return all systems if they have not been generated or sent to the client. Use `scanRegionFullyLoaded` to see if this is the case. ---
---@param region RectI
---@param includedTypes Set<String>
---@return List<CelestialCoordinate>
function celestial.scanSystems(region, includedTypes) end

--- Returns the constellation lines for the specified universe region. ---
---@param region RectI
---@return List<pair<Vec2I, Vec2I>>
function celestial.scanConstellationLines(region) end

--- Returns whether the specified universe region has been fully loaded. ---
---@param region RectI
---@return boolean
function celestial.scanRegionFullyLoaded(region) end

--- Returns the images with scales for the central body (star) for the specified system coordinate. ---
---@param system CelestialCoordinate
---@return List<pair<String, float>>
function celestial.centralBodyImages(system) end

--- Returns the smallImages with scales for the specified planet or moon. ---
---@param coordinate CelestialCoordinate
---@return List<pair<String, float>>
function celestial.planetaryObjectImages(coordinate) end

--- Returns the generated world images with scales for the specified planet or moon. ---
---@param coordinate CelestialCoordinate
---@return List<pair<String, float>>
function celestial.worldImages(coordinate) end

--- Returns the star image for the specified system. Requires a twinkle time to provide the correct image frame.
---@param system CelestialCoordinate
---@param twinkleTime number
---@return List<pair<String, float>>
function celestial.starImages(system, twinkleTime) end
