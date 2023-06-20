#include "StarPlayerLog.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

PlayerLog::PlayerLog() {
  m_deathCount = 0;
  m_playTime = 0;
  m_introComplete = false;
  m_scannedObjects = StringSet();
  m_radioMessages = StringSet();
  m_cinematics = StringSet();
}

PlayerLog::PlayerLog(Json const& json) {
  m_deathCount = json.getInt("deathCount");
  m_playTime = json.getDouble("playTime");
  m_introComplete = json.getBool("introComplete");
  m_scannedObjects = jsonToStringSet(json.get("scannedObjects"));
  m_radioMessages = jsonToStringSet(json.get("radioMessages"));
  m_cinematics = jsonToStringSet(json.get("cinematics"));

  for (auto pair : json.get("collections").iterateObject())
    m_collections[pair.first] = jsonToStringSet(pair.second);
}

Json PlayerLog::toJson() const {
  auto collections = JsonObject();
  for (auto pair : m_collections)
    collections[pair.first] = jsonFromStringSet(pair.second);

  return JsonObject{{"deathCount", m_deathCount},
      {"playTime", m_playTime},
      {"introComplete", m_introComplete},
      {"scannedObjects", jsonFromStringSet(m_scannedObjects)},
      {"radioMessages", jsonFromStringSet(m_radioMessages)},
      {"cinematics", jsonFromStringSet(m_cinematics)},
      {"collections", collections}};
}

int PlayerLog::deathCount() const {
  return m_deathCount;
}

void PlayerLog::addDeathCount(int deaths) {
  m_deathCount += deaths;
}

double PlayerLog::playTime() const {
  return m_playTime;
}

void PlayerLog::addPlayTime(double elapsedTime) {
  m_playTime += elapsedTime;
}

bool PlayerLog::introComplete() const {
  return m_introComplete;
}

void PlayerLog::setIntroComplete(bool complete) {
  m_introComplete = complete;
}

StringSet PlayerLog::scannedObjects() const {
  return m_scannedObjects;
}

bool PlayerLog::addScannedObject(String const& objectName) {
  return m_scannedObjects.add(objectName);
}

void PlayerLog::removeScannedObject(String const& objectName) {
  m_scannedObjects.remove(objectName);
}

void PlayerLog::clearScannedObjects() {
  m_scannedObjects.clear();
}

StringSet PlayerLog::radioMessages() const {
  return m_radioMessages;
}

bool PlayerLog::addRadioMessage(String const& messageName) {
  return m_radioMessages.add(messageName);
}

void PlayerLog::clearRadioMessages() {
  m_radioMessages.clear();
}

StringSet PlayerLog::cinematics() const {
  return m_cinematics;
}

bool PlayerLog::addCinematic(String const& cinematic) {
  return m_cinematics.add(cinematic);
}

void PlayerLog::clearCinematics() {
  m_cinematics.clear();
}

StringList PlayerLog::collections() const {
  return m_collections.keys();
}

StringSet PlayerLog::collectables(String const& collection) const {
  if (!m_collections.contains(collection))
    return {};

  return m_collections.get(collection);
}

bool PlayerLog::addCollectable(String const& collection, String const& collectable) {
  if (!m_collections.contains(collection))
    m_collections[collection] = {};

  return m_collections[collection].add(collectable);
}

void PlayerLog::clearCollectables(String const& collection) {
  m_collections.remove(collection);
}

}
