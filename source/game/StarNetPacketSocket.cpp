#include "StarNetPacketSocket.hpp"
#include "StarIterator.hpp"
#include "StarCompression.hpp"
#include "StarLogging.hpp"

namespace Star {

PacketStatCollector::PacketStatCollector(float calculationWindow)
  : m_calculationWindow(calculationWindow), m_stats(), m_lastMixTime(0) {}

void PacketStatCollector::mix(PacketType type, size_t size) {
  calculate();
  m_unmixed[type] += size;
}

void PacketStatCollector::mix(HashMap<PacketType, size_t> const& sizes) {
  calculate();
  for (auto const& p : sizes)
    m_unmixed[p.first] += p.second;
}

PacketStats PacketStatCollector::stats() const {
  const_cast<PacketStatCollector*>(this)->calculate();
  return m_stats;
}

void PacketStatCollector::calculate() {
  int64_t currentTime = Time::monotonicMilliseconds();
  float elapsedTime = (currentTime - m_lastMixTime) / 1000.0f;
  if (elapsedTime >= m_calculationWindow) {
    m_lastMixTime = currentTime;
    m_stats.worstPacketSize = 0;

    float total = 0.0f;
    for (auto& pair : m_unmixed) {
      if (pair.second > m_stats.worstPacketSize) {
        m_stats.worstPacketType = pair.first;
        m_stats.worstPacketSize = pair.second;
      }

      auto& bytes = m_stats.packetBytesPerSecond[pair.first];
      bytes = pair.second / elapsedTime;
      total += bytes;
    }
    m_stats.bytesPerSecond = total;
    m_unmixed.clear();
  }
}

Maybe<PacketStats> PacketSocket::incomingStats() const {
  return {};
}

Maybe<PacketStats> PacketSocket::outgoingStats() const {
  return {};
}

void PacketSocket::setLegacy(bool legacy) { m_legacy = legacy; }
bool PacketSocket::legacy() const { return m_legacy; }

pair<LocalPacketSocketUPtr, LocalPacketSocketUPtr> LocalPacketSocket::openPair() {
  auto lhsIncomingPipe = make_shared<Pipe>();
  auto rhsIncomingPipe = make_shared<Pipe>();

  return {
    LocalPacketSocketUPtr(new LocalPacketSocket(lhsIncomingPipe, weak_ptr<Pipe>(rhsIncomingPipe))),
    LocalPacketSocketUPtr(new LocalPacketSocket(rhsIncomingPipe, weak_ptr<Pipe>(lhsIncomingPipe)))
  };
}

bool LocalPacketSocket::isOpen() const {
  return m_incomingPipe && !m_outgoingPipe.expired();
}

void LocalPacketSocket::close() {
  m_incomingPipe.reset();
}

void LocalPacketSocket::sendPackets(List<PacketPtr> packets) {
  if (!isOpen() || packets.empty())
    return;

  if (auto outgoingPipe = m_outgoingPipe.lock()) {
    MutexLocker locker(outgoingPipe->mutex);

#ifdef STAR_DEBUG
    // Test serialization if STAR_DEBUG is enabled
    DataStreamBuffer buffer;
    for (auto inPacket : take(packets)) {
      buffer.clear();
      inPacket->write(buffer);
      auto outPacket = createPacket(inPacket->type());
      buffer.seek(0);
      outPacket->read(buffer);
      packets.append(outPacket);
    }
#endif

    outgoingPipe->queue.appendAll(std::move(packets));
  }
}

List<PacketPtr> LocalPacketSocket::receivePackets() {
  MutexLocker locker(m_incomingPipe->mutex);
  List<PacketPtr> packets;
  packets.appendAll(take(m_incomingPipe->queue));
  return packets;
}

bool LocalPacketSocket::sentPacketsPending() const {
  return false;
}

bool LocalPacketSocket::writeData() {
  return false;
}

bool LocalPacketSocket::readData() {
  return false;
}

LocalPacketSocket::LocalPacketSocket(shared_ptr<Pipe> incomingPipe, weak_ptr<Pipe> outgoingPipe)
  : m_incomingPipe(std::move(incomingPipe)), m_outgoingPipe(std::move(outgoingPipe)) {}

TcpPacketSocketUPtr TcpPacketSocket::open(TcpSocketPtr socket) {
  socket->setNoDelay(true);
  socket->setNonBlocking(true);
  return TcpPacketSocketUPtr(new TcpPacketSocket(std::move(socket)));
}

bool TcpPacketSocket::isOpen() const {
  return m_socket->isActive();
}

void TcpPacketSocket::close() {
  m_socket->close();
}

void TcpPacketSocket::sendPackets(List<PacketPtr> packets) {
  auto it = makeSMutableIterator(packets);

  while (it.hasNext()) {
    PacketType currentType = it.peekNext()->type();
    PacketCompressionMode currentCompressionMode = it.peekNext()->compressionMode();

    DataStreamBuffer packetBuffer;
    while (it.hasNext()
      && it.peekNext()->type() == currentType
      && it.peekNext()->compressionMode() == currentCompressionMode) {
      if (legacy())
        it.next()->writeLegacy(packetBuffer);
      else
        it.next()->write(packetBuffer);
    }

    // Packets must read and write actual data, because this is used to
    // determine packet count
    starAssert(!packetBuffer.empty());

    ByteArray compressedPackets;
    bool mustCompress = currentCompressionMode == PacketCompressionMode::Enabled;
    bool perhapsCompress = currentCompressionMode == PacketCompressionMode::Automatic && packetBuffer.size() > 64;
    if (mustCompress || perhapsCompress)
      compressedPackets = compressData(packetBuffer.data());

    DataStreamBuffer outBuffer;
    outBuffer.write(currentType);

    if (!compressedPackets.empty() && (mustCompress || compressedPackets.size() < packetBuffer.size())) {
      outBuffer.writeVlqI(-(int)(compressedPackets.size()));
      outBuffer.writeData(compressedPackets.ptr(), compressedPackets.size());
      m_outgoingStats.mix(currentType, compressedPackets.size());
    } else {
      outBuffer.writeVlqI((int)(packetBuffer.size()));
      outBuffer.writeData(packetBuffer.ptr(), packetBuffer.size());
      m_outgoingStats.mix(currentType, packetBuffer.size());
    }
    m_outputBuffer.append(outBuffer.takeData());
  }
}

List<PacketPtr> TcpPacketSocket::receivePackets() {
  uint64_t const PacketSizeLimit = 64<<20;
  List<PacketPtr> packets;
  try {
    while (!m_inputBuffer.empty()) {
      PacketType packetType;
      uint64_t packetSize = 0;
      bool packetCompressed = false;

      DataStreamBuffer ds(m_inputBuffer);
      try {
        packetType = ds.read<PacketType>();
        int64_t len = ds.readVlqI();
        if (len < 0) {
          packetSize = -len;
          packetCompressed = true;
        } else {
          packetSize = len;
          packetCompressed = false;
        }
      } catch (EofException const&) {
        // Guard against not having the entire packet header available when
        // trying to read.
        break;
      }

      if (packetSize > PacketSizeLimit)
        throw IOException::format("Packet size {} exceeds maximum allowed packet size!", packetSize);

      if (packetSize > ds.size() - ds.pos())
        break;

      ByteArray packetBytes = ds.readBytes(packetSize);
      if (packetCompressed)
        packetBytes = uncompressData(packetBytes);

      m_incomingStats.mix(packetType, packetSize);

      DataStreamBuffer packetStream(std::move(packetBytes));
      do {
        PacketPtr packet = createPacket(packetType);
        packet->setCompressionMode(packetCompressed ? PacketCompressionMode::Enabled : PacketCompressionMode::Disabled);
        if (legacy())
          packet->readLegacy(packetStream);
        else
          packet->read(packetStream);
        packets.append(std::move(packet));
      } while (!packetStream.atEnd());

      m_inputBuffer = ds.readBytes(ds.size() - ds.pos());
    }
  } catch (IOException const& e) {
    Logger::warn("I/O error in TcpPacketSocket::readPackets, closing: {}", outputException(e, false));
    m_inputBuffer.clear();
    m_socket->shutdown();
  }
  return packets;
}

bool TcpPacketSocket::sentPacketsPending() const {
  return !m_outputBuffer.empty();
}

bool TcpPacketSocket::writeData() {
  if (!isOpen())
    return false;

  bool dataSent = false;
  try {
    if (m_outputBuffer.empty())
      return false;

    while (!m_outputBuffer.empty()) {
      size_t writtenAmount = m_socket->send(m_outputBuffer.ptr(), m_outputBuffer.size());
      if (writtenAmount == 0)
        break;
      dataSent = true;
      m_outputBuffer.trimLeft(writtenAmount);
    }
  } catch (SocketClosedException const& e) {
    Logger::debug("TcpPacketSocket socket closed: {}", outputException(e, false));
  } catch (IOException const& e) {
    Logger::warn("I/O error in TcpPacketSocket::sendData: {}", outputException(e, false));
    m_socket->shutdown();
  }
  return dataSent;
}

bool TcpPacketSocket::readData() {
  bool dataReceived = false;
  try {
    char readBuffer[1024];
    while (true) {
      size_t readAmount = m_socket->receive(readBuffer, 1024);
      if (readAmount == 0)
        break;
      dataReceived = true;
      m_inputBuffer.append(readBuffer, readAmount);
    }
  } catch (SocketClosedException const& e) {
    Logger::debug("TcpPacketSocket socket closed: {}", outputException(e, false));
  } catch (IOException const& e) {
    Logger::warn("I/O error in TcpPacketSocket::receiveData: {}", outputException(e, false));
    m_socket->shutdown();
  }
  return dataReceived;
}

Maybe<PacketStats> TcpPacketSocket::incomingStats() const {
  return m_incomingStats.stats();
}

Maybe<PacketStats> TcpPacketSocket::outgoingStats() const {
  return m_outgoingStats.stats();
}

TcpPacketSocket::TcpPacketSocket(TcpSocketPtr socket)
  : m_socket(std::move(socket)) {}

P2PPacketSocketUPtr P2PPacketSocket::open(P2PSocketUPtr socket) {
  return P2PPacketSocketUPtr(new P2PPacketSocket(std::move(socket)));
}

bool P2PPacketSocket::isOpen() const {
  return m_socket && m_socket->isOpen();
}

void P2PPacketSocket::close() {
  m_socket.reset();
}

void P2PPacketSocket::sendPackets(List<PacketPtr> packets) {
  auto it = makeSMutableIterator(packets);

  while (it.hasNext()) {
    PacketType currentType = it.peekNext()->type();
    PacketCompressionMode currentCompressionMode = it.peekNext()->compressionMode();

    DataStreamBuffer packetBuffer;
    while (it.hasNext()
      && it.peekNext()->type() == currentType
      && it.peekNext()->compressionMode() == currentCompressionMode) {
      if (legacy())
        it.next()->writeLegacy(packetBuffer);
      else
        it.next()->write(packetBuffer);
    }

    // Packets must read and write actual data, because this is used to
    // determine packet count
    starAssert(!packetBuffer.empty());

    ByteArray compressedPackets;
    bool mustCompress = currentCompressionMode == PacketCompressionMode::Enabled;
    bool perhapsCompress = currentCompressionMode == PacketCompressionMode::Automatic && packetBuffer.size() > 64;
    if (mustCompress || perhapsCompress)
      compressedPackets = compressData(packetBuffer.data());

    DataStreamBuffer outBuffer;
    outBuffer.write(currentType);

    if (!compressedPackets.empty() && (mustCompress || compressedPackets.size() < packetBuffer.size())) {
      outBuffer.write<bool>(true);
      outBuffer.writeData(compressedPackets.ptr(), compressedPackets.size());
      m_outgoingStats.mix(currentType, compressedPackets.size());
    } else {
      outBuffer.write<bool>(false);
      outBuffer.writeData(packetBuffer.ptr(), packetBuffer.size());
      m_outgoingStats.mix(currentType, packetBuffer.size());
    }
    m_outputMessages.append(outBuffer.takeData());
  }
}

List<PacketPtr> P2PPacketSocket::receivePackets() {
  List<PacketPtr> packets;
  try {
    for (auto& inputMessage : take(m_inputMessages)) {
      DataStreamBuffer ds(std::move(inputMessage));

      PacketType packetType = ds.read<PacketType>();
      bool packetCompressed = ds.read<bool>();
      size_t packetSize = ds.size() - ds.pos();

      ByteArray packetBytes = ds.readBytes(packetSize);
      if (packetCompressed)
        packetBytes = uncompressData(packetBytes);

      m_incomingStats.mix(packetType, packetSize);

      DataStreamBuffer packetStream(std::move(packetBytes));
      do {
        PacketPtr packet = createPacket(packetType);
        packet->setCompressionMode(packetCompressed ? PacketCompressionMode::Enabled : PacketCompressionMode::Disabled);
        if (legacy())
          packet->readLegacy(packetStream);
        else
          packet->read(packetStream);
        packets.append(std::move(packet));
      } while (!packetStream.atEnd());
    }
  } catch (IOException const& e) {
    Logger::warn("I/O error in P2PPacketSocket::readPackets, closing: {}", outputException(e, false));
    m_socket.reset();
  }
  return packets;
}

bool P2PPacketSocket::sentPacketsPending() const {
  return !m_outputMessages.empty();
}

bool P2PPacketSocket::writeData() {
  bool workDone = false;

  if (m_socket) {
    while (!m_outputMessages.empty()) {
      if (m_socket->sendMessage(m_outputMessages.first())) {
        m_outputMessages.removeFirst();
        workDone = true;
      } else {
        break;
      }
    }
  }

  return workDone;
}

bool P2PPacketSocket::readData() {
  bool workDone = false;

  if (m_socket) {
    while (auto message = m_socket->receiveMessage()) {
      m_inputMessages.append(message.take());
      workDone = true;
    }
  }

  return workDone;
}

Maybe<PacketStats> P2PPacketSocket::incomingStats() const {
  return m_incomingStats.stats();
}

Maybe<PacketStats> P2PPacketSocket::outgoingStats() const {
  return m_outgoingStats.stats();
}

P2PPacketSocket::P2PPacketSocket(P2PSocketPtr socket)
  : m_socket(std::move(socket)) {}

}
