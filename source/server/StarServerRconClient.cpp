#include "StarServerRconThread.hpp"
#include "StarServerRconClient.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarUniverseServer.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

ServerRconClient::ServerRconClient(UniverseServer* universe, TcpSocketPtr socket)
  : Thread("RconClient"),
    m_universe(universe),
    m_socket(socket),
    m_packetBuffer(MaxPacketSize),
    m_stop(true),
    m_authed(false) {
  auto& root = Root::singleton();
  auto cfg = root.configuration();

  m_packetBuffer.setByteOrder(ByteOrder::LittleEndian);
  m_packetBuffer.setNullTerminatedStrings(true);

  m_rconPassword = cfg->get("rconServerPassword").toString();
}

ServerRconClient::~ServerRconClient() {
  stop();
  join();
}

String ServerRconClient::handleCommand(String commandLine) {
  String command = commandLine.extract();

  if (command == "echo") {
    return commandLine;
  } else if (command == "broadcast" || command == "say") {
    m_universe->adminBroadcast(commandLine);
    return strf("OK: said {}", commandLine);
  } else if (command == "stop") {
    m_universe->stop();
    return "OK: shutting down";
  } else {
    return m_universe->adminCommand(strf("{} {}", command, commandLine));
  }
}

void ServerRconClient::receive(size_t size) {
  m_packetBuffer.reset(size);
  auto ptr = m_packetBuffer.ptr();
  while (size > 0) {
    size_t r = m_socket->receive(ptr, size);
    if (r == 0)
      throw NoMoreRequests();
    size -= r;
    ptr += r;
  }
}

void ServerRconClient::send(uint32_t requestId, uint32_t cmd, String str) {
  m_packetBuffer.clear();
  m_packetBuffer << (uint32_t)(str.utf8Size() + 10) << requestId << cmd << str << (uint8_t)0x00;
  m_socket->send(m_packetBuffer.ptr(), m_packetBuffer.size());
}

void ServerRconClient::sendAuthFailure() {
  send(SERVERDATA_AUTH_FAILURE, SERVERDATA_AUTH_RESPONSE, "");
}

void ServerRconClient::sendCmdResponse(uint32_t requestId, String response) {
  size_t len = response.length();
  // Always send at least one packet even if the response was blank
  do {
    auto dataLen = (len >= MaxPacketSize) ? MaxPacketSize : len;
    send(requestId, SERVERDATA_RESPONSE_VALUE, response.substr(0, dataLen));
    response = response.substr(dataLen);
    len = response.length();
  } while (len > 0);
}

void ServerRconClient::start() {
  m_stop = false;
  Thread::start();
}

void ServerRconClient::stop() {
  m_stop = true;
  m_socket->close();
}

void ServerRconClient::processRequest() {
  receive(4);
  uint32_t size = m_packetBuffer.read<uint32_t>();

  receive(size);
  uint32_t requestId;
  m_packetBuffer >> requestId;

  uint32_t cmd;
  m_packetBuffer >> cmd;

  switch (cmd) {
    case SERVERDATA_AUTH: {
      String password;
      m_packetBuffer >> password;
      if (!m_rconPassword.empty() && m_rconPassword.equals(password)) {
        m_authed = true;
        send(requestId, SERVERDATA_RESPONSE_VALUE);
        send(requestId, SERVERDATA_AUTH_RESPONSE);
      } else {
        m_authed = false;
        sendAuthFailure();
      }
      break;
    }
    case SERVERDATA_EXECCOMMAND:
      if (m_authed) {
        String command;
        m_packetBuffer >> command;
        try {
          Logger::info("RCON {}: {}", m_socket->remoteAddress(), command);
          sendCmdResponse(requestId, handleCommand(command));
        } catch (std::exception const& e) {
          sendCmdResponse(requestId, strf("RCON: Error executing: {}: {}", command, outputException(e, true)));
        }
      } else {
        sendAuthFailure();
      }
      break;
    default:
      sendCmdResponse(requestId, strf("Unknown request {:06x}", cmd));
  }
}

void ServerRconClient::run() {
  try {
    while (!m_stop)
      processRequest();
  } catch (NoMoreRequests const&) {
  } catch (std::exception const& e) {
    Logger::error("ServerRconClient exception caught: {}", outputException(e, false));
  }
}

}
