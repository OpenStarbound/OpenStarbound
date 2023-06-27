#ifndef STAR_HOST_ADDRESS_HPP
#define STAR_HOST_ADDRESS_HPP

#include "StarString.hpp"
#include "StarEither.hpp"

namespace Star {

STAR_EXCEPTION(NetworkException, IOException);

STAR_CLASS(HostAddress);

enum class NetworkMode {
  IPv4,
  IPv6
};

class HostAddress {
public:
  static HostAddress localhost(NetworkMode mode = NetworkMode::IPv4);

  // Returns either error or valid HostAddress
  static Either<String, HostAddress> lookup(String const& address);

  // If address is nullptr, constructs the zero address.
  HostAddress(NetworkMode mode = NetworkMode::IPv4, uint8_t* address = nullptr);
  // Throws if address is not valid
  explicit HostAddress(String const& address);

  NetworkMode mode() const;
  uint8_t const* bytes() const;
  uint8_t octet(size_t i) const;
  size_t size() const;

  bool isLocalHost() const;
  bool isZero() const;

  bool operator==(HostAddress const& a) const;

private:
  void set(String const& address);
  void set(NetworkMode mode, uint8_t const* addr);

  NetworkMode m_mode;
  uint8_t m_address[16];
};

std::ostream& operator<<(std::ostream& os, HostAddress const& address);

template <>
struct hash<HostAddress> {
  size_t operator()(HostAddress const& address) const;
};

class HostAddressWithPort {
public:
  // Returns either error or valid HostAddressWithPort
  static Either<String, HostAddressWithPort> lookup(String const& address, uint16_t port);
  // Format may have [] brackets around address or not, to distinguish address
  // portion from port portion.
  static Either<String, HostAddressWithPort> lookupWithPort(String const& address);

  HostAddressWithPort();
  HostAddressWithPort(HostAddress const& address, uint16_t port);
  HostAddressWithPort(NetworkMode mode, uint8_t* address, uint16_t port);
  // Throws if address or port is not valid
  HostAddressWithPort(String const& address, uint16_t port);
  explicit HostAddressWithPort(String const& address);

  HostAddress address() const;
  uint16_t port() const;

  bool operator==(HostAddressWithPort const& a) const;

private:
  HostAddress m_address;
  uint16_t m_port;
};

std::ostream& operator<<(std::ostream& os, HostAddressWithPort const& address);

template <>
struct hash<HostAddressWithPort> {
  size_t operator()(HostAddressWithPort const& address) const;
};

}

template <> struct fmt::formatter<Star::HostAddress> : ostream_formatter {};
template <> struct fmt::formatter<Star::HostAddressWithPort> : ostream_formatter {};

#endif
