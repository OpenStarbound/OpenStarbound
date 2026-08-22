#include "StarWorldClientThread.hpp"
#include "StarTickRateMonitor.hpp"
#include "StarNpc.hpp"
#include "StarRoot.hpp"
#include "StarLogging.hpp"
#include "StarAssets.hpp"
#include "StarPlayer.hpp"

namespace Star {

WorldClientThread::WorldClientThread(ClientSubWorldId subWorldId)
  : Thread("WorldClientThread: " + String(subWorldId)),
    m_subWorldId(subWorldId),
    m_stop(false),
    m_errorOccurred(false),
    m_shouldExpire(false) {
    m_worldClient = make_shared<WorldClient>(subWorldId);
}

WorldClientThread::~WorldClientThread() {
  m_stop = true;
  join();

  RecursiveMutexLocker locker(m_mutex);
}

ClientSubWorldId WorldClientThread::subWorldId() const {
  return m_subWorldId;
}

void WorldClientThread::start() {
  m_stop = false;
  m_errorOccurred = false;
  Thread::start();
}

void WorldClientThread::stop() {
  m_stop = true;
  Thread::join();
}

void WorldClientThread::setPause(shared_ptr<const atomic<bool>> pause) {
  m_pause = pause;
}

bool WorldClientThread::errorOccurred() {
  return m_errorOccurred;
}

bool WorldClientThread::shouldExpire() {
  return m_shouldExpire;
}

void WorldClientThread::pushIncomingPackets(List<PacketPtr> packets) {
  RecursiveMutexLocker queueLocker(m_queueMutex);
  m_incomingPacketQueue.appendAll(std::move(packets));
}

List<PacketPtr> WorldClientThread::pullOutgoingPackets() {
  RecursiveMutexLocker queueLocker(m_queueMutex);
  return take(m_outgoingPacketQueue);
}

void WorldClientThread::executeAction(WorldClientAction action) {
  RecursiveMutexLocker locker(m_mutex);
  action(this, m_worldClient.get());
}

void WorldClientThread::setUpdateAction(WorldClientAction updateAction) {
  RecursiveMutexLocker locker(m_mutex);
  m_updateAction = updateAction;
}

void WorldClientThread::passMessage(Message&& message) {
  RecursiveMutexLocker locker(m_messageMutex);
  m_messages.append(std::move(message));
}

void WorldClientThread::run() {
  try {
    auto& root = Root::singleton();
    double updateMeasureWindow = root.assets()->json("/client.config:subWorldUpdateMeasureWindow").toDouble();

    TickRateApproacher tickApproacher(1.0f / GlobalTimestep, updateMeasureWindow);

    while (!m_stop && !m_errorOccurred) {
      LogMap::set(strf("client_{}_update", m_subWorldId), strf("{:4.2f}Hz", tickApproacher.rate()));

      update();
      tickApproacher.setTargetTickRate(1.0f / GlobalTimestep);
      tickApproacher.tick();

      double spareTime = tickApproacher.spareTime();

      int64_t spareMilliseconds = floor(spareTime * 1000);
      if (spareMilliseconds > 0)
        Thread::sleepPrecise(spareMilliseconds);
    }
  } catch (std::exception const& e) {
    Logger::error("WorldClientThread exception caught: {}", outputException(e, true));
    m_errorOccurred = true;
  }
}

void WorldClientThread::update() {
  RecursiveMutexLocker locker(m_mutex);
  RecursiveMutexLocker queueLocker(m_queueMutex);
  auto incomingPackets = take(m_incomingPacketQueue);
  queueLocker.unlock();
  try {
    m_worldClient->handleIncomingPackets(std::move(incomingPackets));
  } catch (std::exception const& e) {
    Logger::error("WorldClientThread exception caught handling incoming packets: {}", outputException(e, true));
    queueLocker.lock();
    m_outgoingPacketQueue.append({make_shared<ClientSubWorldRequest>(m_subWorldId, WorldId())});
    queueLocker.unlock();
    m_errorOccurred = true;
  }

  float dt = GlobalTimestep * GlobalTimescale;
  if (dt > 0.0f && (!m_pause || *m_pause == false))
    m_worldClient->update(dt);

  List<Message> messages;
  {
    RecursiveMutexLocker locker(m_messageMutex);
    messages = std::move(m_messages);
  }
  for (auto& message : messages) {
    if (auto resp = m_worldClient->receiveMessage(ServerConnectionId, message.message, message.args))
      message.promise.fulfill(*resp);
    else
      message.promise.fail("Message not handled by world");
  }

  auto outgoingPackets = m_worldClient->getOutgoingPackets();
  auto shouldDestroy = m_worldClient->pullRequestedDestroy();
  queueLocker.lock();
  m_outgoingPacketQueue.append(make_shared<ClientSubWorldPackets>(m_subWorldId,std::move(outgoingPackets)));
  if (shouldDestroy) {
    Logger::info("WorldClientThread requesting destroy");
    m_outgoingPacketQueue.append(make_shared<ClientSubWorldRequest>(m_subWorldId, WorldId()));
  }
  queueLocker.unlock();

  m_shouldExpire = m_worldClient->shouldExpire();

  if (m_updateAction)
    m_updateAction(this, m_worldClient.get());
}

}
