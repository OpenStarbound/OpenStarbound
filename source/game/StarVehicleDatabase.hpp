#pragma once

#include "StarJson.hpp"
#include "StarVehicle.hpp"

namespace Star {

STAR_EXCEPTION(VehicleDatabaseException, StarException);

class VehicleDatabase {
public:
  VehicleDatabase();

  VehiclePtr create(String const& vehicleName, Json const& extraConfig = Json()) const;

  ByteArray netStore(VehiclePtr const& vehicle) const;
  VehiclePtr netLoad(ByteArray const& netStore) const;

  Json diskStore(VehiclePtr const& vehicle) const;
  VehiclePtr diskLoad(Json const& diskStore) const;

private:
  StringMap<pair<String, Json>> m_vehicles;
};

}
