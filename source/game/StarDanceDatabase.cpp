#include "StarDanceDatabase.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

DanceDatabase::DanceDatabase() {
  auto assets = Root::singleton().assets();
  auto& files = assets->scanExtension("dance");
  for (auto& file : files) {
    try {
      DancePtr dance = readDance(file);
      m_dances[dance->name] = dance;
    } catch (std::exception const& e) {
      Logger::error("Error loading dance file {}: {}", file, outputException(e, true));
    }
  }
}

DancePtr DanceDatabase::getDance(String const& name) const {
  if (auto dance = m_dances.ptr(name))
    return *dance;
  else {
    Logger::error("Invalid dance '{}', using default", name);
    return m_dances.get("assetmissing");
  }
}

DancePtr DanceDatabase::readDance(String const& path) {
  auto assets = Root::singleton().assets();
  Json config = assets->json(path);

  String name = config.getString("name");
  List<String> states = config.getArray("states").transformed([](Json const& state) { return state.toString(); });
  float cycle = config.getFloat("cycle");
  bool cyclic = config.getBool("cyclic");
  float duration = config.getFloat("duration");
  List<DanceStep> steps = config.getArray("steps").transformed([](Json const& step) {
    if (step.isType(Json::Type::Object)) {
      Maybe<String> bodyFrame = step.optString("bodyFrame");
      Maybe<String> frontArmFrame = step.optString("frontArmFrame");
      Maybe<String> backArmFrame = step.optString("backArmFrame");
      Vec2F headOffset = step.opt("headOffset").apply(jsonToVec2F).value(Vec2F());
      Vec2F frontArmOffset = step.opt("frontArmOffset").apply(jsonToVec2F).value(Vec2F());
      Vec2F backArmOffset = step.opt("backArmOffset").apply(jsonToVec2F).value(Vec2F());
      float frontArmRotation = step.optFloat("frontArmRotation").value(0.0f);
      float backArmRotation = step.optFloat("frontArmRotation").value(0.0f);
      return DanceStep{bodyFrame,
          frontArmFrame,
          backArmFrame,
          headOffset,
          frontArmOffset,
          backArmOffset,
          frontArmRotation,
          backArmRotation};
    } else {
      Maybe<String> bodyFrame = step.get(0).optString();
      Maybe<String> frontArmFrame = step.get(1).optString();
      Maybe<String> backArmFrame = step.get(2).optString();
      Vec2F headOffset = step.get(3).opt().apply(jsonToVec2F).value(Vec2F());
      Vec2F frontArmOffset = step.get(4).opt().apply(jsonToVec2F).value(Vec2F());
      Vec2F backArmOffset = step.get(5).opt().apply(jsonToVec2F).value(Vec2F());
      return DanceStep{bodyFrame, frontArmFrame, backArmFrame, headOffset, frontArmOffset, backArmOffset, 0.0f, 0.0f};
    }
  });

  return make_shared<Dance>(Dance{name, states, cycle, cyclic, duration, steps});
}

}
