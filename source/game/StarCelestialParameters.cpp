#include "StarCelestialParameters.hpp"
#include "StarStaticRandom.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarWeatherTypes.hpp"

namespace Star {

CelestialParameters::CelestialParameters() : m_seed(0) {}

CelestialParameters::CelestialParameters(CelestialCoordinate coordinate, uint64_t seed, String name, Json parameters)
  : m_coordinate(move(coordinate)), m_seed(seed), m_name(move(name)), m_parameters(move(parameters)) {
  if (auto worldType = getParameter("worldType").optString()) {
    if (worldType->equalsIgnoreCase("Terrestrial")) {
      auto worldSize = getParameter("worldSize").toString();
      auto type = randomizeParameterList("terrestrialType").toString();
      m_visitableParameters = generateTerrestrialWorldParameters(type, worldSize, m_seed);
    } else if (worldType->equalsIgnoreCase("Asteroids")) {
      m_visitableParameters = generateAsteroidsWorldParameters(m_seed);
    } else if (worldType->equalsIgnoreCase("FloatingDungeon")) {
      m_visitableParameters = generateFloatingDungeonWorldParameters(getParameter("dungeonWorld").toString());
    }
  }
}

CelestialParameters::CelestialParameters(ByteArray netStore) {
  DataStreamBuffer ds(move(netStore));
  ds >> m_coordinate;
  ds >> m_seed;
  ds >> m_name;
  ds >> m_parameters;
  m_visitableParameters = netLoadVisitableWorldParameters(ds.read<ByteArray>());
}

CelestialParameters::CelestialParameters(Json const& variant) {
  m_coordinate = CelestialCoordinate(variant.get("coordinate"));
  m_seed = variant.getUInt("seed");
  m_name = variant.getString("name");
  m_parameters = variant.get("parameters");
  m_visitableParameters = diskLoadVisitableWorldParameters(variant.get("visitableParameters"));
}

Json CelestialParameters::diskStore() const {
  return JsonObject{{"coordinate", m_coordinate.toJson()},
      {"seed", m_seed},
      {"name", m_name},
      {"parameters", m_parameters},
      {"visitableParameters", diskStoreVisitableWorldParameters(m_visitableParameters)}};
}

ByteArray CelestialParameters::netStore() const {
  DataStreamBuffer ds;
  ds << m_coordinate;
  ds << m_seed;
  ds << m_name;
  ds << m_parameters;
  ds.write(netStoreVisitableWorldParameters(m_visitableParameters));

  return ds.takeData();
}

CelestialCoordinate CelestialParameters::coordinate() const {
  return m_coordinate;
}

String CelestialParameters::name() const {
  return m_name;
}

uint64_t CelestialParameters::seed() const {
  return m_seed;
}

Json CelestialParameters::parameters() const {
  return m_parameters;
}

Json CelestialParameters::getParameter(String const& name, Json def) const {
  return m_parameters.get(name, move(def));
}

Json CelestialParameters::randomizeParameterList(String const& name, int32_t mix) const {
  auto parameter = getParameter(name);
  if (parameter.isNull())
    return Json();
  return staticRandomFrom(parameter.toArray(), mix, m_seed, name);
}

Json CelestialParameters::randomizeParameterRange(String const& name, int32_t mix) const {
  auto parameter = getParameter(name);
  if (parameter.isNull()) {
    return Json();
  } else {
    JsonArray list = parameter.toArray();
    if (list.size() != 2)
      throw CelestialException(
          strf("Parameter '%s' does not appear to be a range in CelestialParameters::randomizeRange", name));

    return randomizeParameterRange(list, mix, name);
  }
}

Json CelestialParameters::randomizeParameterRange(
    JsonArray const& range, int32_t mix, Maybe<String> const& name) const {
  if (range[0].type() == Json::Type::Int) {
    int64_t min = range[0].toInt();
    int64_t max = range[1].toInt();
    return staticRandomU64(mix, m_seed, name.value("")) % (max - min + 1) + min;
  } else {
    double min = range[0].toDouble();
    double max = range[1].toDouble();
    return staticRandomDouble(mix, m_seed, name.value("")) * (max - min) + min;
  }
}

bool CelestialParameters::isVisitable() const {
  return (bool)m_visitableParameters;
}

VisitableWorldParametersConstPtr CelestialParameters::visitableParameters() const {
  return m_visitableParameters;
}

void CelestialParameters::setVisitableParameters(VisitableWorldParametersPtr const& newVisitableParameters) {
  m_visitableParameters = newVisitableParameters;
}

}
