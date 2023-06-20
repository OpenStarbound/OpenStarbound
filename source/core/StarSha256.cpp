#include "StarSha256.hpp"
#include "StarFormat.hpp"
#include "StarEncode.hpp"

namespace Star {

// An implementation of the SHA-256 hash function, this is endian neutral
// so should work just about anywhere.
//
// This code works much like the MD5 code provided by RSA. You sha_init()
// a "sha_state" then sha_process() the bytes you want and sha_done() to get
// the output.
//
// Revised Code:  Complies to SHA-256 standard now.
//
// Tom St Denis

// the K array
static const uint32_t K[64] = {0x428a2f98U,
    0x71374491U,
    0xb5c0fbcfU,
    0xe9b5dba5U,
    0x3956c25bU,
    0x59f111f1U,
    0x923f82a4U,
    0xab1c5ed5U,
    0xd807aa98U,
    0x12835b01U,
    0x243185beU,
    0x550c7dc3U,
    0x72be5d74U,
    0x80deb1feU,
    0x9bdc06a7U,
    0xc19bf174U,
    0xe49b69c1U,
    0xefbe4786U,
    0x0fc19dc6U,
    0x240ca1ccU,
    0x2de92c6fU,
    0x4a7484aaU,
    0x5cb0a9dcU,
    0x76f988daU,
    0x983e5152U,
    0xa831c66dU,
    0xb00327c8U,
    0xbf597fc7U,
    0xc6e00bf3U,
    0xd5a79147U,
    0x06ca6351U,
    0x14292967U,
    0x27b70a85U,
    0x2e1b2138U,
    0x4d2c6dfcU,
    0x53380d13U,
    0x650a7354U,
    0x766a0abbU,
    0x81c2c92eU,
    0x92722c85U,
    0xa2bfe8a1U,
    0xa81a664bU,
    0xc24b8b70U,
    0xc76c51a3U,
    0xd192e819U,
    0xd6990624U,
    0xf40e3585U,
    0x106aa070U,
    0x19a4c116U,
    0x1e376c08U,
    0x2748774cU,
    0x34b0bcb5U,
    0x391c0cb3U,
    0x4ed8aa4aU,
    0x5b9cca4fU,
    0x682e6ff3U,
    0x748f82eeU,
    0x78a5636fU,
    0x84c87814U,
    0x8cc70208U,
    0x90befffaU,
    0xa4506cebU,
    0xbef9a3f7U,
    0xc67178f2UL};

// Various logical functions
#define Ch(x, y, z) ((x & y) ^ (~x & z))
#define Maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define S(x, n) (((x) >> ((n)&31)) | ((x) << (32 - ((n)&31))))
#define R(x, n) ((x) >> (n))
#define Sigma0(x) (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x) (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x) (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x) (S(x, 17) ^ S(x, 19) ^ R(x, 10))

// compress 512-bits
static void sha_compress(sha_state* md) {
  uint32_t S[8], W[64], t0, t1;
  int i;

  /* copy state into S */
  for (i = 0; i < 8; i++)
    S[i] = md->state[i];

  /* copy the state into 512-bits into W[0..15] */
  for (i = 0; i < 16; i++)
    W[i] = (((uint32_t)md->buf[(4 * i) + 0]) << 24) | (((uint32_t)md->buf[(4 * i) + 1]) << 16)
        | (((uint32_t)md->buf[(4 * i) + 2]) << 8) | (((uint32_t)md->buf[(4 * i) + 3]));

  /* fill W[16..63] */
  for (i = 16; i < 64; i++)
    W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];

  /* Compress */
  for (i = 0; i < 64; i++) {
    t0 = S[7] + Sigma1(S[4]) + Ch(S[4], S[5], S[6]) + K[i] + W[i];
    t1 = Sigma0(S[0]) + Maj(S[0], S[1], S[2]);
    S[7] = S[6];
    S[6] = S[5];
    S[5] = S[4];
    S[4] = S[3] + t0;
    S[3] = S[2];
    S[2] = S[1];
    S[1] = S[0];
    S[0] = t0 + t1;
  }

