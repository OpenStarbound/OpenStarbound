#ifndef STAR_FARMABLE_OBJECT_HPP
#define STAR_FARMABLE_OBJECT_HPP

#include "StarObject.hpp"

namespace Star {

class FarmableObject : public Object {
public:
  FarmableObject(ObjectConfigConstPtr config, Json const& parameters);

  void update(float dt, uint64_t currentStep) override;

  bool damageTiles(List<Vec2I> const& position, Vec2F const& sourcePosition, TileDamage const& tileDamage) override;
  InteractAction interact(InteractRequest const& request) override;

  bool harvest();
  int stage() const;

protected:
  void readStoredData(Json const& diskStore) override;
  Json writeStoredData() const override;

private:
  void enterStage(int newStage);

  int m_stage;
  int m_stageAlt;
  double m_stageEnterTime;
  double m_nextStageTime;

  SlidingWindow m_immersion;
  float m_minImmersion;
  float m_maxImmersion;

  bool m_consumeSoilMoisture;

  JsonArray m_stages;
  bool m_finalStage;
};

}

#endif
