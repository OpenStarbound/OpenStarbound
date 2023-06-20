#include "StarSecureRandom.hpp"
#include "StarFile.hpp"

namespace Star {

ByteArray secureRandomBytes(size_t size) {
  return File::open("/dev/urandom", IOMode::Read)->readBytes(size);
}

}
