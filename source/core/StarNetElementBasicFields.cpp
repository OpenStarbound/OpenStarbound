#include "StarNetElementBasicFields.hpp"

namespace Star {

void NetElementSize::readData(DataStream& ds, size_t& v) const {
  uint64_t s = ds.readVlqU();
  if (s == 0)
    v = NPos;
  else
    v = s - 1;
}

void NetElementSize::writeData(DataStream& ds, size_t const& v) const {
  if (v == NPos)
    ds.writeVlqU(0);
  else
    ds.writeVlqU(v + 1);
}

void NetElementBool::readData(DataStream& ds, bool& v) const {
  ds.read(v);
}

void NetElementBool::writeData(DataStream& ds, bool const& v) const {
  ds.write(v);
}

void NetElementEvent::trigger() {
  set(get() + 1);
}

uint64_t NetElementEvent::pullOccurrences() {
  uint64_t occurrences = get();
  starAssert(occurrences >= m_pulledOccurrences);
  uint64_t unchecked = occurrences - m_pulledOccurrences;
  m_pulledOccurrences = occurrences;
  return unchecked;
}

bool NetElementEvent::pullOccurred() {
  return pullOccurrences() != 0;
}

void NetElementEvent::ignoreOccurrences() {
  m_pulledOccurrences = get();
}

void NetElementEvent::setIgnoreOccurrencesOnNetLoad(bool ignoreOccurrencesOnNetLoad) {
  m_ignoreOccurrencesOnNetLoad = ignoreOccurrencesOnNetLoad;
}

void NetElementEvent::netLoad(DataStream& ds) {
  NetElementUInt::netLoad(ds);
  if (m_ignoreOccurrencesOnNetLoad)
    ignoreOccurrences();
}

void NetElementEvent::updated() {
  NetElementBasicField::updated();
  if (m_pulledOccurrences > get())
    m_pulledOccurrences = get();
}

}
