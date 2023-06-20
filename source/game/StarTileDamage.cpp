#include "StarTileDamage.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"

namespace Star {

List<Vec2I> tileAreaBrush(float range, Vec2F const& centerOffset, bool squareMode) {
  if (range == 0)
    return {};

  List<Vec2I> result;
  float workingRange = range * (squareMode ? 1 : 2) + (squareMode ? 0 : 1);
  Vec2F offset = Vec2F::filled(-workingRange / 2.0f);
  Vec2I intOffset = Vec2I::round(offset + centerOffset);
  for (int x = 0; x < workingRange; ++x) {
    for (int y = 0; y < workingRange; ++y) {
      Vec2F fromCenter = Vec2F(x, y) + Vec2F(0.5, 0.5) + offset; // distance to the center of the tile
      Vec2I intPos = Vec2I(x, y);
      if (squareMode || fromCenter.magnitude() <= range) {
        result.append(intPos + intOffset);
      }
    }
  }
  sort(result, [](Vec2I const& a, Vec2I const& b) {
      auto ams = a.magnitudeSquared();
      auto bms = b.magnitudeSquared();
      return std::tie(ams, a) < std::tie(bms, b);
    });

  return result;
}

EnumMap<TileDamageType> const TileDamageTypeNames{
  {TileDamageType::Protected, "protected"},
  {TileDamageType::Plantish, "plantish"},
  {TileDamageType::Blockish, "blockish"},
  {TileDamageType::Beamish, "beamish"},
  {TileDamageType::Explosive, "explosive"},
  {TileDamageType::Fire, "fire"},
  {TileDamageType::Tilling, "tilling"}
};

bool tileDamageIsPenetrating(TileDamageType damageType) {
  return damageType == TileDamageType::Explosive;
}

TileDamage::TileDamage() : type(), amount(), harvestLevel() {}

TileDamage::TileDamage(TileDamageType type, float amount, unsigned harvestLevel)
  : type(type), amount(amount), harvestLevel(harvestLevel) {}

DataStream& operator>>(DataStream& ds, TileDamage& tileDamage) {
  ds.read(tileDamage.type);
  ds.read(tileDamage.amount);
  ds.read(tileDamage.harvestLevel);
  return ds;
}

DataStream& operator<<(DataStream& ds, TileDamage const& tileDamage) {
  ds.write(tileDamage.type);
  ds.write(tileDamage.amount);
  ds.write(tileDamage.harvestLevel);
  return ds;
}

TileDamageParameters::TileDamageParameters()
  : m_damageRecoveryPerSecond(0.0f), m_maximumEffectTime(0.0f), m_totalHealth(0), m_requiredHarvestLevel(0) {}

TileDamageParameters::TileDamageParameters(Json config, Maybe<float> healthOverride, Maybe<unsigned> harvestLevelOverride) {
  if (config.type() == Json::Type::String)
    config = Root::singleton().assets()->json(config.toString());

  for (auto const& pair : config.getObject("damageFactors"))
    m_damages[TileDamageTypeNames.getLeft(pair.first)] = pair.second.toFloat();
  m_damageRecoveryPerSecond = config.getFloat("damageRecovery");

  m_requiredHarvestLevel = config.getUInt("harvestLevel", 1);
  m_maximumEffectTime = config.getFloat("maximumEffectTime", 1.5);

  m_totalHealth = config.getFloat("totalHealth", 1.0f);

  if (healthOverride)
    m_totalHealth = *healthOverride;
  if (harvestLevelOverride)
    m_requiredHarvestLevel = *harvestLevelOverride;
}

float TileDamageParameters::damageDone(TileDamage const& damage) const {
  return m_damages.value(damage.type) * damage.amount;
}

float TileDamageParameters::recoveryPerSecond() const {
  return m_damageRecoveryPerSecond;
}

unsigned TileDamageParameters::requiredHarvestLevel() const {
  return m_requiredHarvestLevel;
}

float TileDamageParameters::maximumEffectTime() const {
  return m_maximumEffectTime;
}

float TileDamageParameters::totalHealth() const {
  return m_totalHealth;
}

TileDamageParameters TileDamageParameters::sum(TileDamageParameters const& other) const {
  TileDamageParameters result;

  result.m_damageRecoveryPerSecond = m_damageRecoveryPerSecond + other.m_damageRecoveryPerSecond;
  result.m_totalHealth = m_totalHealth + other.m_totalHealth;

  result.m_requiredHarvestLevel = max(m_requiredHarvestLevel, other.m_requiredHarvestLevel);
  result.m_maximumEffectTime = max(m_maximumEffectTime, other.m_maximumEffectTime);

  for (auto key : m_damages.keys()) {
    if (other.m_damages.contains(key))
      result.m_damages[key] = result.m_totalHealth / ((m_totalHealth / m_damages.value(key, 0)) + (other.m_totalHealth / other.m_damages.value(key, 0)));
    else
      result.m_damages[key] = m_damages.value(key, 0);
  }

  for (auto key : m_damages.keys()) {
    if (m_damages.contains(key))
      result.m_damages[key] = result.m_totalHealth / ((m_totalHealth / m_damages.value(key, 0)) + (other.m_totalHealth / other.m_damages.value(key, 0)));
    else
      result.m_damages[key] = other.m_damages.value(key, 0);
  }

  return result;
}

Json TileDamageParameters::toJson() const {
  return JsonObject{
    {"damageFactors", jsonFromMapK<Map<TileDamageType, float>>(m_damages, [](TileDamageType a) {
        return TileDamageTypeNames.getRight(a);
      })},
    {"damageRecovery", m_damageRecoveryPerSecond},
    {"requiredHarvestLevel", m_requiredHarvestLevel},
    {"maximumEffectTime", m_maximumEffectTime},
    {"totalHealth", m_totalHealth}
  };
}

DataStream& operator>>(DataStream& ds, TileDamageParameters& tileDamage) {
  ds.readMapContainer(tileDamage.m_damages);
  ds.read(tileDamage.m_damageRecoveryPerSecond);
  ds.read(tileDamage.m_requiredHarvestLevel);
  ds.read(tileDamage.m_maximumEffectTime);
  ds.read(tileDamage.m_totalHealth);
  return ds;
}

DataStream& operator<<(DataStream& ds, TileDamageParameters const& tileDamage) {
  ds.writeMapContainer(tileDamage.m_damages);
  ds.write(tileDamage.m_damageRecoveryPerSecond);
  ds.write(tileDamage.m_requiredHarvestLevel);
  ds.write(tileDamage.m_maximumEffectTime);
  ds.write(tileDamage.m_totalHealth);
  return ds;
}

TileDamageStatus::TileDamageStatus() {
  reset();
}

void TileDamageStatus::reset() {
  m_damagePercentage = 0.0f;
  m_damageEffectTimeFactor = 0.0f;
  m_harvested = false;
  m_damageSourcePosition = Vec2F();
  m_damageType = TileDamageType::Protected;
  m_damageEffectPercentage = 0.0f;
}

void TileDamageStatus::damage(TileDamageParameters const& damageParameters, Vec2F const& sourcePosition, TileDamage const& damage) {
  auto percentageDelta = damageParameters.damageDone(damage) / damageParameters.totalHealth();
  m_damagePercentage = min(1.0f, m_damagePercentage + percentageDelta);
  m_harvested = damage.harvestLevel >= damageParameters.requiredHarvestLevel();
  m_damageSourcePosition = sourcePosition;
  m_damageType = damage.type;

  if (percentageDelta > 0)
    m_damageEffectTimeFactor = damageParameters.maximumEffectTime();

  updateDamageEffectPercentage();
}

void TileDamageStatus::recover(TileDamageParameters const& damageParameters, float dt) {
  // Once the tile becomes dead, it should not recover from it
  if (healthy() || dead())
    return;

  m_damagePercentage -= damageParameters.recoveryPerSecond() * dt / damageParameters.totalHealth();
  m_damageEffectTimeFactor -= dt;

  if (m_damagePercentage <= 0.0f) {
    m_damagePercentage = 0.0f;
    m_damageEffectTimeFactor = 0.0f;
    m_damageType = TileDamageType::Protected;
  }

  updateDamageEffectPercentage();
}

bool TileDamageStatus::healthy() const {
  return m_damagePercentage <= 0.0f;
}

bool TileDamageStatus::damaged() const {
  return m_damagePercentage > 0.0f;
}

bool TileDamageStatus::damageProtected() const {
  return m_damageType == TileDamageType::Protected;
}

bool TileDamageStatus::dead() const {
  return m_damagePercentage >= 1.0f && m_damageType != TileDamageType::Protected;
}

bool TileDamageStatus::harvested() const {
  return m_harvested;
}

DataStream& operator>>(DataStream& ds, TileDamageStatus& tileDamageStatus) {
  ds.read(tileDamageStatus.m_damagePercentage);
  ds.read(tileDamageStatus.m_damageEffectTimeFactor);
  ds.read(tileDamageStatus.m_harvested);
  ds.read(tileDamageStatus.m_damageSourcePosition);
  ds.read(tileDamageStatus.m_damageType);

  tileDamageStatus.updateDamageEffectPercentage();

  return ds;
}

DataStream& operator<<(DataStream& ds, TileDamageStatus const& tileDamageStatus) {
  ds.write(tileDamageStatus.m_damagePercentage);
  ds.write(tileDamageStatus.m_damageEffectTimeFactor);
  ds.write(tileDamageStatus.m_harvested);
  ds.write(tileDamageStatus.m_damageSourcePosition);
  ds.write(tileDamageStatus.m_damageType);

  return ds;
}

void TileDamageStatus::updateDamageEffectPercentage() {
  m_damageEffectPercentage = clamp(m_damageEffectTimeFactor, 0.0f, 1.0f) * m_damagePercentage;
}

EntityTileDamageStatus::EntityTileDamageStatus() {
  m_damagePercentage.setFixedPointBase(0.001f);
  m_damageEffectTimeFactor.setFixedPointBase(0.001f);
  m_damagePercentage.setInterpolator(lerp<float, float>);
  m_damageEffectTimeFactor.setInterpolator(lerp<float, float>);

  addNetElement(&m_damagePercentage);
  addNetElement(&m_damageEffectTimeFactor);
  addNetElement(&m_damageHarvested);
  addNetElement(&m_damageType);
}

float EntityTileDamageStatus::damagePercentage() const {
  return m_damagePercentage.get();
}

float EntityTileDamageStatus::damageEffectPercentage() const {
  return clamp(m_damageEffectTimeFactor.get(), 0.0f, 1.0f) * m_damagePercentage.get();
}

TileDamageType EntityTileDamageStatus::damageType() const {
  return m_damageType.get();
}

void EntityTileDamageStatus::reset() {
  m_damagePercentage.set(0.0f);
  m_damageEffectTimeFactor.set(0.0f);
  m_damageHarvested.set(false);
}

void EntityTileDamageStatus::damage(TileDamageParameters const& damageParameters, TileDamage const& damage) {
  auto percentageDelta = damageParameters.damageDone(damage) / damageParameters.totalHealth();
  m_damagePercentage.set(min(1.0f, m_damagePercentage.get() + percentageDelta));
  m_damageHarvested.set(damage.harvestLevel >= damageParameters.requiredHarvestLevel());
  m_damageType.set(damage.type);

  if (percentageDelta > 0)
    m_damageEffectTimeFactor.set(damageParameters.maximumEffectTime());
}

void EntityTileDamageStatus::recover(TileDamageParameters const& damageParameters, float dt) {
  // Once the tile becomes dead, it should not recover from it
  if (healthy() || dead())
    return;

  m_damagePercentage.set(m_damagePercentage.get() - damageParameters.recoveryPerSecond() * dt / damageParameters.totalHealth());
  m_damageEffectTimeFactor.set(m_damageEffectTimeFactor.get() - dt);

  if (m_damagePercentage.get() <= 0.0f) {
    m_damagePercentage.set(0.0f);
    m_damageEffectTimeFactor.set(0.0f);
    m_damageType.set(TileDamageType::Protected);
  }
}

bool EntityTileDamageStatus::healthy() const {
  return m_damagePercentage.get() <= 0.0f;
}

bool EntityTileDamageStatus::damaged() const {
  return m_damagePercentage.get() > 0.0f;
}

bool EntityTileDamageStatus::damageProtected() const {
  return m_damageType.get() == TileDamageType::Protected;
}

bool EntityTileDamageStatus::dead() const {
  return m_damagePercentage.get() >= 1.0f && m_damageType.get() != TileDamageType::Protected;
}

bool EntityTileDamageStatus::harvested() const {
  return m_damageHarvested.get();
}

}
