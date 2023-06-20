#include "StarWorldClientState.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

WorldClientState::WorldClientState() {
  auto clientConfig = Root::singleton().assets()->json("/client.config");

  m_windowMonitoringBorder = clientConfig.getInt("windowMonitoringBorder");
  m_presenceEntityMonitoringBorder = clientConfig.getInt("presenceEntityMonitoringBorder");

  m_playerId.set(NullEntityId);

  m_netVersion = 0;

  m_netGroup.addNetElement(&m_windowXMin);
  m_netGroup.addNetElement(&m_windowYMin);
  m_netGroup.addNetElement(&m_windowWidth);
  m_netGroup.addNetElement(&m_windowHeight);

  m_netGroup.addNetElement(&m_playerId);
  m_netGroup.addNetElement(&m_clientPresenceEntities);
}

RectI WorldClientState::window() const {
  return RectI::withSize(Vec2I(m_windowXMin.get(), m_windowYMin.get()), Vec2I(m_windowWidth.get(), m_windowHeight.get()));
}

void WorldClientState::setWindow(RectI const& window) {
  m_windowXMin.set(window.xMin());
  m_windowYMin.set(window.yMin());
  m_windowWidth.set(window.width());
  m_windowHeight.set(window.height());
}

Vec2F WorldClientState::windowCenter() const {
  return RectF(window()).center();
}

EntityId WorldClientState::playerId() const {
  return m_playerId.get();
}

void WorldClientState::setPlayer(EntityId playerId) {
  return m_playerId.set(playerId);
}

List<EntityId> const& WorldClientState::clientPresenceEntities() const {
  return m_clientPresenceEntities.get();
}

void WorldClientState::setClientPresenceEntities(List<EntityId> entities) {
  m_clientPresenceEntities.set(move(entities));
}

List<RectI> WorldClientState::monitoringRegions(function<Maybe<RectI>(EntityId)> entityBounds) const {
  List<RectI> regions;
  
  auto windowRegion = window().padded(m_windowMonitoringBorder);

  if (window() != RectI())
    regions.append(windowRegion);

  if (auto playerBounds = entityBounds(m_playerId.get())) {
    // add an extra region the size of the window centered on the player's position to prevent nearby sectors
    // being unloaded due to camera panning or centering on other entities
    regions.append(RectI::withCenter(playerBounds->center(), windowRegion.size()));
  }

  for (auto entityId : m_clientPresenceEntities.get()) {
    if (auto bounds = entityBounds(entityId))
      regions.append(bounds->padded(m_presenceEntityMonitoringBorder));
  }

  return regions;
}

ByteArray WorldClientState::writeDelta() {
  ByteArray delta;
  tie(delta, m_netVersion) = m_netGroup.writeNetState(m_netVersion);
  return delta;
}

void WorldClientState::readDelta(ByteArray delta) {
  m_netGroup.readNetState(move(delta));
}

void WorldClientState::reset() {
  m_netVersion = 0;
}

}
