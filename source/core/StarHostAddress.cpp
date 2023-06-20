#include "StarHostAddress.hpp"
#include "StarLexicalCast.hpp"
#include "StarNetImpl.hpp"

namespace Star {

HostAddress HostAddress::localhost(NetworkMode mode) {
  if (mode == NetworkMode::IPv4) {
    uint8_t addr[4] = {127, 0, 0, 1};
    return HostAddress(mode, addr);
  } else if (mode == NetworkMode::IPv6) {
    uint8_t addr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    return HostAddress(mode, addr);
  }

  return HostAddress();
}

Either<String, HostAddress> HostAddress::lookup(String const& address) {
  try {
    HostAddress ha;
    ha.set(address);
    return makeRight(move(ha));
  } catch (NetworkException const& e) {
    return makeLeft(String(e.what()));
  }
}

HostAddress::HostAddress(NetworkMode mode, uint8_t* address) {
  set(mode, address);
}

HostAddress::HostAddress(String const& address) {
  auto a = lookup(address);
  if (a.isLeft())
    throw NetworkException(a.left().takeUtf8());
  else
    *this = move(a.right());
}

NetworkMode HostAddress::mode() const {
  return m_mode;
}

uint8_t const* HostAddress::bytes() const {
  return m_address;
}

uint8_t HostAddress::octet(size_t i) const {
  return m_address[i];
}

bool HostAddress::isLocalHost() const {
  if (m_mode == NetworkMode::IPv4) {
    return (m_address[0] == 127 && m_address[1] == 0 && m_address[2] == 0 && m_address[3] == 1);

  } else {
    for (size_t i = 0; i < 15; ++i) {
      if (m_address[i] != 0)
        return false;
    }

    return m_address[15] == 1;
  }
}

bool HostAddress::isZero() const {
  if (mode() == NetworkMode::IPv4)
    return m_address[0] == 0 && m_address[1] == 0 && m_address[2] == 0 && m_address[3] == 0;

  if (mode() == NetworkMode::IPv6) {
    for (size_t i = 0; i < 16; i++) {
      if (m_address[i] != 0)
        return false;
    }
    return true;
  }

  return false;
}

size_t HostAddress::size() const {
  switch (m_mode) {
    case NetworkMode::IPv4:
      return 4;
    case NetworkMode::IPv6:
      return 16;
    default:
      return 0;
  }
}

bool HostAddress::operator==(HostAddress const& a) const {
  if (m_mode != a.m_mode)
    return false;

  size_t len = a.size();
  for (size_t i = 0; i < len; i++) {
    if (m_address[i] != a.m_address[i])
      return false;
  }

  return true;
}

void HostAddress::set(String const& address) {
  if (address.empty())
    return;

  if (address.compare("*") == 0 || address.compare("0.0.0.0") == 0) {
    uint8_t inaddr_any[4];
    memset(inaddr_any, 0, sizeof(inaddr_any));
    set(NetworkMode::IPv4, inaddr_any);
  } else if (address.compare("::") == 0) {
    // NOTE: This will likely bind to both IPv6 and IPv4, but it does depending
    // on the OS settings
    uint8_t inaddr_any[16];
    memset(inaddr_any, 0, sizeof(inaddr_any));
    set(NetworkMode::IPv6, inaddr_any);
  } else {
    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    // Eliminate duplicates being returned one for each socket type.
    // As we're not using the return socket type or protocol this doesn't effect
    // us.
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Request only usable addresses e.g. IPv6 only if IPv6 is available
    hints.ai_flags = AI_ADDRCONFIG;

    if (::getaddrinfo(address.utf8Ptr(), NULL, &hints, &result) != 0)
      throw NetworkException(strf("Failed to determine address for '%s' (%s)", address, netErrorString()));

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
      NetworkMode mode;
      switch (ptr->ai_family) {
        case AF_INET:
          mode = NetworkMode::IPv4;
          break;
        case AF_INET6:
          mode = NetworkMode::IPv6;
          break;
        default:
          continue;
      }
      if (mode == NetworkMode::IPv4) {
        struct sockaddr_in* info = (struct sockaddr_in*)ptr->ai_addr;
        set(mode, (uint8_t*)(&info->sin_addr));
      } else {
        struct sockaddr_in6* info = (struct sockaddr_in6*)ptr->ai_addr;
        set(mode, (uint8_t*)(&info->sin6_addr));
      }
      break;
    }
    freeaddrinfo(result);
  }
}

