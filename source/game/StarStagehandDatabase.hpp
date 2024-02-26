#pragma once

#include "StarJson.hpp"

namespace Star {

STAR_EXCEPTION(StagehandDatabaseException, StarException);

STAR_CLASS(Stagehand);

STAR_CLASS(StagehandDatabase);

class StagehandDatabase {
public:
  StagehandDatabase();

  StagehandPtr createStagehand(String const& stagehandType, Json const& extraConfig = Json()) const;

private:
  StringMap<Json> m_stagehandTypes;
};

}
