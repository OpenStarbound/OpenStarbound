#include "StarCelestialDatabase.hpp"
#include "StarLexicalCast.hpp"
#include "StarCasting.hpp"
#include "StarRandom.hpp"
#include "StarCompression.hpp"
#include "StarFile.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarVersioningDatabase.hpp"
#include "StarIterator.hpp"

namespace Star {

CelestialDatabase::~CelestialDatabase() {}

RectI CelestialDatabase::xyRange() const {
  auto range = m_baseInformation.xyCoordRange;
  return RectI(range[0], range[0], range[1], range[1]);
}

int CelestialDatabase::planetOrbitalLevels() const {
  return m_baseInformation.planetOrbitalLevels;
}

int CelestialDatabase::satelliteOrbitalLevels() const {
  return m_baseInformation.satelliteOrbitalLevels;
}

Vec2I CelestialDatabase::chunkIndexFor(CelestialCoordinate const& coordinate) const {
  return chunkIndexFor(coordinate.location().vec2());
}

Vec2I CelestialDatabase::chunkIndexFor(Vec2I const& systemXY) const {
  return {(systemXY[0] - pmod(systemXY[0], m_baseInformation.chunkSize)) / m_baseInformation.chunkSize,
      (systemXY[1] - pmod(systemXY[1], m_baseInformation.chunkSize)) / m_baseInformation.chunkSize};
}

List<Vec2I> CelestialDatabase::chunkIndexesFor(RectI const& region) const {
  if (region.isEmpty())
    return {};

  List<Vec2I> chunkLocations;
  RectI chunkRegion(chunkIndexFor(region.min()), chunkIndexFor(region.max() - Vec2I(1, 1)));
  for (int x = chunkRegion.xMin(); x <= chunkRegion.xMax(); ++x) {
    for (int y = chunkRegion.yMin(); y <= chunkRegion.yMax(); ++y)
      chunkLocations.append({x, y});
  }
  return chunkLocations;
}

RectI CelestialDatabase::chunkRegion(Vec2I const& chunkIndex) const {
  return RectI(chunkIndex * m_baseInformation.chunkSize, (chunkIndex + Vec2I(1, 1)) * m_baseInformation.chunkSize);
}

CelestialMasterDatabase::CelestialMasterDatabase(Maybe<String> databaseFile) {
  auto assets = Root::singleton().assets();

  auto config = assets->json("/celestial.config");

  m_baseInformation.planetOrbitalLevels = config.getInt("planetOrbitalLevels");
  m_baseInformation.satelliteOrbitalLevels = config.getInt("satelliteOrbitalLevels");
  m_baseInformation.chunkSize = config.getInt("chunkSize");
  m_baseInformation.xyCoordRange = jsonToVec2I(config.get("xyCoordRange"));
  m_baseInformation.zCoordRange = jsonToVec2I(config.get("zCoordRange"));
  m_baseInformation.enforceCoordRange = config.getBool("enforceCoordRange", false);

  m_generationInformation.systemProbability = config.getFloat("systemProbability");
  m_generationInformation.constellationProbability = config.getFloat("constellationProbability");
  m_generationInformation.constellationLineCountRange = jsonToVec2U(config.get("constellationLineCountRange"));
  m_generationInformation.constellationMaxTries = config.getUInt("constellationMaxTries");
  m_generationInformation.maximumConstellationLineLength = config.getFloat("maximumConstellationLineLength");
  m_generationInformation.minimumConstellationLineLength = config.getFloat("minimumConstellationLineLength");
  m_generationInformation.minimumConstellationMagnitude = config.getFloat("minimumConstellationMagnitude");
  m_generationInformation.minimumConstellationLineCloseness = config.getFloat("minimumConstellationLineCloseness");

  // Copy construct into a Map<String, Json> in the parsing of the weighted
  // pools to make sure that each WeightedPool is predictably populated based
  // on key order.

  for (auto const& systemPair : Map<String, Json>::from(config.getObject("systemTypes"))) {
    SystemType systemType;
    systemType.typeName = systemPair.first;
    systemType.baseParameters = systemPair.second.get("baseParameters");
    systemType.variationParameters = systemPair.second.getArray("variationParameters", JsonArray());
    for (auto const& orbitRegion : systemPair.second.getArray("orbitRegions", JsonArray())) {
      String regionName = orbitRegion.getString("regionName");
      Vec2I orbitRange = jsonToVec2I(orbitRegion.get("orbitRange"));
      float bodyProbability = orbitRegion.getFloat("bodyProbability");
      WeightedPool<String> regionPlanetaryTypes = jsonToWeightedPool<String>(orbitRegion.get("planetaryTypes"));
      WeightedPool<String> regionSatelliteTypes = jsonToWeightedPool<String>(orbitRegion.get("satelliteTypes"));
      systemType.orbitRegions.append({regionName, orbitRange, bodyProbability, regionPlanetaryTypes, regionSatelliteTypes});
    }
    m_generationInformation.systemTypes.add(systemPair.first, systemType);
  }

  m_generationInformation.systemTypePerlin = PerlinD(config.getObject("systemTypePerlin"), staticRandomU64("SystemTypePerlin"));
  m_generationInformation.systemTypeBins = config.get("systemTypeBins");

  for (auto const& planetaryPair : Map<String, Json>::from(config.getObject("planetaryTypes"))) {
    PlanetaryType planetaryType;
    planetaryType.typeName = planetaryPair.first;
    planetaryType.satelliteProbability = planetaryPair.second.getFloat("satelliteProbability");
    planetaryType.maxSatelliteCount =
        planetaryPair.second.getUInt("maxSatelliteCount", m_baseInformation.satelliteOrbitalLevels);
    planetaryType.baseParameters = planetaryPair.second.get("baseParameters");
    planetaryType.variationParameters = planetaryPair.second.getArray("variationParameters", JsonArray());
    planetaryType.orbitParameters = planetaryPair.second.getObject("orbitParameters", JsonObject());
    m_generationInformation.planetaryTypes[planetaryType.typeName] = planetaryType;
  }

  for (auto const& satellitePair : Map<String, Json>::from(config.getObject("satelliteTypes"))) {
    SatelliteType satelliteType;
    satelliteType.typeName = satellitePair.first;
    satelliteType.baseParameters = satellitePair.second.get("baseParameters");
    satelliteType.variationParameters = satellitePair.second.getArray("variationParameters", JsonArray());
    satelliteType.orbitParameters = satellitePair.second.getObject("orbitParameters", JsonObject());
    m_generationInformation.satelliteTypes[satelliteType.typeName] = satelliteType;
  }

  auto namesConfig = assets->json("/celestial/names.config");
  m_generationInformation.planetarySuffixes = jsonToStringList(namesConfig.get("planetarySuffixes"));
  m_generationInformation.satelliteSuffixes = jsonToStringList(namesConfig.get("satelliteSuffixes"));

  for (auto const& list : namesConfig.get("systemPrefixNames").iterateArray())
    m_generationInformation.systemPrefixNames.add(list.getFloat(0), list.getString(1));

  for (auto const& list : namesConfig.get("systemNames").iterateArray())
    m_generationInformation.systemNames.add(list.getFloat(0), list.getString(1));

  for (auto const& list : namesConfig.get("systemSuffixNames").iterateArray())
    m_generationInformation.systemSuffixNames.add(list.getFloat(0), list.getString(1));

  if (databaseFile) {
    m_database.setContentIdentifier("Celestial2");
    m_database.setIODevice(File::open(*databaseFile, IOMode::ReadWrite));
    m_database.open();
    if (m_database.contentIdentifier() != "Celestial2") {
      Logger::error("CelestialMasterDatabase database content identifier is not 'Celestial2', moving out of the way and recreating");
      m_database.close();
      File::rename(*databaseFile, strf("{}.{}.fail", *databaseFile, Time::millisecondsSinceEpoch()));
      m_database.setIODevice(File::open(*databaseFile, IOMode::ReadWrite));
      m_database.open();
    }
    m_database.setAutoCommit(false);
  }

  m_commitInterval = config.getFloat("commitInterval");
  m_commitTimer.restart(m_commitInterval);
}

CelestialBaseInformation CelestialMasterDatabase::baseInformation() const {
  return m_baseInformation;
}

CelestialResponse CelestialMasterDatabase::respondToRequest(CelestialRequest const& request) {
  RecursiveMutexLocker locker(m_mutex);

  if (auto chunkLocation = request.maybeLeft()) {
    auto chunk = getChunk(*chunkLocation);
    // System objects are sent by separate system requests.
    chunk.systemObjects.clear();
    return makeLeft(std::move(chunk));
  } else if (auto systemLocation = request.maybeRight()) {
    auto const& chunk = getChunk(chunkIndexFor(*systemLocation));
    CelestialSystemObjects systemObjects = {*systemLocation, chunk.systemObjects.get(*systemLocation)};
    return makeRight(std::move(systemObjects));
  } else {
    return CelestialResponse();
  }
}

void CelestialMasterDatabase::cleanupAndCommit() {
  RecursiveMutexLocker locker(m_mutex);
  m_chunkCache.cleanup();
  if (m_database.isOpen() && m_commitTimer.timeUp()) {
    m_database.commit();
    m_commitTimer.restart(m_commitInterval);
  }
}

bool CelestialMasterDatabase::coordinateValid(CelestialCoordinate const& coordinate) {
  RecursiveMutexLocker locker(m_mutex);

  if (!coordinate)
    return false;

  auto const& chunk = getChunk(chunkIndexFor(coordinate));

  auto systemObjects = chunk.systemObjects.ptr(coordinate.location());
  if (!systemObjects)
    return false;

  if (coordinate.isSystem())
    return true;

  auto planet = systemObjects->ptr(coordinate.planet().orbitNumber());
  if (!planet)
    return false;

  if (coordinate.isPlanetaryBody())
    return true;

  return planet->satelliteParameters.contains(coordinate.orbitNumber());
}

Maybe<CelestialCoordinate> CelestialMasterDatabase::findRandomWorld(unsigned tries, unsigned trySpatialRange,
    function<bool(CelestialCoordinate)> filter, Maybe<uint64_t> seed) {
  RandomSource randSource;
  if (seed)
    randSource.init(*seed);
  for (unsigned i = 0; i < tries; ++i) {
    RectI range = xyRange();
    Vec2I randomLocation = Vec2I(randSource.randInt(range.xMin(), range.xMax()), randSource.randInt(range.yMin(), range.yMax()));
    for (auto& system : scanSystems(RectI::withCenter(randomLocation, Vec2I::filled(trySpatialRange)))) {
      if (!hasChildren(system).value(false))
        continue;

      auto world = randSource.randFrom(children(system));
      // This sucks, 50% of the time will try and return satellite, not really
      // balanced probability wise
      if (hasChildren(world).value(false) && randSource.randb())
        world = randSource.randFrom(children(world));

      if (!filter || filter(world))
        return world;
    }
  }

  return {};
}

Maybe<CelestialParameters> CelestialMasterDatabase::parameters(CelestialCoordinate const& coordinate) {
  RecursiveMutexLocker locker(m_mutex);

  if (!coordinateValid(coordinate))
    throw CelestialException("CelestialMasterDatabase::parameters called on invalid coordinate");

  auto const& chunk = getChunk(chunkIndexFor(coordinate));

  if (coordinate.isSatelliteBody())
    return chunk.systemObjects.get(coordinate.location())
        .get(coordinate.parent().orbitNumber())
        .satelliteParameters.get(coordinate.orbitNumber());

  if (coordinate.isPlanetaryBody())
    return chunk.systemObjects.get(coordinate.location()).get(coordinate.orbitNumber()).planetParameters;

  return chunk.systemParameters.get(coordinate.location());
}

Maybe<String> CelestialMasterDatabase::name(CelestialCoordinate const& coordinate) {
  return parameters(coordinate)->name();
}

Maybe<bool> CelestialMasterDatabase::hasChildren(CelestialCoordinate const& coordinate) {
  RecursiveMutexLocker locker(m_mutex);

  if (!coordinateValid(coordinate))
    throw CelestialException("CelestialMasterDatabase::hasChildren called on invalid coordinate");

  auto const& systemObjects = getChunk(chunkIndexFor(coordinate)).systemObjects.get(coordinate.location());

  if (coordinate.isSystem())
    return !systemObjects.empty();

  if (coordinate.isPlanetaryBody())
    return !systemObjects.get(coordinate.orbitNumber()).satelliteParameters.empty();

  return false;
}

List<CelestialCoordinate> CelestialMasterDatabase::children(CelestialCoordinate const& coordinate) {
  return childOrbits(coordinate).transformed(bind(&CelestialCoordinate::child, coordinate, _1));
}

List<int> CelestialMasterDatabase::childOrbits(CelestialCoordinate const& coordinate) {
  RecursiveMutexLocker locker(m_mutex);

  if (!coordinateValid(coordinate))
    throw CelestialException("CelestialMasterDatabase::childOrbits called on invalid coordinate");

  auto const& systemObjects = getChunk(chunkIndexFor(coordinate)).systemObjects.get(coordinate.location());

  if (coordinate.isSystem())
    return systemObjects.keys();

  if (coordinate.isPlanetaryBody())
    return systemObjects.get(coordinate.orbitNumber()).satelliteParameters.keys();

  throw CelestialException("CelestialMasterDatabase::childOrbits called on improper type!");
}

List<CelestialCoordinate> CelestialMasterDatabase::scanSystems(RectI const& region, Maybe<StringSet> const& includedTypes) {
  RecursiveMutexLocker locker(m_mutex);

  List<CelestialCoordinate> systems;
  for (auto const& chunkLocation : chunkIndexesFor(region)) {
    auto const& chunkData = getChunk(chunkLocation, [&](std::function<void()>&& func) {
      locker.unlock();
      func();
      locker.lock();
    });
    locker.unlock();
    for (auto const& pair : chunkData.systemParameters) {
      Vec3I systemLocation = pair.first;
      if (region.contains(systemLocation.vec2())) {
        if (includedTypes) {
          String thisType = pair.second.getParameter("typeName", "").toString();
          if (!includedTypes->contains(thisType))
            continue;
        }
        systems.append(CelestialCoordinate(systemLocation));
      }
    }
    locker.lock();
  }
  return systems;
}

List<pair<Vec2I, Vec2I>> CelestialMasterDatabase::scanConstellationLines(RectI const& region) {
  RecursiveMutexLocker locker(m_mutex);

  List<pair<Vec2I, Vec2I>> lines;
  for (auto const& chunkLocation : chunkIndexesFor(region)) {
    auto const& chunkData = getChunk(chunkLocation);
    for (auto const& constellation : chunkData.constellations) {
      for (auto const& line : constellation) {
        if (region.intersects(Line2I(line.first, line.second)))
          lines.append(line);
      }
    }
  }
  return lines;
}

bool CelestialMasterDatabase::scanRegionFullyLoaded(RectI const&) {
  return true;
}

void CelestialMasterDatabase::updateParameters(CelestialCoordinate const& coordinate, CelestialParameters const& parameters) {
  RecursiveMutexLocker locker(m_mutex);

  if (!coordinateValid(coordinate))
    throw CelestialException("CelestialMasterDatabase::updateParameters called on invalid coordinate");

  auto chunkIndex = chunkIndexFor(coordinate);
  auto chunk = getChunk(chunkIndex);

  bool updated = false;
  if (coordinate.isSatelliteBody()) {
    chunk.systemObjects.get(coordinate.location())
        .get(coordinate.parent().orbitNumber())
        .satelliteParameters.set(coordinate.orbitNumber(), parameters);
    updated = true;
  } else if (coordinate.isPlanetaryBody()) {
    chunk.systemObjects.get(coordinate.location()).get(coordinate.orbitNumber()).planetParameters = parameters;
    updated = true;
  }

  if (updated && m_database.isOpen()) {
    auto versioningDatabase = Root::singleton().versioningDatabase();
    auto versionedChunk = versioningDatabase->makeCurrentVersionedJson("CelestialChunk", chunk.toJson());
    m_database.insert(DataStreamBuffer::serialize(chunkIndex), compressData(DataStreamBuffer::serialize<VersionedJson>(versionedChunk)));

    m_chunkCache.remove(chunkIndex);
  } else {
    updated = false;
  }

  if (!updated)
    throw CelestialException("CelestialMasterDatabase::updateParameters failed; coordinate is not a valid planet or satellite, or celestial database was not open for writing");
}

Maybe<CelestialOrbitRegion> CelestialMasterDatabase::orbitRegion(
    List<CelestialOrbitRegion> const& orbitRegions, int planetaryOrbitNumber) {
  for (auto const& region : orbitRegions) {
    if (planetaryOrbitNumber >= region.orbitRange[0] && planetaryOrbitNumber <= region.orbitRange[1])
      return region;
  }
  return {};
}

CelestialChunk const& CelestialMasterDatabase::getChunk(Vec2I const& chunkIndex, UnlockDuringFunction unlockDuring) {
  return m_chunkCache.get(chunkIndex, [&](Vec2I const& chunkIndex) -> CelestialChunk {
      auto versioningDatabase = Root::singleton().versioningDatabase();

      if (m_database.isOpen()) {
        if (auto chunkData = m_database.find(DataStreamBuffer::serialize(chunkIndex))) {
          auto versionedChunk = DataStreamBuffer::deserialize<VersionedJson>(uncompressData(chunkData.take()));
          if (!versioningDatabase->versionedJsonCurrent(versionedChunk)) {
            versionedChunk = versioningDatabase->updateVersionedJson(versionedChunk);
            m_database.insert(DataStreamBuffer::serialize(chunkIndex),
                compressData(DataStreamBuffer::serialize<VersionedJson>(versionedChunk)));
          }
          return CelestialChunk(versionedChunk.content);
        }
      }

      CelestialChunk newChunk;
      auto producer = [&]() { newChunk = produceChunk(chunkIndex); };
      if (unlockDuring)
        unlockDuring(producer);
      else
        producer();
      if (m_database.isOpen()) {
        auto versionedChunk = versioningDatabase->makeCurrentVersionedJson("CelestialChunk", newChunk.toJson());
        m_database.insert(DataStreamBuffer::serialize(chunkIndex),
            compressData(DataStreamBuffer::serialize<VersionedJson>(versionedChunk)));
      }

      return newChunk;
    });
}

CelestialChunk CelestialMasterDatabase::produceChunk(Vec2I const& chunkIndex) const {
  CelestialChunk chunkData;
  chunkData.chunkIndex = chunkIndex;
  RectI region = chunkRegion(chunkIndex);
  if (m_baseInformation.enforceCoordRange && !xyRange().contains(region, true))
    return chunkData;

  RandomSource random(staticRandomU64(chunkIndex[0], chunkIndex[1], "ChunkIndexMix"));

  List<Vec3I> systemLocations;
  for (int x = region.xMin(); x < region.xMax(); ++x) {
    for (int y = region.yMin(); y < region.yMax(); ++y) {
      if (random.randf() < m_generationInformation.systemProbability) {
        auto z = random.randi32() % (m_baseInformation.zCoordRange[1] - m_baseInformation.zCoordRange[0])
            + m_baseInformation.zCoordRange[0];
        systemLocations.append(Vec3I(x, y, z));
      }
    }
  }

  List<Vec2I> constellationCandidates;
  for (auto const& systemLocation : systemLocations) {
    if (auto systemInformation = produceSystem(random, systemLocation)) {
      chunkData.systemParameters[systemLocation] = systemInformation.get().first;
      chunkData.systemObjects[systemLocation] = std::move(systemInformation.get().second);

      if (systemInformation.get().first.getParameter("magnitude").toFloat()
          >= m_generationInformation.minimumConstellationMagnitude)
        constellationCandidates.append(systemLocation.vec2());
    }
  }

  chunkData.constellations = produceConstellations(random, constellationCandidates);

  return chunkData;
}

Maybe<pair<CelestialParameters, HashMap<int, CelestialPlanet>>> CelestialMasterDatabase::produceSystem(
    RandomSource& random, Vec3I const& location) const {
  float typeSelector = m_generationInformation.systemTypePerlin.get(location[0], location[1]);
  String systemTypeName = binnedChoiceFromJson(m_generationInformation.systemTypeBins, typeSelector, "").toString();
  if (systemTypeName.empty())
    return {};
  auto systemType = m_generationInformation.systemTypes.get(systemTypeName);

  CelestialCoordinate systemCoordinate(location);
  uint64_t systemSeed = random.randu64();

  String prefix = m_generationInformation.systemPrefixNames.select(random);
  String mid = m_generationInformation.systemNames.select(random);
  String suffix = m_generationInformation.systemSuffixNames.select(random);

  String systemName = String(strf("{} {} {}", prefix, mid, suffix)).trim();

  systemName = systemName.replace("<onedigit>", strf("{:01d}", random.randu32() % 10));
  systemName = systemName.replace("<twodigit>", strf("{:02d}", random.randu32() % 100));
  systemName = systemName.replace("<threedigit>", strf("{:03d}", random.randu32() % 1000));
  systemName = systemName.replace("<fourdigit>", strf("{:04d}", random.randu32() % 10000));

  CelestialParameters systemParameters = CelestialParameters(systemCoordinate,
      systemSeed,
      systemName,
      jsonMerge(systemType.baseParameters, random.randValueFrom(systemType.variationParameters)));

  List<int> planetaryOrbits;
  for (int i = 1; i <= m_baseInformation.planetOrbitalLevels; ++i) {
    if (auto systemOrbitRegion = orbitRegion(systemType.orbitRegions, i)) {
      if (random.randf() <= systemOrbitRegion->bodyProbability)
        planetaryOrbits.append(i);
    }
  }

  HashMap<int, CelestialPlanet> systemObjects;
  for (auto planetPair : enumerateIterator(planetaryOrbits)) {
    auto systemOrbitRegion = orbitRegion(systemType.orbitRegions, planetPair.first);

    auto planetaryTypeName = systemOrbitRegion->planetaryTypes.select(random);
    if (m_generationInformation.planetaryTypes.contains(planetaryTypeName)) {
      auto planetaryType = m_generationInformation.planetaryTypes.get(planetaryTypeName);
      auto planetaryParameters =
          jsonMerge(planetaryType.baseParameters, random.randValueFrom(planetaryType.variationParameters));

      CelestialCoordinate planetCoordinate(location, planetPair.first);
      uint64_t planetarySeed = random.randu64();
      String planetaryName = strf("{} {}", systemName, m_generationInformation.planetarySuffixes.at(planetPair.second));

      CelestialPlanet planet;
      planet.planetParameters =
          CelestialParameters(planetCoordinate, planetarySeed, planetaryName, planetaryParameters);

      List<int> satelliteOrbits;
      for (int i = 1; i <= m_baseInformation.satelliteOrbitalLevels; ++i) {
        if (satelliteOrbits.size() < planetaryType.maxSatelliteCount
            && random.randf() < planetaryType.satelliteProbability)
          satelliteOrbits.append(i);
      }

      for (auto satellitePair : enumerateIterator(satelliteOrbits)) {
        auto satelliteTypeName = systemOrbitRegion->satelliteTypes.select(random);
        if (m_generationInformation.satelliteTypes.contains(satelliteTypeName)) {
          auto satelliteType = m_generationInformation.satelliteTypes.get(satelliteTypeName);
          auto satelliteParameters = jsonMerge(satelliteType.baseParameters,
              random.randValueFrom(satelliteType.variationParameters),
              random.randValueFrom(
                  satelliteType.orbitParameters.value(systemOrbitRegion->regionName, JsonArray()).toArray()));

          CelestialCoordinate satelliteCoordinate(location, planetPair.first, satellitePair.first);
          uint64_t satelliteSeed = random.randu64();
          String satelliteName =
              strf("{} {}", planetaryName, m_generationInformation.satelliteSuffixes.at(satellitePair.second));

          planet.satelliteParameters[satellitePair.first] =
              CelestialParameters(satelliteCoordinate, satelliteSeed, satelliteName, satelliteParameters);
        }
      }

      systemObjects[planetPair.first] = std::move(planet);
    }
  }

  return pair<CelestialParameters, HashMap<int, CelestialPlanet>>{std::move(systemParameters), std::move(systemObjects)};
}

List<CelestialConstellation> CelestialMasterDatabase::produceConstellations(
    RandomSource& random, List<Vec2I> const& constellationCandidates) const {
  List<CelestialConstellation> constellations;

  if (random.randf() < m_generationInformation.constellationProbability && constellationCandidates.size() > 2) {
    unsigned targetConstellationLineCount = random.randUInt(
        m_generationInformation.constellationLineCountRange[0], m_generationInformation.constellationLineCountRange[1]);
    Set<Vec2I> constellationPoints;
    Set<Line2I> constellationLines;

    unsigned tries = 0;

    while (constellationLines.size() < targetConstellationLineCount) {
      if (++tries > m_generationInformation.constellationMaxTries)
        break;

      Vec2I start;
      if (constellationPoints.empty())
        start = random.randValueFrom(constellationCandidates);
      else
        start = random.randValueFrom(constellationPoints);

      Vec2I end = random.randValueFrom(constellationCandidates);

      Line2I proposedLine(start, end);
      Line2D proposedLineD(proposedLine);

      if (start == end)
        continue;

      if (constellationLines.contains(proposedLine) || constellationLines.contains(proposedLine.reversed()))
        continue;

      if (proposedLineD.diff().magnitude() > m_generationInformation.maximumConstellationLineLength)
        continue;

      if (proposedLineD.diff().magnitude() < m_generationInformation.minimumConstellationLineLength)
        continue;

      bool valid = true;
      for (auto const& constellationLine : constellationLines) {
        Line2D constellationLineD(constellationLine);
        auto intersection = proposedLineD.intersection(constellationLineD);
        if (intersection.intersects && Vec2I::round(intersection.point) != proposedLine.min()
            && Vec2I::round(intersection.point) != proposedLine.max()) {
          valid = false;
          break;
        }

        if (proposedLine.min() != constellationLine.min() && proposedLine.min() != constellationLine.max()
            && constellationLineD.distanceTo(proposedLineD.min())
                < m_generationInformation.minimumConstellationLineCloseness) {
          valid = false;
          break;
        }

        if (proposedLine.max() != constellationLine.min() && proposedLine.max() != constellationLine.max()
            && constellationLineD.distanceTo(proposedLineD.max())
                < m_generationInformation.minimumConstellationLineCloseness) {
          valid = false;
          break;
        }
      }

      if (valid) {
        constellationLines.add(proposedLine);
        constellationPoints.add(proposedLine.min());
        constellationPoints.add(proposedLine.max());
      }
    }

    if (constellationLines.size() > 1) {
      CelestialConstellation constellation;
      for (auto const& line : constellationLines)
        constellation.append({line.min(), line.max()});
      constellations.append(constellation);
    }
  }

  return constellations;
}

CelestialSlaveDatabase::CelestialSlaveDatabase(CelestialBaseInformation baseInformation) {
  auto config = Root::singleton().assets()->json("/celestial.config");

  m_baseInformation = std::move(baseInformation);
  m_requestTimeout = config.getFloat("requestTimeout");
}

void CelestialSlaveDatabase::signalRegion(RectI const& region) {
  RecursiveMutexLocker locker(m_mutex);

  for (auto location : chunkIndexesFor(region)) {
    if (!m_chunkCache.ptr(location) && !m_pendingChunkRequests.contains(location))
      m_pendingChunkRequests[location] = Timer();
  }
}

void CelestialSlaveDatabase::signalSystem(CelestialCoordinate const& system) {
  RecursiveMutexLocker locker(m_mutex);

  if (auto chunk = m_chunkCache.ptr(chunkIndexFor(system))) {
    if (!chunk->systemObjects.contains(system.location()))
      m_pendingSystemRequests[system.location()] = Timer();
  } else {
    signalRegion(RectI::withSize(system.location().vec2(), {1, 1}));
  }
}

List<CelestialRequest> CelestialSlaveDatabase::pullRequests() {
  RecursiveMutexLocker locker(m_mutex);

  List<CelestialRequest> requests;

  auto chunkIt = makeSMutableMapIterator(m_pendingChunkRequests);
  while (chunkIt.hasNext()) {
    auto& pair = chunkIt.next();
    if (!pair.second.running()) {
      requests.append(makeLeft(pair.first));
      pair.second.restart(m_requestTimeout);
    } else if (pair.second.timeUp()) {
      chunkIt.remove();
    }
  }

  auto systemIt = makeSMutableMapIterator(m_pendingSystemRequests);
  while (systemIt.hasNext()) {
    auto& pair = systemIt.next();
    if (!pair.second.running()) {
      requests.append(makeRight(pair.first));
      pair.second.restart(m_requestTimeout);
    } else if (pair.second.timeUp()) {
      systemIt.remove();
    }
  }

  return requests;
}

void CelestialSlaveDatabase::pushResponses(List<CelestialResponse> responses) {
  RecursiveMutexLocker locker(m_mutex);

  for (auto& response : responses) {
    if (auto celestialChunk = response.leftPtr()) {
      m_pendingChunkRequests.remove(celestialChunk->chunkIndex);
      m_chunkCache.set(celestialChunk->chunkIndex, std::move(*celestialChunk));
    } else if (auto celestialSystemObjects = response.rightPtr()) {
      m_pendingSystemRequests.remove(celestialSystemObjects->systemLocation);
      auto chunkLocation = chunkIndexFor(celestialSystemObjects->systemLocation);
      if (auto chunk = m_chunkCache.ptr(chunkLocation))
        chunk->systemObjects[celestialSystemObjects->systemLocation] = std::move(celestialSystemObjects->planets);
    }
  }
}

void CelestialSlaveDatabase::cleanup() {
  RecursiveMutexLocker locker(m_mutex);
  m_chunkCache.cleanup();
}

Maybe<CelestialParameters> CelestialSlaveDatabase::parameters(CelestialCoordinate const& coordinate) {
  if (!coordinate)
    throw CelestialException("CelestialSlaveDatabase::parameters called on null coordinate");

  RecursiveMutexLocker locker(m_mutex);

  if (coordinate.isSystem())
    signalRegion(RectI::withSize(coordinate.location().vec2(), {1, 1}));
  else
    signalSystem(coordinate);

  if (auto chunk = m_chunkCache.ptr(chunkIndexFor(coordinate))) {
    if (coordinate.isSystem())
      return chunk->systemParameters.maybe(coordinate.location());

    if (auto systemObjects = chunk->systemObjects.ptr(coordinate.location())) {
      auto const& planet = systemObjects->get(coordinate.planet().orbitNumber());
      if (coordinate.isPlanetaryBody())
        return planet.planetParameters;
      else if (coordinate.isSatelliteBody())
        return planet.satelliteParameters.get(coordinate.orbitNumber());
    }
  }

  return {};
}

Maybe<String> CelestialSlaveDatabase::name(CelestialCoordinate const& coordinate) {
  if (auto p = parameters(coordinate))
    return p->name();
  return {};
}

Maybe<bool> CelestialSlaveDatabase::hasChildren(CelestialCoordinate const& coordinate) {
  if (!coordinate)
    throw CelestialException("CelestialSlaveDatabase::hasChildren called on null coordinate");

  RecursiveMutexLocker locker(m_mutex);

  signalSystem(coordinate);

  if (auto chunk = m_chunkCache.ptr(chunkIndexFor(coordinate))) {
    if (auto systemObjects = chunk->systemObjects.ptr(coordinate.location())) {
      if (coordinate.isSystem())
        return !systemObjects->empty();
      else if (coordinate.isPlanetaryBody())
        return !systemObjects->get(coordinate.orbitNumber()).satelliteParameters.empty();
    }
  }

  return {};
}

List<CelestialCoordinate> CelestialSlaveDatabase::children(CelestialCoordinate const& coordinate) {
  return childOrbits(coordinate).transformed(bind(&CelestialCoordinate::child, coordinate, _1));
}

List<int> CelestialSlaveDatabase::childOrbits(CelestialCoordinate const& coordinate) {
  if (!coordinate)
    throw CelestialException("CelestialSlaveDatabase::childOrbits called on null coordinate");

  if (coordinate.isSatelliteBody())
    throw CelestialException("CelestialSlaveDatabase::childOrbits called on improper type!");

  RecursiveMutexLocker locker(m_mutex);

  signalSystem(coordinate);

  if (auto chunk = m_chunkCache.ptr(chunkIndexFor(coordinate))) {
    if (auto systemObjects = chunk->systemObjects.ptr(coordinate.location())) {
      if (coordinate.isSystem())
        return systemObjects->keys();
      else if (coordinate.isPlanetaryBody())
        return systemObjects->get(coordinate.orbitNumber()).satelliteParameters.keys();
    }
  }

  return {};
}

List<CelestialCoordinate> CelestialSlaveDatabase::scanSystems(RectI const& region, Maybe<StringSet> const& includedTypes) {
  RecursiveMutexLocker locker(m_mutex);

  signalRegion(region);

  List<CelestialCoordinate> systems;
  for (auto const& chunkLocation : chunkIndexesFor(region)) {
    if (auto chunkData = m_chunkCache.ptr(chunkLocation)) {
      for (auto const& pair : chunkData->systemParameters) {
        Vec3I systemLocation = pair.first;
        if (region.contains(systemLocation.vec2())) {
          if (includedTypes) {
            String thisType = pair.second.getParameter("typeName", "").toString();
            if (!includedTypes->contains(thisType))
              continue;
          }
          systems.append(CelestialCoordinate(systemLocation));
        }
      }
    }
  }
  return systems;
}

List<pair<Vec2I, Vec2I>> CelestialSlaveDatabase::scanConstellationLines(RectI const& region) {
  RecursiveMutexLocker locker(m_mutex);

  signalRegion(region);

  List<pair<Vec2I, Vec2I>> lines;
  for (auto const& chunkLocation : chunkIndexesFor(region)) {
    if (auto chunkData = m_chunkCache.ptr(chunkLocation)) {
      for (auto const& constellation : chunkData->constellations) {
        for (auto const& line : constellation) {
          if (region.intersects(Line2I(line.first, line.second)))
            lines.append(line);
        }
      }
    }
  }
  return lines;
}

bool CelestialSlaveDatabase::scanRegionFullyLoaded(RectI const& region) {
  RecursiveMutexLocker locker(m_mutex);

  signalRegion(region);

  for (auto const& chunkLocation : chunkIndexesFor(region)) {
    if (!m_chunkCache.ptr(chunkLocation))
      return false;
  }
  return true;
}

void CelestialSlaveDatabase::invalidateCacheFor(CelestialCoordinate const& coordinate) {
  RecursiveMutexLocker locker(m_mutex);

  m_chunkCache.remove(chunkIndexFor(coordinate));
}

}
