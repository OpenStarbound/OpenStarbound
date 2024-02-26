#pragma once

#include "StarString.hpp"
#include "StarByteArray.hpp"

namespace Star {

typedef struct sha_state_struct {
  uint32_t state[8], length, curlen;
  uint8_t buf[64];
} sha_state;

class Sha256Hasher {
public:
  Sha256Hasher();

  void push(char const* data, size_t length);
  void push(String const& data);
  void push(ByteArray const& data);

  // Produces 32 bytes
  void compute(char* hashDestination);
  ByteArray compute();

private:
  bool m_finished;
  sha_state m_state;
};

// Sha256 must, obviously, have 32 bytes available in the destination.
void sha256(char const* source, size_t length, char* hashDestination);

ByteArray sha256(char const* source, size_t length);

void sha256(ByteArray const& in, ByteArray& out);
void sha256(String const& in, ByteArray& out);

ByteArray sha256(ByteArray const& in);
ByteArray sha256(String const& in);

}
