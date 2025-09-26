#include "StarWireInterface.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarWorldClient.hpp"
#include "StarWireEntity.hpp"
#include "StarWorldGeometry.hpp"
#include "StarWorldPainter.hpp"
#include "StarPlayer.hpp"
#include "StarTools.hpp"
#include "StarAssets.hpp"

namespace Star {

WirePane::WirePane(WorldClientPtr worldClient, PlayerPtr player, WorldPainterPtr worldPainter) {
  m_worldClient = worldClient;
  m_player = player;
  m_worldPainter = worldPainter;

  m_connecting = false;

  auto assets = Root::singleton().assets();
  GuiReader reader;
  reader.construct(assets->json("/interface/wires/wires.config:gui"), this);

  m_inSize = Vec2F(context()->textureSize("/interface/wires/inbound.png")) / TilePixels;
  m_outSize = Vec2F(context()->textureSize("/interface/wires/outbound.png")) / TilePixels;
  m_nodeSize = Vec2F(1.8f, 1.8f);

  JsonObject config = assets->json("/player.config:wireConfig").toObject();
  m_minBeamWidth = config.get("minWireWidth").toFloat();
  m_maxBeamWidth = config.get("maxWireWidth").toFloat();
  m_beamWidthDev = config.value("wireWidthDev", (m_maxBeamWidth - m_minBeamWidth) / 3).toFloat();
  m_minBeamTrans = config.get("minWireTrans").toFloat();
  m_maxBeamTrans = config.get("maxWireTrans").toFloat();
  m_beamTransDev = config.value("wireTransDev", (m_maxBeamTrans - m_minBeamTrans) / 3).toFloat();
  m_innerBrightnessScale = config.get("innerBrightnessScale").toFloat();
  m_firstStripeThickness = config.get("firstStripeThickness").toFloat();
  m_secondStripeThickness = config.get("secondStripeThickness").toFloat();

  setTitle({}, "", "Wire you looking at me like that?");
  disableScissoring();
  markAsContainer();
}

void WirePane::reset() {
  m_connecting = false;
}

void WirePane::update(float) {
  if (!active())
    return;
  if (!m_worldClient->inWorld()) {
    dismiss();
    return;
  }

  if (m_connecting) {
    for (auto entity : m_worldClient->atTile<WireEntity>(m_sourceConnector.entityLocation)) {
      if (m_sourceConnector.nodeIndex < entity->nodeCount(m_sourceDirection))
        return;
    }

    // stop pending connection if node has been removed
    m_connecting = false;
  }
}

void WirePane::renderWire(Vec2F from, Vec2F to, Color baseColor) {
  if (m_worldClient->isTileProtected(Vec2I::floor(from)) || m_worldClient->isTileProtected(Vec2I::floor(to)))
    return;

  from = m_worldPainter->camera().worldToScreen(from);
  to = m_worldPainter->camera().worldToScreen(to);

  auto rangeRand = [&](float dev, float min, float max) {
    return clamp<float>(Random::nrandf(dev, max), min, max);
  };

  float lineThickness = m_worldPainter->camera().pixelRatio() * rangeRand(m_beamWidthDev, m_minBeamWidth, m_maxBeamWidth);
  float beamTransparency = rangeRand(m_beamTransDev, m_minBeamTrans, m_maxBeamTrans);
  baseColor.setAlphaF(baseColor.alphaF() * beamTransparency);
  Color innerStripe = baseColor;
  innerStripe.setValue(1 - (1 - innerStripe.value()) / m_innerBrightnessScale);
  innerStripe.setSaturation(innerStripe.saturation() / m_innerBrightnessScale);
  Color firstStripe = innerStripe;
  innerStripe.setValue(1 - (1 - innerStripe.value()) / m_innerBrightnessScale);
  innerStripe.setSaturation(innerStripe.saturation() / m_innerBrightnessScale);
  Color secondStripe = innerStripe;

  context()->drawLine(from, to, baseColor.toRgba(), lineThickness);
  context()->drawLine(from, to, firstStripe.toRgba(), lineThickness * m_firstStripeThickness);
  context()->drawLine(from, to, secondStripe.toRgba(), lineThickness * m_secondStripeThickness);
}

void WirePane::renderImpl() {
  if (!m_worldClient->inWorld())
    return;

  auto region = RectF(m_worldClient->clientWindow());

  auto const& camera = m_worldPainter->camera();
  auto badWire = Color::rgbf(0.6f + (float)sin(Time::monotonicTime() * Constants::pi * 2.0) * 0.4f, 0.0f, 0.0f);
  auto white = Color::White.toRgba();
  float phase = 0.5f + 0.5f * std::sin((double)Time::monotonicMilliseconds() / 100.0);
  auto drawLineColor = Color::Red.mix(Color::White, phase);

  for (auto entity : m_worldClient->query<WireEntity>(region)) {
    for (size_t i = 0; i < entity->nodeCount(WireDirection::Input); ++i) {
      Vec2I position = entity->tilePosition() + entity->nodePosition({WireDirection::Input, i});
      if (!m_worldClient->isTileProtected(position)) {
        context()->drawQuad(entity->nodeIcon({WireDirection::Input, i}),
            camera.worldToScreen(centerOfTile(position) - (m_inSize / 2.0f)),
            camera.pixelRatio(), white);
      }
    }

    for (size_t i = 0; i < entity->nodeCount(WireDirection::Output); ++i) {
      Vec2I position = entity->tilePosition() + entity->nodePosition({WireDirection::Output, i});
      if (!m_worldClient->isTileProtected(position)) {
        context()->drawQuad(entity->nodeIcon({WireDirection::Output, i}),
            camera.worldToScreen(centerOfTile(position) - (m_outSize / 2.0f)),
            camera.pixelRatio(), white);
      }
    }
  }

  HashSet<pair<WireConnection, WireConnection>> visitedConnections;
  for (auto entity : m_worldClient->query<WireEntity>(region)) {
    for (size_t i = 0; i < entity->nodeCount(WireDirection::Input); ++i) {
      Vec2I tilePosition = entity->tilePosition();
      Vec2I inPosition = tilePosition + entity->nodePosition({WireDirection::Input, i});

      for (auto const& connection : entity->connectionsForNode({WireDirection::Input, i})) {
        visitedConnections.add({{tilePosition, i}, connection});

        auto wire = entity->nodeColor({WireDirection::Input, i}).mix(Color::Black, 0.8f);
        Vec2I outPosition = connection.entityLocation;
        if (auto sourceEntity = m_worldClient->atTile<WireEntity>(connection.entityLocation).get(0)) {
          if (connection.nodeIndex < sourceEntity->nodeCount(WireDirection::Output)) {
            outPosition += sourceEntity->nodePosition({WireDirection::Output, connection.nodeIndex});
            wire = sourceEntity->nodeColor({WireDirection::Output, i});
            if (!sourceEntity->nodeState(WireNode{WireDirection::Output, connection.nodeIndex}))
              wire = wire.mix(Color::Black, 0.8f);
          } else {
            wire = badWire;
          }
        }

        renderWire(centerOfTile(inPosition), centerOfTile(outPosition), wire);
      }
    }

    for (size_t i = 0; i < entity->nodeCount(WireDirection::Output); ++i) {
      Vec2I tilePosition = entity->tilePosition();
      Vec2I outPosition = tilePosition + entity->nodePosition({WireDirection::Output, i});

      auto wire = entity->nodeColor({WireDirection::Output, i});
      if (!entity->nodeState({WireDirection::Output, i}))
        wire = wire.mix(Color::Black, 0.8f);

      for (auto const& connection : entity->connectionsForNode({WireDirection::Output, i})) {
        if (visitedConnections.add({connection, {tilePosition, i}}))
          continue;

        Vec2I inPosition = connection.entityLocation;
        if (auto sourceEntity = m_worldClient->atTile<WireEntity>(connection.entityLocation).get(0)) {
          if (connection.nodeIndex < sourceEntity->nodeCount(WireDirection::Input))
            inPosition += sourceEntity->nodePosition({WireDirection::Input, connection.nodeIndex});
          else
            wire = badWire;
        }

        renderWire(centerOfTile(outPosition), centerOfTile(inPosition), wire);
      }
    }
  }

  if (m_connecting) {
    Vec2F aimPos = m_worldPainter->camera().screenToWorld(Vec2F(m_mousePos) * m_context->interfaceScale());
    Vec2I sourcePosition = m_sourceConnector.entityLocation;
    if (auto sourceEntity = m_worldClient->atTile<WireEntity>(m_sourceConnector.entityLocation).get(0)) {
      if (m_sourceDirection == WireDirection::Input){
        sourcePosition += sourceEntity->nodePosition({WireDirection::Input, m_sourceConnector.nodeIndex});
        drawLineColor = sourceEntity->nodeColor({WireDirection::Input, m_sourceConnector.nodeIndex}).mix(Color::White, phase);
      }else{
        sourcePosition += sourceEntity->nodePosition({WireDirection::Output, m_sourceConnector.nodeIndex});
        drawLineColor = sourceEntity->nodeColor({WireDirection::Output, m_sourceConnector.nodeIndex}).mix(Color::White, phase);
      }
    }
    renderWire(centerOfTile(sourcePosition), aimPos, drawLineColor);
  }
}

bool WirePane::sendEvent(InputEvent const& event) {
  if (event.is<MouseMoveEvent>())
    m_mousePos = *context()->mousePosition(event);

  if (event.is<MouseButtonDownEvent>())
    m_mousePos = *context()->mousePosition(event);

  return false;
}

WireConnector::SwingResult WirePane::swing(WorldGeometry const& geometry, Vec2F pos, FireMode mode) {
  pos = geometry.xwrap(pos);

  if (m_worldClient->isTileProtected((Vec2I)pos)) {
    m_connecting = false;
    return Protected;
  }

  RectF bounds = {pos - Vec2F(16, 16), pos + Vec2F(16, 16)};

  if (mode == FireMode::Primary) {
    Maybe<WireConnection> matchNode;
    WireDirection matchDirection = WireDirection::Output;
    float bestDist = 10000;
    for (auto entity : m_worldClient->query<WireEntity>(bounds)) {
      for (size_t i = 0; i < entity->nodeCount(WireDirection::Input); ++i) {
        RectF inbounds = RectF::withSize(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Input, i})) - (m_nodeSize / 2.0f), m_nodeSize);
        if (geometry.rectContains(inbounds, pos)) {
          if (!matchNode) {
            matchNode = WireConnection{entity->tilePosition(), i};
            matchDirection = WireDirection::Input;
            bestDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Input, i})), pos).magnitudeSquared();
          } else {
            float thisDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Input, i})), pos).magnitudeSquared();
            if (thisDist < bestDist) {
              matchNode = WireConnection{entity->tilePosition(), i};
              matchDirection = WireDirection::Input;
              bestDist = thisDist;
            }
          }
        }
      }

      for (size_t i = 0; i < entity->nodeCount(WireDirection::Output); ++i) {
        RectF outbounds = RectF::withSize(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Output, i})) - (m_nodeSize / 2.0f), m_nodeSize);
        if (geometry.rectContains(outbounds, pos)) {
          if (!matchNode) {
            matchNode = WireConnection{entity->tilePosition(), i};
            matchDirection = WireDirection::Output;
            bestDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Output, i})), pos).magnitudeSquared();
          } else {
            float thisDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Output, i})), pos).magnitudeSquared();
            if (thisDist < bestDist) {
              matchNode = WireConnection{entity->tilePosition(), i};
              matchDirection = WireDirection::Output;
              bestDist = thisDist;
            }
          }
        }
      }
    }

    if (matchNode) {
      if (m_connecting) {
        if (m_sourceDirection == matchDirection) {
          return Mismatch;
        } else if (m_sourceConnector.entityLocation == matchNode->entityLocation) {
          return Mismatch;
        } else {
          if (matchDirection == WireDirection::Output)
            m_worldClient->connectWire(*matchNode, m_sourceConnector);
          else
            m_worldClient->connectWire(m_sourceConnector, *matchNode);
        }
      } else {
        m_connecting = true;
        m_sourceDirection = matchDirection;
        m_sourceConnector = *matchNode;
      }
      return Connect;
    }

  } else {
    m_connecting = false;

    Maybe<WireNode> matchNode;
    Maybe<Vec2I> matchPosition;
    float bestDist = 10000;
    for (auto entity : m_worldClient->query<WireEntity>(bounds)) {
      for (size_t i = 0; i < entity->nodeCount(WireDirection::Input); ++i) {
        RectF inbounds = RectF::withSize(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Input, i})) - (m_nodeSize / 2.0f), m_nodeSize);
        if (geometry.rectContains(inbounds, pos) && entity->connectionsForNode({WireDirection::Input, i}).size() > 0) {
          if (!matchNode) {
            matchPosition = entity->tilePosition();
            matchNode = WireNode{WireDirection::Input, i};
            bestDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Input, i})), pos).magnitudeSquared();
          } else {
            float thisDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Input, i})), pos).magnitudeSquared();
            if (thisDist < bestDist) {
              matchPosition = entity->tilePosition();
              matchNode = WireNode{WireDirection::Input, i};
              bestDist = thisDist;
            }
          }
        }
      }

      for (size_t i = 0; i < entity->nodeCount(WireDirection::Output); ++i) {
        RectF outbounds = RectF::withSize(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Output, i})) - (m_nodeSize / 2.0f), m_nodeSize);
        if (geometry.rectContains(outbounds, pos) && entity->connectionsForNode({WireDirection::Output, i}).size() > 0) {
          if (!matchNode) {
            matchPosition = entity->tilePosition();
            matchNode = WireNode{WireDirection::Output, i};
            bestDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Output, i})), pos).magnitudeSquared();
          } else {
            float thisDist = geometry.diff(centerOfTile(entity->tilePosition() + entity->nodePosition({WireDirection::Output, i})), pos).magnitudeSquared();
            if (thisDist < bestDist) {
              matchPosition = entity->tilePosition();
              matchNode = WireNode{WireDirection::Output, i};
              bestDist = thisDist;
            }
          }
        }
      }
    }

    if (matchNode) {
      m_worldClient->disconnectAllWires(*matchPosition, *matchNode);
      return Connect;
    }
  }
  return Nothing;
}

bool WirePane::connecting() {
  return m_connecting;
}

}
