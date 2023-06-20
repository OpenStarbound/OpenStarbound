#include "StarNetElement.hpp"


namespace Star {

uint64_t NetElementVersion::current() const {
  return m_version;
}

void NetElementVersion::increment() {
  ++m_version;
}

void NetElement::enableNetInterpolation(float) {}

void NetElement::disableNetInterpolation() {}

void NetElement::tickNetInterpolation(float) {}

void NetElement::blankNetDelta(float) {}

}
