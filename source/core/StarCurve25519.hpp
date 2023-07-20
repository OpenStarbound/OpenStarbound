#ifndef STAR_CURVE_25519_HPP
#define STAR_CURVE_25519_HPP
#include "StarEncode.hpp"
#include "StarByteArray.hpp"
#include "StarArray.hpp"

namespace Star::Curve25519 {

constexpr size_t PublicKeySize = 32;
constexpr size_t SecretKeySize = 32;
constexpr size_t PrivateKeySize = 64;
constexpr size_t SignatureSize = 64;

typedef Array<uint8_t, PublicKeySize> PublicKey;
typedef Array<uint8_t, SecretKeySize> SecretKey;
typedef Array<uint8_t, PrivateKeySize> PrivateKey;
typedef Array<uint8_t, SignatureSize> Signature;

PublicKey const& publicKey();
Signature sign(void* data, size_t len);
bool verify(uint8_t const* signature, uint8_t const* publicKey, void* data, size_t len);

}

#endif