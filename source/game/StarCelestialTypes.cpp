#include "StarCelestialTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

DataStream& operator>>(DataStream& ds, CelestialPlanet& planet) {
  planet.planetParameters = CelestialParameters(ds.read<ByteArray>());
  ds.readMapContainer(planet.satelliteParameters,
      [](DataStream& ds, int& orbit, CelestialParameters& parameters) {
        ds.read(orbit);
        parameters = CelestialParameters(ds.read<ByteArray>());
      });

  return ds;
}

DataStream& operator<<(DataStream& ds, CelestialPlanet const& planet) {
  ds.write(planet.planetParameters.netStore());
  ds.writeMapContainer(planet.satelliteParameters,
      [](DataStream& ds, int orbit, CelestialParameters const& parameters) {
        ds.write(orbit);
        ds.write(parameters.netStore());
      });

  return ds;
}

DataStream& operator>>(DataStream& ds, CelestialSystemObjects& systemObjects) {
  ds.read(systemObjects.systemLocation);
  ds.read(systemObjects.planets);

  return ds;
}

DataStream& operator<<(DataStream& ds, CelestialSystemObjects const& systemObjects) {
  ds.write(systemObjects.systemLocation);
  ds.write(systemObjects.planets);

  return ds;
}

CelestialChunk::CelestialChunk() {}

CelestialChunk::CelestialChunk(Json const& store) {
  chunkIndex = jsonToVec2I(store.get("chunkIndex"));

  for (auto const& lines : store.getArray("constellations")) {
    CelestialConstellation constellation;
    for (auto const& l : lines.toArray())
      constellation.append({jsonToVec2I(l.get(0)), jsonToVec2I(l.get(1))});
    constellations.append(std::move(constellation));
  }

  for (auto const& p : store.getArray("systemParameters"))
    systemParameters[jsonToVec3I(p.get(0))] = CelestialParameters(p.get(1));

  for (auto const& p : store.getArray("systemObjects")) {
    HashMap<int, CelestialPlanet> celestialSystemObjects;
    for (auto const& planetPair : p.getArray(1)) {
      CelestialPlanet planet;
      planet.planetParameters = CelestialParameters(planetPair.get(1).get("parameters"));
      for (auto const& satellitePair : planetPair.get(1).getArray("satellites"))
        planet.satelliteParameters.add(satellitePair.getInt(0), CelestialParameters(satellitePair.get(1)));
      celestialSystemObjects.add(planetPair.getInt(0), std::move(planet));
    }

    systemObjects[jsonToVec3I(p.get(0))] = std::move(celestialSystemObjects);
  }
}

Json CelestialChunk::toJson() const {
  JsonArray constellationStore;
  for (auto const& constellation : constellations) {
    JsonArray lines;
    for (auto const& p : constellation)
      lines.append(JsonArray{jsonFromVec2I(p.first), jsonFromVec2I(p.second)});
    constellationStore.append(lines);
  }

  JsonArray systemParametersStore;
  for (auto const& p : systemParameters)
    systemParametersStore.append(JsonArray{jsonFromVec3I(p.first), p.second.diskStore()});

  JsonArray systemObjectsStore;
  for (auto const& systemObjectPair : systemObjects) {
    JsonArray planetsStore;
    for (auto const& planetPair : systemObjectPair.second) {
      JsonArray satellitesStore;
      for (auto const& satellitePair : planetPair.second.satelliteParameters)
        satellitesStore.append(JsonArray{satellitePair.first, satellitePair.second.diskStore()});

      planetsStore.append(JsonArray{planetPair.first,
          JsonObject{
              {"parameters", planetPair.second.planetParameters.diskStore()}, {"satellites", std::move(satellitesStore)}}});
    }
    systemObjectsStore.append(JsonArray{jsonFromVec3I(systemObjectPair.first), std::move(planetsStore)});
  }

  return JsonObject{{"chunkIndex", jsonFromVec2I(chunkIndex)},
      {"constellations", constellationStore},
      {"systemParameters", systemParametersStore},
      {"systemObjects", systemObjectsStore}};
}

DataStream& operator>>(DataStream& ds, CelestialChunk& chunk) {
  ds.read(chunk.chunkIndex);
  ds.read(chunk.constellations);
  ds.readMapContainer(chunk.systemParameters,
      [](DataStream& ds, Vec3I& location, CelestialParameters& parameters) {
        ds.read(location);
        parameters = CelestialParameters(ds.read<ByteArray>());
      });
  ds.read(chunk.systemObjects);

  return ds;
}

DataStream& operator<<(DataStream& ds, CelestialChunk const& chunk) {
  ds.write(chunk.chunkIndex);
  ds.write(chunk.constellations);
  ds.writeMapContainer(chunk.systemParameters,
      [](DataStream& ds, Vec3I const& location, CelestialParameters const& parameters) {
        ds.write(location);
        ds.write(parameters.netStore());
      });
  ds.write(chunk.systemObjects);

  return ds;
}

DataStream& operator>>(DataStream& ds, CelestialBaseInformation& celestialInformation) {
  ds.read(celestialInformation.planetOrbitalLevels);
  ds.read(celestialInformation.satelliteOrbitalLevels);
  ds.read(celestialInformation.chunkSize);
  ds.read(celestialInformation.xyCoordRange);
  ds.read(celestialInformation.zCoordRange);

  return ds;
}

DataStream& operator<<(DataStream& ds, CelestialBaseInformation const& celestialInformation) {
  ds.write(celestialInformation.planetOrbitalLevels);
  ds.write(celestialInformation.satelliteOrbitalLevels);
  ds.write(celestialInformation.chunkSize);
  ds.write(celestialInformation.xyCoordRange);
  ds.write(celestialInformation.zCoordRange);

  return ds;
}

}
