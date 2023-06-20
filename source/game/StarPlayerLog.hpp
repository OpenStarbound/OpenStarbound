#ifndef STAR_PLAYER_LOG_HPP
#define STAR_PLAYER_LOG_HPP

#include "StarSet.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_CLASS(PlayerLog);

class PlayerLog {
public:
  PlayerLog();
  PlayerLog(Json const& json);

  Json toJson() const;

  int deathCount() const;
  void addDeathCount(int deaths);

  double playTime() const;
  void addPlayTime(double elapsedTime);

  bool introComplete() const;
  void setIntroComplete(bool complete);

  StringSet scannedObjects() const;
  bool addScannedObject(String const& objectName);
  void removeScannedObject(String const& objectName);
  void clearScannedObjects();

  StringSet radioMessages() const;
  bool addRadioMessage(String const& messageName);
  void clearRadioMessages();

  StringSet cinematics() const;
  bool addCinematic(String const& cinematic);
  void clearCinematics();

  StringList collections() const;
  StringSet collectables(String const& collection) const;
  bool addCollectable(String const& collection, String const& collectable);
  void clearCollectables(String const& collection);

private:
  int m_deathCount;
  double m_playTime;
  bool m_introComplete;
  StringSet m_scannedObjects;
  StringSet m_radioMessages;
  StringSet m_cinematics;
  StringMap<StringSet> m_collections;
};

}

#endif