  /* feedback */
  for (i = 0; i < 8; i++)
    md->state[i] += S[i];
}

// init the SHA state
static void sha_init(sha_state* md) {
  md->curlen = md->length = 0;
  md->state[0] = 0x6A09E667U;
  md->state[1] = 0xBB67AE85U;
  md->state[2] = 0x3C6EF372U;
  md->state[3] = 0xA54FF53AU;
  md->state[4] = 0x510E527FU;
  md->state[5] = 0x9B05688CU;
  md->state[6] = 0x1F83D9ABU;
  md->state[7] = 0x5BE0CD19U;
}

static void sha_process(sha_state* md, uint8_t* buf, int len) {
  while (len--) {
    /* copy byte */
    md->buf[md->curlen++] = *buf++;

    /* is 64 bytes full? */
    if (md->curlen == 64) {
      sha_compress(md);
      md->length += 512;
      md->curlen = 0;
    }
  }
}

static void sha_done(sha_state* md, uint8_t* hash) {
  int i;

  /* increase the length of the message */
  md->length += md->curlen * 8;

  /* append the '1' bit */
  md->buf[md->curlen++] = 0x80;

  /* if the length is currently above 56 bytes we append zeros then compress.
    Then we can fall back to padding zeros and length encoding like normal. */

  if (md->curlen > 56) {
    for (; md->curlen < 64;)
      md->buf[md->curlen++] = 0;
    sha_compress(md);
    md->curlen = 0;
  }

  /* pad upto 56 bytes of zeroes */
  for (; md->curlen < 56;)
    md->buf[md->curlen++] = 0;

  /* since all messages are under 2^32 bits we mark the top bits zero */
  for (i = 56; i < 60; i++)
    md->buf[i] = 0;

  /* append length */
  for (i = 60; i < 64; i++)
    md->buf[i] = (md->length >> ((63 - i) * 8)) & 255;
  sha_compress(md);

  /* copy output */
  for (i = 0; i < 32; i++)
    hash[i] = (md->state[i >> 2] >> (((3 - i) & 3) << 3)) & 255;
}

Sha256Hasher::Sha256Hasher() {
  m_finished = false;
  sha_init(&m_state);
}

void Sha256Hasher::push(char const* data, size_t length) {
  if (m_finished) {
    sha_init(&m_state);
    m_finished = false;
  }

  sha_process(&m_state, (uint8_t*)data, length);
}

void Sha256Hasher::push(String const& data) {
  push(data.utf8Ptr(), data.utf8Size());
}

void Sha256Hasher::push(ByteArray const& data) {
  push(data.ptr(), data.size());
}

ByteArray Sha256Hasher::compute() {
  ByteArray dest(32, 0);
  sha_done(&m_state, (uint8_t*)dest.ptr());
  m_finished = true;
  return dest;
}

void Sha256Hasher::compute(char* hashDestination) {
  sha_done(&m_state, (uint8_t*)hashDestination);
  m_finished = true;
}

void sha256(char const* source, size_t length, char* hashDestination) {
  sha_state state;
  sha_init(&state);
  sha_process(&state, (uint8_t*)source, length);
  sha_done(&state, (uint8_t*)hashDestination);
}

ByteArray sha256(char const* source, size_t length) {
  ByteArray dest(32, 0);
  sha256(source, length, dest.ptr());
  return dest;
}

void sha256(ByteArray const& in, ByteArray& out) {
  out.resize(32, 0);
  sha256(in.ptr(), in.size(), out.ptr());
}

void sha256(String const& in, ByteArray& out) {
  out.resize(32, 0);
  sha256(in.utf8Ptr(), in.utf8Size(), out.ptr());
}

ByteArray sha256(ByteArray const& in) {
  return sha256(in.ptr(), in.size());
}

ByteArray sha256(String const& in) {
  return sha256(in.utf8Ptr(), in.utf8Size());
}

}
