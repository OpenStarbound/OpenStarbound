#include "StarUuid.hpp"
#include "StarRandom.hpp"
#include "StarFormat.hpp"
#include "StarEncode.hpp"

namespace Star {

Uuid::Uuid() : Uuid(Random::randBytes(UuidSize)) {}

Uuid::Uuid(ByteArray const& bytes) {
  if (bytes.size() != UuidSize)
    throw UuidException(strf("Size mismatch in reading Uuid from ByteArray: %s vs %s", bytes.size(), UuidSize));

  bytes.copyTo(m_data.ptr(), UuidSize);
}

Uuid::Uuid(String const& hex) : Uuid(hexDecode(hex)) {}

char const* Uuid::ptr() const {
  return m_data.ptr();
}

ByteArray Uuid::bytes() const {
  return ByteArray(m_data.ptr(), UuidSize);
}

String Uuid::hex() const {
  return hexEncode(m_data.ptr(), UuidSize);
}

bool Uuid::operator==(Uuid const& u) const {
  return m_data == u.m_data;
}

bool Uuid::operator!=(Uuid const& u) const {
  return m_data != u.m_data;
}

bool Uuid::operator<(Uuid const& u) const {
  return m_data < u.m_data;
}

bool Uuid::operator<=(Uuid const& u) const {
  return m_data <= u.m_data;
}

bool Uuid::operator>(Uuid const& u) const {
  return m_data > u.m_data;
}

bool Uuid::operator>=(Uuid const& u) const {
  return m_data >= u.m_data;
}

size_t hash<Uuid>::operator()(Uuid const& u) const {
  size_t hashval = 0;
  for (size_t i = 0; i < UuidSize; ++i)
    hashCombine(hashval, u.ptr()[i]);
  return hashval;
}

DataStream& operator>>(DataStream& ds, Uuid& uuid) {
  uuid = Uuid(ds.readBytes(UuidSize));
  return ds;
}

DataStream& operator<<(DataStream& ds, Uuid const& uuid) {
  ds.writeData(uuid.ptr(), UuidSize);
  return ds;
}

}