void HostAddress::set(NetworkMode mode, uint8_t const* addr) {
  m_mode = mode;
  if (addr)
    memcpy(m_address, addr, size());
  else
    memset(m_address, 0, 16);
}

std::ostream& operator<<(std::ostream& os, HostAddress const& address) {
  switch (address.mode()) {
    case NetworkMode::IPv4:
      format(os, "%d.%d.%d.%d", address.octet(0), address.octet(1), address.octet(2), address.octet(3));
      break;

    case NetworkMode::IPv6:
      format(os,
          "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
          address.octet(0),
          address.octet(1),
          address.octet(2),
          address.octet(3),
          address.octet(4),
          address.octet(5),
          address.octet(6),
          address.octet(7),
          address.octet(8),
          address.octet(9),
          address.octet(10),
          address.octet(11),
          address.octet(12),
          address.octet(13),
          address.octet(14),
          address.octet(15));
      break;

    default:
      throw NetworkException(strf("Unknown address mode (%d)", (int)address.mode()));
  }
  return os;
}

size_t hash<HostAddress>::operator()(HostAddress const& address) const {
  PLHasher hash;
  for (size_t i = 0; i < address.size(); ++i)
    hash.put(address.octet(i));
  return hash.hash();
}

Either<String, HostAddressWithPort> HostAddressWithPort::lookup(String const& address, uint16_t port) {
  auto hostAddress = HostAddress::lookup(address);
  if (hostAddress.isLeft())
    return makeLeft(move(hostAddress.left()));
  else
    return makeRight(HostAddressWithPort(move(hostAddress.right()), port));
}

Either<String, HostAddressWithPort> HostAddressWithPort::lookupWithPort(String const& address) {
  String host = address;
  String port = host.rextract(":");
  if (host.beginsWith("[") && host.endsWith("]"))
    host = host.substr(1, host.size() - 2);

  auto portNum = maybeLexicalCast<uint16_t>(port);
  if (!portNum)
    return makeLeft(strf("Could not parse port portion of HostAddressWithPort '%s'", port));

  auto hostAddress = HostAddress::lookup(host);
  if (hostAddress.isLeft())
    return makeLeft(move(hostAddress.left()));

  return makeRight(HostAddressWithPort(move(hostAddress.right()), *portNum));
}

HostAddressWithPort::HostAddressWithPort() : m_port(0) {}

HostAddressWithPort::HostAddressWithPort(HostAddress const& address, uint16_t port)
  : m_address(address), m_port(port) {}

HostAddressWithPort::HostAddressWithPort(NetworkMode mode, uint8_t* address, uint16_t port) {
  m_address = HostAddress(mode, address);
  m_port = port;
}

HostAddressWithPort::HostAddressWithPort(String const& address, uint16_t port) {
  auto a = lookup(address, port);
  if (a.isLeft())
    throw NetworkException(a.left().takeUtf8());
  *this = move(a.right());
}

HostAddressWithPort::HostAddressWithPort(String const& address) {
  auto a = lookupWithPort(address);
  if (a.isLeft())
    throw NetworkException(a.left().takeUtf8());
  *this = move(a.right());
}

HostAddress HostAddressWithPort::address() const {
  return m_address;
}

uint16_t HostAddressWithPort::port() const {
  return m_port;
}

bool HostAddressWithPort::operator==(HostAddressWithPort const& rhs) const {
  return tie(m_address, m_port) == tie(rhs.m_address, rhs.m_port);
}

std::ostream& operator<<(std::ostream& os, HostAddressWithPort const& addressWithPort) {
  os << addressWithPort.address() << ":" << addressWithPort.port();
  return os;
}

size_t hash<HostAddressWithPort>::operator()(HostAddressWithPort const& addressWithPort) const {
  return hashOf(addressWithPort.address(), addressWithPort.port());
}

}
