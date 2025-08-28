#include "StarVehicleDatabase.hpp"
#include "StarVehicle.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"

namespace Star {

VehicleDatabase::VehicleDatabase() : m_luaRoot(make_shared<LuaRoot>()) {
  auto assets = Root::singleton().assets();
  auto& files = assets->scanExtension("vehicle");
  assets->queueJsons(files);
  for (String file : files) {
    try {
      auto config = assets->json(file);
      String name = config.getString("name");

      if (m_vehicles.contains(name))
        throw VehicleDatabaseException::format("Repeat vehicle name '{}'", name);

      m_vehicles.add(std::move(name), make_pair(std::move(file), std::move(config)));
    } catch (StarException const& e) {
      throw VehicleDatabaseException(strf("Error loading vehicle '{}'", file), e);
    }
  }

  for (auto& path : assets->assetSources()) {
    auto metadata = assets->assetSourceMetadata(path);
    if (auto scripts = metadata.maybe("errorHandlers"))
      if (auto rebuildScripts = scripts.value().optArray("vehicle"))
        m_rebuildScripts.insertAllAt(0, jsonToStringList(rebuildScripts.value()));
  }
}

VehiclePtr VehicleDatabase::create(String const& vehicleName, Json const& extraConfig) const {
  auto configPair = m_vehicles.ptr(vehicleName);
  if (!configPair)
    throw VehicleDatabaseException::format("No such vehicle named '{}'", vehicleName);
  return make_shared<Vehicle>(configPair->second, configPair->first, extraConfig);
}

ByteArray VehicleDatabase::netStore(VehiclePtr const& vehicle, NetCompatibilityRules rules) const {
  DataStreamBuffer ds;
  ds.setStreamCompatibilityVersion(rules);

  ds.write(vehicle->baseConfig().getString("name"));
  ds.write(vehicle->dynamicConfig());
  return ds.takeData();
}

VehiclePtr VehicleDatabase::netLoad(ByteArray const& netStore, NetCompatibilityRules rules) const {
  DataStreamBuffer ds(netStore);
  ds.setStreamCompatibilityVersion(rules);

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
  VehiclePtr vehicle;
  try {
    vehicle = create(diskStore.getString("name"), diskStore.get("dynamicConfig"));
    vehicle->diskLoad(diskStore.get("state"));
    return vehicle;
  } catch (std::exception const& e) {
    auto lastException = e;
    Json newDiskStore = diskStore;
    for (auto script : m_rebuildScripts) {
      RecursiveMutexLocker locker(m_luaMutex);
      auto context = m_luaRoot->createContext(script);
      context.setCallbacks("root", LuaBindings::makeRootCallbacks());
      context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
      Json returnedDiskStore = context.invokePath<Json>("error", newDiskStore, strf("{}", outputException(lastException, false)));
      if (returnedDiskStore != newDiskStore) {
        newDiskStore = returnedDiskStore;
        if (script != m_rebuildScripts.last())
          try {
            vehicle = create(diskStore.getString("name"), diskStore.get("dynamicConfig"));
            vehicle->diskLoad(diskStore.get("state"));
            return vehicle;
          } catch (std::exception const& e) {
            lastException = e;
          }
      }
    }
    vehicle = create(diskStore.getString("name"), diskStore.get("dynamicConfig"));
    vehicle->diskLoad(diskStore.get("state"));
    return vehicle;
  }
}

}
