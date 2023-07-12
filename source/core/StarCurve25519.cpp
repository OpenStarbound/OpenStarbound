#include "StarCurve25519.hpp"
#include "StarRandom.hpp"
#include "StarLogging.hpp"

#include "curve25519/include/curve25519_dh.h"
#include "curve25519/include/ed25519_signature.h"

namespace Star::Curve25519 {

struct KeySet {
  PrivateKey privateKey;
  PublicKey publicKey;

  KeySet() {
    SecretKey secret;
    Random::randBytes(SecretKeySize).copyTo((char*)secret.data());

    secret[0]  &= 248;
    secret[31] &= 127;
    secret[31] |= 64;

    ed25519_CreateKeyPair(privateKey.data(), publicKey.data(), nullptr, secret.data());

    Logger::info("Generated Curve25519 key-pair");
  }
};

static KeySet const& staticKeys() {
  static KeySet keys;

  return keys;
}

PrivateKey const& privateKey() { return staticKeys().privateKey; }



Signature sign(void* data, size_t len) {
  Signature signature;
  ed25519_SignMessage(signature.data(), privateKey().data(), nullptr, (unsigned char*)data, len);
  return signature;
}

bool verify(uint8_t const* signature, uint8_t const* publicKey, void* data, size_t len) {
  return ed25519_VerifySignature(signature, publicKey, (unsigned char*)data, len);
}

PublicKey  const& publicKey()  { return staticKeys().publicKey;  }

}