#include "StarSecureRandom.hpp"
#include <windows.h>
#include <wincrypt.h>

namespace Star {

ByteArray secureRandomBytes(size_t size) {
  HCRYPTPROV context = 0;
  auto res = ByteArray(size, '\0');

  CryptAcquireContext(&context, 0, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  auto success = CryptGenRandom(context, size, (PBYTE)res.ptr());
  CryptReleaseContext(context, 0);

  if (!success)
    throw StarException("Could not read random bytes from source.");

  return res;
}

}
