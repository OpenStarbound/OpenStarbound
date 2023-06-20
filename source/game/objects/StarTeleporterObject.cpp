#include "StarTeleporterObject.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

TeleporterObject::TeleporterObject(ObjectConfigConstPtr config, Json const& parameters) : Object(config, parameters) {
  setUniqueId(configValue("uniqueId", Uuid().hex()).optString());
}

Vec2F TeleporterObject::footPosition() const {
  if (auto footPos = configValue("teleporterFootPosition"))
    return jsonToVec2F(footPos);
  return Vec2F();
}

}
