#include "StarNetPacketSocket.hpp"
#include "StarIterator.hpp"
#include "StarCompression.hpp"
#include "StarLogging.hpp"

namespace Star {

PacketStatCollector::PacketStatCollector(float calculationWindow)
  : m_calculationWindow(calculationWindow), m_stats(), m_totalBytes(0), m_lastMixTime(0) {}

void PacketStatCollector::mix(size_t size) {
  calculate();
  m_totalBytes += size;
}

void PacketStatCollector::mix(PacketType type, size_t size, bool addToTotal) {
  calculate();
  m_unmixed[type] += size;
  if (addToTotal)
    m_totalBytes += size;
}

void PacketStatCollector::mix(HashMap<PacketType, size_t> const& sizes, bool addToTotal) {
  calculate();
  for (auto const& p : sizes) {
    if (addToTotal)
      m_totalBytes += p.second;
    m_unmixed[p.first] += p.second;
  }
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

    for (auto& pair : m_unmixed) {
      if (pair.second > m_stats.worstPacketSize) {
        m_stats.worstPacketType = pair.first;
        m_stats.worstPacketSize = pair.second;
      }

      m_stats.packetBytesPerSecond[pair.first] = round(pair.second / elapsedTime);
    }
    m_stats.bytesPerSecond = round(float(m_totalBytes) / elapsedTime);
    m_totalBytes = 0;
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

void CompressedPacketSocket::setCompressionStreamEnabled(bool enabled) { m_useCompressionStream = enabled; }
bool CompressedPacketSocket::compressionStreamEnabled() const { return m_useCompressionStream; }

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
  if (compressionStreamEnabled()) {
    DataStreamBuffer outBuffer;
    while (it.hasNext()) {
      PacketPtr& packet = it.next();
      auto packetType = packet->type();
      DataStreamBuffer packetBuffer;
      packet->write(packetBuffer);
      outBuffer.write(packetType);
      outBuffer.writeVlqI((int)packetBuffer.size());
      outBuffer.writeData(packetBuffer.ptr(), packetBuffer.size());
      m_outgoingStats.mix(packetType, packetBuffer.size(), false);
    }
    m_outputBuffer.append(outBuffer.ptr(), outBuffer.size());
  } else {
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
}

List<PacketPtr> TcpPacketSocket::receivePackets() {
  // How large can uncompressed packets be
  // this limit is now also used during decompression
  uint64_t const PacketSizeLimit = 64 << 20;
  // How many packets can be batched together into one compressed chunk at once
  uint64_t const PacketBatchLimit = 131072;
  List<PacketPtr> packets;
  try {
    DataStreamExternalBuffer ds(m_inputBuffer);
    size_t trimPos = 0;
    while (!ds.atEnd()) {
      PacketType packetType;
      uint64_t packetSize = 0;
      bool packetCompressed = false;

      try {
        packetType = ds.read<PacketType>();
        int64_t len = ds.readVlqI();
        packetCompressed = len < 0;
        packetSize = packetCompressed ? -len : len;
      } catch (EofException const&) {
        // Guard against not having the entire packet header available when
        // trying to read.
        break;
      }

      if (packetSize > PacketSizeLimit)
        throw IOException::format("{} bytes large {} exceeds max size!", packetSize, PacketTypeNames.getRight(packetType));

      if (packetSize > ds.remaining())
        break;

      m_incomingStats.mix(packetType, packetSize, !compressionStreamEnabled());

      DataStreamExternalBuffer packetStream(ds.ptr() + ds.pos(), packetSize);
      ByteArray uncompressed;
      if (packetCompressed) {
        uncompressed = uncompressData(packetStream.ptr(), packetSize, PacketSizeLimit);
        packetStream.reset(uncompressed.ptr(), uncompressed.size());
      }
      ds.seek(packetSize, IOSeek::Relative);
      trimPos = ds.pos();

      size_t count = 0;
      do {
        if (++count > PacketBatchLimit) {
          throw IOException::format("Packet batch limit {} reached while reading {}s!", PacketBatchLimit, PacketTypeNames.getRight(packetType));
          break;
        }
        PacketPtr packet = createPacket(packetType);
        packet->setCompressionMode(packetCompressed ? PacketCompressionMode::Enabled : PacketCompressionMode::Disabled);
        if (legacy())
          packet->readLegacy(packetStream);
        else
          packet->read(packetStream);
        packets.append(std::move(packet));
      } while (!packetStream.atEnd());
    }
    if (trimPos)
      m_inputBuffer.trimLeft(trimPos);
  } catch (IOException const& e) {
    Logger::warn("I/O error in TcpPacketSocket::receivePackets, closing: {}", outputException(e, false));
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
    if (!m_outputBuffer.empty()) {
      if (compressionStreamEnabled()) {
        auto compressedBuffer = m_compressionStream.compress(m_outputBuffer);
        m_outputBuffer.clear();
        do {
          size_t written = m_socket->send(compressedBuffer.ptr(), compressedBuffer.size());
          if (written > 0) {
            dataSent = true;
            compressedBuffer.trimLeft(written);
            m_outgoingStats.mix(written);
          }
        } while (!compressedBuffer.empty());
      } else {
        do {
          size_t written = m_socket->send(m_outputBuffer.ptr(), m_outputBuffer.size());
          if (written == 0)
            break;
          dataSent = true;
          m_outputBuffer.trimLeft(written);
        } while (!m_outputBuffer.empty());
      }
    }
  } catch (SocketClosedException const& e) {
    Logger::debug("TcpPacketSocket socket closed: {}", outputException(e, false));
  } catch (IOException const& e) {
    Logger::warn("I/O error in TcpPacketSocket::writeData: {}", outputException(e, false));
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
      if (compressionStreamEnabled()) {
        m_incomingStats.mix(readAmount);
        auto decompressed = m_decompressionStream.decompress(readBuffer, readAmount);
        m_inputBuffer.append(decompressed);
      } else {
        m_inputBuffer.append(readBuffer, readAmount);
      }
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

void TcpPacketSocket::setLegacy(bool legacy) {
  PacketSocket::setLegacy(legacy);
}

TcpPacketSocket::TcpPacketSocket(TcpSocketPtr socket) : m_socket(std::move(socket)) {}

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

  if (compressionStreamEnabled()) {
    DataStreamBuffer outBuffer;
    while (it.hasNext()) {
      PacketType currentType = it.peekNext()->type();
      DataStreamBuffer packetBuffer;
      while (it.hasNext() && it.peekNext()->type() == currentType)
        it.next()->write(packetBuffer);
      outBuffer.write(currentType);
      outBuffer.write<bool>(false);
      outBuffer.writeData(packetBuffer.ptr(), packetBuffer.size());
      m_outgoingStats.mix(currentType, packetBuffer.size(), false);
    }
    m_outputMessages.append(m_compressionStream.compress(outBuffer.data()));
  } else {
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

      m_incomingStats.mix(packetType, packetSize, !compressionStreamEnabled());

      DataStreamExternalBuffer packetStream(packetBytes);
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
    Logger::warn("I/O error in P2PPacketSocket::receivePackets, closing: {}", outputException(e, false));
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
      m_incomingStats.mix(message->size());
      m_inputMessages.append(compressionStreamEnabled()
        ? m_decompressionStream.decompress(*message)
        : *message);
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
