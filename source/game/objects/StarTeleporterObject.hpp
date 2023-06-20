#include "StarWarpTargetEntity.hpp"
#include "StarObject.hpp"

namespace Star {

class TeleporterObject : public Object, public WarpTargetEntity {
public:
  TeleporterObject(ObjectConfigConstPtr config, Json const& parameters = JsonObject());

  Vec2F footPosition() const override;
};

}
