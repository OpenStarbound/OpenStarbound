#include "StarVehicleDatabase.hpp"
#include "StarVehicle.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

VehicleDatabase::VehicleDatabase() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("vehicle");
  assets->queueJsons(files);
  for (auto file : files) {
    try {
      auto config = assets->json(file);
      String name = config.getString("name");

      if (m_vehicles.contains(name))
        throw VehicleDatabaseException::format("Repeat vehicle name '%s'", name);

      m_vehicles.add(move(name), make_pair(move(file), move(config)));
    } catch (StarException const& e) {
      throw VehicleDatabaseException(strf("Error loading vehicle '%s'", file), e);
    }
  }
}

VehiclePtr VehicleDatabase::create(String const& vehicleName, Json const& extraConfig) const {
  auto configPair = m_vehicles.ptr(vehicleName);
  if (!configPair)
    throw VehicleDatabaseException::format("No such vehicle named '%s'", vehicleName);
  return make_shared<Vehicle>(configPair->second, configPair->first, extraConfig);
}

ByteArray VehicleDatabase::netStore(VehiclePtr const& vehicle) const {
  DataStreamBuffer ds;
  ds.write(vehicle->baseConfig().getString("name"));
  ds.write(vehicle->dynamicConfig());
  return ds.takeData();
}

VehiclePtr VehicleDatabase::netLoad(ByteArray const& netStore) const {
  DataStreamBuffer ds(netStore);

  String name = ds.read<String>();
  auto dynamicConfig = ds.read<Json>();
  auto vehicle = create(name, dynamicConfig);

  return vehicle;
}

Json VehicleDatabase::diskStore(VehiclePtr const& vehicle) const {
  return JsonObject{
    {"name", vehicle->baseConfig().getString("name")},
    {"dynamicConfig", vehicle->dynamicConfig()},
    {"state", vehicle->diskStore()}
  };
}

VehiclePtr VehicleDatabase::diskLoad(Json const& diskStore) const {
  auto vehicle = create(diskStore.getString("name"), diskStore.get("dynamicConfig"));
  vehicle->diskLoad(diskStore.get("state"));
  return vehicle;
}

}
