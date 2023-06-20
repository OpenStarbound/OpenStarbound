#ifndef STAR_SECURE_RANDOM_HPP
#define STAR_SECURE_RANDOM_HPP

#include "StarByteArray.hpp"

namespace Star {

// Generate cryptographically secure random numbers for usage in password salts
// and such using OS facilities
ByteArray secureRandomBytes(size_t size);

}

#endif
