#ifndef STAR_NET_ELEMENT_SYSTEM_HPP
#define STAR_NET_ELEMENT_SYSTEM_HPP

#include "StarNetElementBasicFields.hpp"
#include "StarNetElementFloatFields.hpp"
#include "StarNetElementSyncGroup.hpp"
#include "StarNetElementDynamicGroup.hpp"
#include "StarNetElementContainers.hpp"
#include "StarNetElementSignal.hpp"
#include "StarNetElementTop.hpp"

namespace Star {

// Makes a good default top-level NetElement group.
typedef NetElementTop<NetElementCallbackGroup> NetElementTopGroup;

}

#endif
